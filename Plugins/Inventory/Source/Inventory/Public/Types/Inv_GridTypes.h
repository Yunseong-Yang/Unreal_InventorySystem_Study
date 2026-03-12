// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Inv_GridTypes.generated.h"

UENUM(BlueprintType)
enum class EInv_ItemCategory : uint8
{
	Equippable,
	Consumable,
	Craftable,
	None
};