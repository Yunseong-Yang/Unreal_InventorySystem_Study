// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/Manifest/Inv_ItemManifest.h"
#include "Inv_ItemComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable )
class INVENTORY_API UInv_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInv_ItemComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	FInv_ItemManifest GetItemManifest() const { return ItemManifest; }
	FString GetPickupMessage() const { return PickupMessage; }
private:
	
	UPROPERTY(EditAnywhere, Category = "Inventory", Replicated)
	FInv_ItemManifest ItemManifest;	
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FString PickupMessage;
	
};
