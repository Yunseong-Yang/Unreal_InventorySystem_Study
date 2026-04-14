// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Inv_InventoryComponent.h"

#include "Items/Components/Inv_ItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Items/Inv_InventoryItem.h"

UInv_InventoryComponent::UInv_InventoryComponent()
	: InventoryList(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	// UObject도 동기화를 하는데, 내가 리스트에 수동으로 등록해 둔 객체들만 알아서 동기화해 줘
	bReplicateUsingRegisteredSubObjectList = true;
	bInventoryMenuOpen = false;
}

void UInv_InventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, InventoryList);
}

void UInv_InventoryComponent::TryAddItem(UInv_ItemComponent* ItemComponent)
{
	if (!IsValid(ItemComponent)) return;
	
	FInv_SlotAvailabilityResult Result = InventoryMenu->HasRoomForItem(ItemComponent);
	
	if (Result.TotalRoomToFill == 0)
	{
		NoRoomInInventory.Broadcast();
		return;	
	}
	
	// 인벤토리에 아이템 추가
	UInv_InventoryItem* FoundItem = InventoryList.FindFirstItemByType(ItemComponent->GetItemManifest().GetItemType());
	Result.Item = FoundItem;
	
	// 아이템이 이미 있고 중첩 가능하다면
	if (Result.Item.IsValid() && Result.bStackable)
	{
		// 기존 인벤토리 공간에 아이템을 넣어.
		Server_AddStacksToItem(ItemComponent, Result.TotalRoomToFill, Result.Remainder);
	}
	else 
	{
		// 새 공간에 아이템을 넣되, 중첩 가능한 아이템이 아니라면 중첩 개수 0을 전달해
		Server_AddNewItem(ItemComponent, Result.bStackable ? Result.TotalRoomToFill : 0);
	}
}


void UInv_InventoryComponent::ToggleInventoryMenu()
{
	bInventoryMenuOpen ? CloseInventoryMenu() : OpenInventoryMenu();
}

void UInv_InventoryComponent::AddRepSubObj(UObject* SubObj)
{
	// InventoryComponent가 동기화 될 때 SubObj도 같이 동기화 시켜주라!
	// bReplicateUsingRegisteredSubObjectList가 true인지
	// InventoryComponent가 복제할 준비가 되었는지
	// SubObj이 유효한지
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObj))
	{
		// SubObj를 복제 리스트에 넣어라.
		AddReplicatedSubObject(SubObj);	
	}
}


void UInv_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	ConstructInventory();
}

void UInv_InventoryComponent::Server_AddNewItem_Implementation(UInv_ItemComponent* ItemComponent, int StackCount)
{
	UInv_InventoryItem* NewItem = InventoryList.AddEntry(ItemComponent);
	
	NewItem->SetTotalStackCount(StackCount);
	
	if (GetOwner()->GetNetMode() == NM_Standalone || GetOwner()->GetNetMode() == NM_ListenServer)
	{
		OnItemAdded.Broadcast(NewItem);
	}
	
	// 집으려는 액터 파괴
}

void UInv_InventoryComponent::Server_AddStacksToItem_Implementation(UInv_ItemComponent* ItemComponent, int StackCount,
	int Remainder)
{
	const FGameplayTag& ItemType = IsValid(ItemComponent) ? ItemComponent->GetItemManifest().GetItemType() : FGameplayTag::EmptyTag;
	UInv_InventoryItem* FoundItem = InventoryList.FindFirstItemByType(ItemType);
	if (!IsValid(FoundItem)) return;
	
	FoundItem->SetTotalStackCount(StackCount + FoundItem->GetTotalStackCount());
	// 잔여 수량 : 0 -> 땅에 있는 아이템 삭제
	// 잔여 수량 : 0보다 크면 땅에 있는 아이템의 스택 수량을 잔여 수량으로 설정
}

void UInv_InventoryComponent::ConstructInventory()
{
	OwningController = Cast<APlayerController>(GetOwner());
	checkf(OwningController.IsValid(), TEXT("Inventory Component don't have a PlayerController. Check Owner."));

	// 직접 조종하는 클라가 아니면 리턴
	// HasAuthority()로 예외처리 하면 리슨 서버에서 호스트가 인벤토리를 못 염
	if (!OwningController->IsLocalController()) return; 

	InventoryMenu = CreateWidget<UInv_InventoryBase>(OwningController.Get(), InventoryMenuClass);
	InventoryMenu->AddToViewport();
	CloseInventoryMenu();
}

void UInv_InventoryComponent::OpenInventoryMenu()
{
	if (!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Visible);
	bInventoryMenuOpen = true;

	if (!OwningController.IsValid()) return;

	FInputModeGameAndUI InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(true);
}

void UInv_InventoryComponent::CloseInventoryMenu()
{
	if (!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Collapsed);
	bInventoryMenuOpen = false;

	if (!OwningController.IsValid()) return;

	FInputModeGameOnly InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(false);
}

