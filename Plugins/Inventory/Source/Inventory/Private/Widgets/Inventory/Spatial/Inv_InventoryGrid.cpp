// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"

void UInv_InventoryGrid::NativeConstruct()
{
	Super::NativeConstruct();
	
	ConstructGrid();
	InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::AddItem);
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_ItemComponent* ItemComponent)
{
	return HasRoomForItem(ItemComponent->GetItemManifest());
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_InventoryItem* Item)
{
	return HasRoomForItem(Item->GetItemManifest());
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const FInv_ItemManifest& ItemManifest)
{
	FInv_SlotAvailabilityResult Result;
	
	// 1. 이 아이템이 스택 가능한지(Stackable) 결정 (매우 중요함. 처리 방식이 완전히 달라짐)
	const FInv_StackableFragment* StackableFragment = ItemManifest.GetFragmentOfType<FInv_StackableFragment>();
	Result.bStackable = StackableFragment != nullptr;
	
	// 2. 만약 스택 가능하다면, 총 몇 개의 스택(Stack Amount)을 추가해야 하는지 결정
    const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : 1;
	int32 AmountToFill = StackableFragment ? StackableFragment->GetStackCount() : 1;
	
	TSet<int32> ClaimedIndices;
    // 3. 인벤토리의 모든 그리드 슬롯을 순회하는 메인 루프 시작
	for (const auto& GridSlot: GridSlots)
	{
        // 4. (조기 종료 조건) 만약 더 이상 배치할 남은 아이템 수량(Amount to fill)이 없다면 루프 종료 (Break)
        if (AmountToFill == 0) break;
		
		if (IsIndexClaimed(ClaimedIndices, GridSlot->GetIndex())) continue;
		
        // --- 위치별 검사 시작 ---
        // 5. 이 칸이 내가 놓으려는 아이템의 2D 크기(Grid Dimensions)를 수용할 수 있는가?
        //    (즉, 인벤토리 그리드 바깥으로 삐져나가지 않는가? Out of bounds 검사)
		if (!IsInGridBounds(GridSlot->GetIndex(), GetItemDimensions(ItemManifest))) continue;
		
		TSet<int32> TentativelyClaimed;
        if (!HasRoomAtIndex(GridSlot, GetItemDimensions(ItemManifest), ClaimedIndices, TentativelyClaimed, ItemManifest.GetItemType(), MaxStackSize))
        {
	        continue;
        }
        
        // --- 수량 할당 ---
        // 8. 이 슬롯에 몇 개의 아이템을 채울 수 있는지 계산
		const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Result.bStackable, MaxStackSize, AmountToFill, GridSlot);
		
		if (AmountToFillInSlot == 0) continue;
		ClaimedIndices.Append(TentativelyClaimed);
		
        // 9. 남은 수량(Amount left to fill) 갱신
		Result.TotalRoomToFill -= AmountToFillInSlot;
		Result.AvailableSlots.Emplace(
			FInv_SlotAvailability{
				HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetIndex(),
				Result.bStackable ? AmountToFillInSlot : 0,
				HasValidItem(GridSlot)
			}	
		);
		// 10. 남은 수량(Remainder)이 있는지 확인
		AmountToFill -= AmountToFillInSlot;
		Result.Remainder = AmountToFill;
		if (AmountToFill == 0) break;
	}
	// 11. 1~10의 모든 계산 결과를 FInvSlotAvailabilityResult 구조체에 꽉꽉 채워 담아서 반환
	
	return Result;
}

FIntPoint UInv_InventoryGrid::GetItemDimensions(const FInv_ItemManifest& ItemManifest)
{
	const FInv_GridFragment* GridFragment = ItemManifest.GetFragmentOfType<FInv_GridFragment>();
	FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	return Dimensions;
}

bool UInv_InventoryGrid::HasValidItem(const UInv_GridSlot* GridSlot) const
{
	return GridSlot->GetInventoryItem().IsValid();
}

bool UInv_InventoryGrid::IsUpperLeftSlot(const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot) const
{
	return SubGridSlot->GetUpperLeftIndex() == GridSlot->GetIndex();
}

bool UInv_InventoryGrid::DoesItemTypeMatch(const UInv_InventoryItem* SubItem, const FGameplayTag& ItemType) const
{
	return SubItem->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
}

bool UInv_InventoryGrid::IsInGridBounds(const int32 StartIndex, const FIntPoint ItemDimensions) const
{	
	if (StartIndex < 0 || StartIndex >= GridSlots.Num()) return false;
	
	const int32 EndColumn = (StartIndex % Columns) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / Columns) + ItemDimensions.Y;
	
	return EndColumn <= Columns && EndRow <= Rows;
}

