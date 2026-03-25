// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Manifest/Inv_ItemManifest.h"
#include "Items/Inv_InventoryItem.h"

UInv_InventoryItem* FInv_ItemManifest::Manifest(UObject* NewOuter) const
{
	// NewOuter: 해당 인벤토리 아이템을 담당하는 객체. 즉, 이 메모리를 소유하는 부모는 누구인지?
	// NewOuter가 삭제되면 생성되는 NewObject도 삭제됨
	UInv_InventoryItem* Item = NewObject<UInv_InventoryItem>(NewOuter, UInv_InventoryItem::StaticClass());
	Item->SetItemManifest(*this);
	
	return Item;
}
