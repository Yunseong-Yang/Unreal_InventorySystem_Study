#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "Inv_FastArray.generated.h"

class UInv_ItemComponent;
class UInv_InventoryComponent;
class UInv_InventoryItem;

/* 인벤토리 단일 엔트리 */
USTRUCT(BlueprintType)
struct FInv_InventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY();
	
	FInv_InventoryEntry(){}
	
private:
	friend struct FInv_InventoryFastArray;
	friend UInv_InventoryComponent;
	
	UPROPERTY()
	TObjectPtr<UInv_InventoryItem> Item = nullptr;
};

USTRUCT(BlueprintType)
struct FInv_InventoryFastArray : public FFastArraySerializer
{
	GENERATED_BODY();
public:
	FInv_InventoryFastArray() : OwnerComponent(nullptr) {};
	FInv_InventoryFastArray(UActorComponent *InOwnerComponent)
		: OwnerComponent(InOwnerComponent) {}

	TArray<UInv_InventoryItem*> GetAllItems() const;
	
	// Entries 배열을 FastArray(바뀐 부분만 찾는) 방식으로 직렬화를 수행
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FInv_InventoryEntry, FInv_InventoryFastArray>(Entries, DeltaParams, *this);
	}
	
	/* 아이템을 추가/삭제하기 위해 아래 함수들이 필요. 가상 함수가 아님을 유의 */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	/* Fast Array Serializer 연결 끝 부분 */
	
	UInv_InventoryItem* AddEntry(UInv_ItemComponent* ItemComponent);
	UInv_InventoryItem* AddEntry(UInv_InventoryItem* Item);
	void RemoveEntry(UInv_InventoryItem* Item);
private:
	friend UInv_InventoryComponent;
	
	UPROPERTY()
	TArray<FInv_InventoryEntry> Entries;
	
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

// 기존의 통째로 복사하는 방식이 아닌 직렬화 함수를 쓸 것이니 미리 언급
template<>
struct TStructOpsTypeTraits<FInv_InventoryFastArray> : public TStructOpsTypeTraitsBase2<FInv_InventoryFastArray>
{
	enum { WithNetDeltaSerializer = true };
};