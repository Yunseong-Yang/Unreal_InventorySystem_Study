// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Inv_GridTypes.generated.h"

class UInv_InventoryItem;

UENUM(BlueprintType)
enum class EInv_ItemCategory : uint8
{
	Equippable,
	Consumable,
	Craftable,
	None
};

USTRUCT()
struct FInv_SlotAvailability
{
	GENERATED_BODY()
	
public:
	FInv_SlotAvailability(){}
	FInv_SlotAvailability(int32 ItemIndex, int32 Room, bool bHasItem)
		: Index(ItemIndex), AmountToFill(bHasItem), bItemAtIndex(bHasItem) {}
	
	int32 Index{INDEX_NONE};
	int32 AmountToFill{0};
	bool bItemAtIndex{false};
};

USTRUCT()
struct FInv_SlotAvailabilityResult
{
	GENERATED_BODY()
	
public:
	FInv_SlotAvailabilityResult(){}
	
	TWeakObjectPtr<UInv_InventoryItem> Item;
	int32 TotalRoomToFill{0};
	int32 Remainder{0};
	bool bStackable{false};
	TArray<FInv_SlotAvailability> AvailableSlots;
};