int32 UInv_InventoryGrid::DetermineFillAmountForSlot(const bool bStackable, const int32 MaxStackSize,
	const int32 AmountToFill, const UInv_GridSlot* GridSlot) const
{
	// 1. 슬롯의 여유 공간(Room in Slot) 계산
	const int32 RoomInSlot = MaxStackSize - GetStackAmount(GridSlot);
	// 2. 최종 채울 수량 계산 (삼항 연산자 활용)
	// 스택 가능한 아이템이라면: 우리가 채우고 싶은 수량(AmountToFill)과 슬롯의 여유 공간(RoomInSlot) 중 더 '작은' 값을 선택합니다.
	// 스택이 불가능하다면: 무조건 1개만 반환합니다.
	return bStackable ? FMath::Min(AmountToFill, RoomInSlot) : 1;
}

int32 UInv_InventoryGrid::GetStackAmount(const UInv_GridSlot* GridSlot) const
{
	// 일단 현재 슬롯의 스택 수를 가져옵니다.
	int32 CurrentSlotStackCount = GridSlot->GetStackCount();
	// 이 슬롯에 상위 앵커(UpperLeftIndex)가 등록되어 있는지 확인합니다.
	// C++17 문법: if문 안에서 변수 선언과 조건 검사를 동시에 수행
	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex(); UpperLeftIndex != INDEX_NONE)
	{
		// 앵커가 존재한다면 이 칸은 다중 칸 아이템의 일부일 뿐입니다.
		// 진짜 머리통(앵커 슬롯)을 찾아가서 그곳에 기록된 진짜 스택 수량을 빼앗아 옵니다.
		const UInv_GridSlot* UpperLeftGridSlot = GridSlots[UpperLeftIndex];
		CurrentSlotStackCount = UpperLeftGridSlot->GetStackCount();
	}
	
	return CurrentSlotStackCount;
}

bool UInv_InventoryGrid::HasRoomAtIndex(const UInv_GridSlot* GridSlot, const FIntPoint& Dimensions, const TSet<int32>& CheckedIndices, TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType, const int32 MaxStackSize)
{
	bool bHasRoomAtIndex = true;
	// 6. 이 칸(Index)에 빈 공간이 있는가?
	//    (즉, 내 아이템이 덮게 될 2D 범위(ForEach2D) 안에 다른 방해물이 없는가?)
	UInv_InventoryStatics::ForEach2D(GridSlots, GridSlot->GetIndex(), Dimensions, Columns, [&](const UInv_GridSlot* SubGridSlot)
	{
		if (CheckSlotConstraints(GridSlot, SubGridSlot, CheckedIndices, OutTentativelyClaimed, ItemType, MaxStackSize))
		{
			OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		}
		else
		{
			bHasRoomAtIndex = false;
		}
	});
	
	return bHasRoomAtIndex;
}

bool UInv_InventoryGrid::CheckSlotConstraints(const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot, const TSet<int32>& CheckedIndices, TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType, const int32 MaxStackSize) const
{
	// 7. 기타 중요 조건 검사
	//    - 아이템이 차지할 공간 중 다른 아이템에 선점(Claimed)된 칸이 있는가?
	if (IsIndexClaimed(CheckedIndices, SubGridSlot->GetIndex())) return false;
	//    - 해당 위치에 이미 유효한 아이템이 존재하는가?
	if (!HasValidItem(SubGridSlot))
	{
		OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		return true;
	}
	
	// 현재 슬롯이 왼쪽 상단 슬롯인지?
	if (!IsUpperLeftSlot(SubGridSlot, SubGridSlot)) return false;
	
	// 만약 유효한 아이템이 있다면
	//        - 같은 종류라면 스택이 가능한가?
	const UInv_InventoryItem* SubItem = SubGridSlot->GetInventoryItem().Get();
	if (!SubItem->IsStackable()) return false;
	//        - 지금 줍는 아이템과 같은 종류(Type)인가?
	if (!DoesItemTypeMatch(SubItem, ItemType)) return false;
	//        - 스택이 가능하다면 해당 슬롯이 이미 최대 수량(Max Capacity)에 도달하지 않았는가?
	if (GridSlot->GetStackCount() >= MaxStackSize) return false;
	return true;
}

void UInv_InventoryGrid::AddItem(UInv_InventoryItem* Item)
{
	if (!MatchesCategory(Item)) return;
	
	FInv_SlotAvailabilityResult Result = HasRoomForItem(Item);
	AddItemToIndices(Result, Item);
}



void UInv_InventoryGrid::AddItemToIndices(const FInv_SlotAvailabilityResult& Result, UInv_InventoryItem* Item)
{
	for (const auto& Availability : Result.AvailableSlots)
	{
		AddItemAtIndex(Item, Availability.Index, Result.bStackable, Availability.AmountToFill);
		UpdateGridSlots(Item, Availability.Index, Result.bStackable, Availability.AmountToFill);
	}
}

