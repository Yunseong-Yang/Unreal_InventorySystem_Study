// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/Inv_HUDWidget.h"

#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Widgets/HUD/Inv_InfoMessage.h"

void UInv_HUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	UInv_InventoryComponent* Inv_InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	
	if (!IsValid(Inv_InventoryComponent)) return;
	
	Inv_InventoryComponent->NoRoomInInventory.AddDynamic(this, &ThisClass::UInv_HUDWidget::OnNoRoom);
}

void UInv_HUDWidget::OnNoRoom()
{
	if (!IsValid(InfoMessage)) return;
	
	InfoMessage->SetMessage(FText::FromString("No Room In Inventory"));
}

