// Fill out your copyright notice in the Description page of Project Settings.


#include "Inv_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Widgets/HUD/Inv_HUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Interaction/Inv_Highlightable.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"

AInv_PlayerController::AInv_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	TraceLength = 500.0;
	ItemChannel = ECollisionChannel::ECC_EngineTraceChannel1;
}

void AInv_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TraceForItem();
}

void AInv_PlayerController::ToggleInventory()
{
	InventoryComponent = FindComponentByClass<UInv_InventoryComponent>();

	if (!InventoryComponent.IsValid()) return;

	InventoryComponent->ToggleInventoryMenu();
}

void AInv_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (IsValid(Subsystem))
	{
		for (UInputMappingContext* DefaultIMC : DefaultIMCs)
		{
			Subsystem->AddMappingContext(DefaultIMC, 0);
			UE_LOG(LogTemp, Warning, TEXT("IMC Successfully Added"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Subsystem is not valid"));
	}

	CreateHUDWidget();
}

void AInv_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &AInv_PlayerController::PrimaryInteract);
		EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AInv_PlayerController::ToggleInventory);
	}
}

void AInv_PlayerController::PrimaryInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("Primary Interact"));
}

void AInv_PlayerController::CreateHUDWidget()
{
	HUDWidget = CreateWidget<UInv_HUDWidget>(this, HUDWidgetClass);
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
	}
}

void AInv_PlayerController::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	ViewportSize /= 2;

	FVector TraceStart;
	FVector Direction;
	if (UGameplayStatics::DeprojectScreenToWorld(this, ViewportSize, TraceStart, Direction))
	{
		FVector TraceEnd = TraceStart + Direction * TraceLength;
		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemChannel);

		LastActor = ThisActor;
		ThisActor = HitResult.GetActor();

		if (!ThisActor.IsValid())
		{
			if (IsValid(HUDWidget))
			{
				HUDWidget->HidePickupWidget();
			}
		}

		if (LastActor == ThisActor) return;

		if (ThisActor.IsValid())
		{
			// UInv_Highlightable를 가지고 있는 모든 컴포넌트 순회. UInv_HighlightableStaticMesh를 반환
			if (UActorComponent* Highlightable = ThisActor->FindComponentByInterface(UInv_Highlightable::StaticClass()); IsValid(Highlightable))
			{
				IInv_Highlightable::Execute_Highlight(Highlightable);
			}

			UInv_ItemComponent* comp = ThisActor->FindComponentByClass<UInv_ItemComponent>();

			if (IsValid(HUDWidget))
			{
				HUDWidget->ShowPickupWidget(comp->GetPickupMessage());
			}
		}

		if (LastActor.IsValid())
		{
			if (UActorComponent* Highlightable = LastActor->FindComponentByInterface(UInv_Highlightable::StaticClass()); IsValid(Highlightable))
			{
				IInv_Highlightable::Execute_UnHighlight(Highlightable);
			}
		}
	}
}