void UInv_InventoryGrid::AddItemAtIndex(UInv_InventoryItem* Item, const int32 Index, const bool bStackable,
	const int32 StackAmount)
{
	// TODO 1: 아이템이 몇 칸을 차지하는지 알기 위해 Grid Fragment 가져오기
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	// TODO 2: 화면에 표시할 아이콘을 알기 위해 Image Fragment 가져오기
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(Item, FragmentTags::ImageFragment);
		
	if (!ImageFragment || !GridFragment) return;
		
	// TODO 3: 그리드에 추가할 위젯 생성 및 추가하기
	UInv_SlottedItem* SlottedItem = CreateSlottedItem(Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	AddSlottedItemToCanvas(Index, GridFragment, SlottedItem);
	
	// TODO 4: 나중에 아이템을 버리거나 소모할 때 지울 수 있도록, 생성된 위젯을 컨테이너에 저장하기
	SlottedItems.Add(Index, SlottedItem);
}

UInv_SlottedItem* UInv_InventoryGrid::CreateSlottedItem(UInv_InventoryItem* Item, const bool bStackable,
	const int32 StackAmount, const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment, int32 Index)
{
	UInv_SlottedItem* SlottedItem = CreateWidget<UInv_SlottedItem>(GetOwningPlayer(), SlottedItemClass);
	SlottedItem->SetInventoryItem(Item);
	SetSlottedItemImage(GridFragment, ImageFragment, SlottedItem);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetStackable(bStackable);
	const int32 StackUpdateAmount = bStackable ? StackAmount : 0;
	SlottedItem->UpdateStackCount(StackUpdateAmount);
	
	return SlottedItem;
}

void UInv_InventoryGrid::AddSlottedItemToCanvas(const int32 Index, const FInv_GridFragment* GridFragment,
	UInv_SlottedItem* SlottedItem) const
{
	CanvasPanel->AddChild(SlottedItem);
	UCanvasPanelSlot* CanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	CanvasPanelSlot->SetSize(GetDrawSize(GridFragment));
	const FVector2D DrawPos = UInv_WidgetUtils::GetPositionFromIndex(Index, Columns) * TileSize;
	const FVector2D DrawPosWithPadding = DrawPos + FVector2D(GridFragment->GetGridPadding());
	CanvasPanelSlot->SetPosition(DrawPosWithPadding);
}

void UInv_InventoryGrid::UpdateGridSlots(UInv_InventoryItem* Item, const int32 Index, bool bStackable, const int32 StackAmount)
{
	check(GridSlots.IsValidIndex(Index));
	
	if (bStackable)
	{
		GridSlots[Index]->SetStackCount(StackAmount);
	}
	
	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
	UInv_InventoryStatics::ForEach2D(GridSlots, Index, Dimensions, Columns, [&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(Item);
		GridSlot->SetUpperLeftIndex(Index);
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailable(false);;
	});
	
	UInv_GridSlot* GridSlot = GridSlots[Index];
	GridSlot->SetOccupiedTexture();
}

bool UInv_InventoryGrid::IsIndexClaimed(const TSet<int32>& ClaimedIndices, const int32 Index) const
{
	return ClaimedIndices.Contains(Index);
}

void UInv_InventoryGrid::SetSlottedItemImage(const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment, UInv_SlottedItem* SlottedItem)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = GetDrawSize(GridFragment);
	SlottedItem->SetImageBrush(Brush);
}


FVector2D UInv_InventoryGrid::GetDrawSize(const FInv_GridFragment* GridFragment) const
{
	const float IconTileWidth = TileSize - GridFragment->GetGridPadding() * 2;
	FVector2D IconSize = GridFragment->GetGridSize() * IconTileWidth;
	return IconSize;
}

void UInv_InventoryGrid::ConstructGrid()
{
	GridSlots.Reserve(Rows * Columns);
	
	for (int j = 0; j < Rows; ++j)
	{
		for (int i = 0; i < Columns; ++i)
		{
			UInv_GridSlot* GridSlot = CreateWidget<UInv_GridSlot>(this, GridSlotClass);
			CanvasPanel->AddChildToCanvas(GridSlot);
			
			const FIntPoint TilePosition(i, j);
			int32 index = UInv_WidgetUtils::GetIndexFromPosition(TilePosition, Columns);
			GridSlot->SetTileIndex(index);
			
			UCanvasPanelSlot* GridCanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
			GridCanvasPanelSlot->SetSize(FVector2D(TileSize, TileSize));
			GridCanvasPanelSlot->SetPosition(FVector2D(TilePosition.X * TileSize, TilePosition.Y * TileSize));
			
			GridSlots.Add(GridSlot);
		}
	}
}

bool UInv_InventoryGrid::MatchesCategory(const UInv_InventoryItem* Item)
{
	return Item->GetItemManifest().GetItemCategory() == ItemCategory;
}
