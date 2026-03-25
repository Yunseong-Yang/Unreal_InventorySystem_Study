// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/Inv_GridTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "GameplayTagContainer.h"
#include "Inv_ItemManifest.generated.h"

/*
 * 아이템 매니페스트는 새로운 인벤토리 아이템을 생성하는 데 필요한 모든 데이터를 포함. 
 */

class UInv_InventoryItem;

USTRUCT(BlueprintType)
struct INVENTORY_API FInv_ItemManifest
{
	GENERATED_BODY()
	
public:
	// 실제 아이템을 생성하는 함수
	UInv_InventoryItem* Manifest(UObject* NewOuter) const;
	
	EInv_ItemCategory GetItemCategory() const { return ItemCategory; }
	FGameplayTag GetItemType() const { return ItemType; }
	
private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	EInv_ItemCategory ItemCategory{EInv_ItemCategory::None};
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FGameplayTag ItemType;
};
