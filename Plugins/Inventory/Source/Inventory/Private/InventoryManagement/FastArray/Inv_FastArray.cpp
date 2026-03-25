#include "InventoryManagement/FastArray/Inv_FastArray.h"

#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"

TArray<UInv_InventoryItem*> FInv_InventoryFastArray::GetAllItems() const
{
	TArray<UInv_InventoryItem*> Results;
	Results.Reserve(Entries.Num());
	for (const FInv_InventoryEntry& Entry : Entries)
	{
		if (!IsValid(Entry.Item)) continue;
		Results.Add(Entry.Item);
	}
	
	return Results;
}

void FInv_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	UInv_InventoryComponent* IC = Cast<UInv_InventoryComponent>(OwnerComponent);
	if (!IsValid(IC)) return;
	
	for (int32 index : RemovedIndices)
	{
		// Item이 삭제되었다고 방송함
		IC->OnItemRemoved.Broadcast(Entries[index].Item);
	}
}

void FInv_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UInv_InventoryComponent* IC = Cast<UInv_InventoryComponent>(OwnerComponent);
	if (!IsValid(IC)) return;
	
	for (int32 index : AddedIndices)
	{
		// Item이 추가되었다고 방송함. 
		// 단, PostReplicatedAdd는 클라이언트에만 호출되므로 Host, StandAlone는 아이템이 추가된 시점(UInv_InventoryComponent::Server_AddNewItem 등)에서 다시 방송
		IC->OnItemAdded.Broadcast(Entries[index].Item);
	}
}

UInv_InventoryItem* FInv_InventoryFastArray::AddEntry(UInv_ItemComponent* ItemComponent)
{
	check(OwnerComponent);
	
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	UInv_InventoryComponent* IC = Cast<UInv_InventoryComponent>(OwnerComponent);
	if (!IsValid(IC)) return nullptr;
	
	FInv_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	
	// Item은 UObject로 상속되었기 때문에 서버에서 아이템을 생성해도 클라는 이를 모름(복제는 Actor 단위)
	// Replicated Subobject를 사용하여 이를 동기화 시켜줘야 함 -> InventoryComponent
	NewEntry.Item = ItemComponent->GetItemManifest().Manifest(OwningActor);
	
	IC->AddRepSubObj(NewEntry.Item);
	MarkItemDirty(NewEntry);
	
	return NewEntry.Item;
}

UInv_InventoryItem* FInv_InventoryFastArray::AddEntry(UInv_InventoryItem* Item)
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	
	// Entries 배열 맨 끝에 공간을 추가하고 해당 공간에 대한 참조를 넘긴 후
	FInv_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	// 해당 공간에 아이템을 넣고
	NewEntry.Item = Item;
	// 아이템을 넣었다는 신호를 주기 위해 해당 공간의 참조를 넘긴다!
	MarkItemDirty(NewEntry);
	
	return Item;
}

void FInv_InventoryFastArray::RemoveEntry(UInv_InventoryItem* Item)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		// 해당 반복자 위치의 참조를 얻고
		FInv_InventoryEntry& Entry = *EntryIt;
		if (Entry.Item == Item)
		{
			// 요소가 같으면 반복자를 이용하여 현재 요소 제거
			EntryIt.RemoveCurrent();
			// 여기서는 배열 전체가 바뀌었다고 신호를 줌!
			MarkArrayDirty();
		}
	}
}
