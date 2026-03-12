// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"

void UInv_InventoryGrid::NativeConstruct()
{
	Super::NativeConstruct();
	
	ConstructGrid();
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
