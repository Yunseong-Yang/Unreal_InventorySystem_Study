// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/Inv_InfoMessage.h"

#include "Components/TextBlock.h"

void UInv_InfoMessage::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Text_Message->SetText(FText::GetEmpty());
	HideMessage();
}

void UInv_InfoMessage::SetMessage(const FText& Message)
{
	Text_Message->SetText(Message);
	
	if (!bIsMessageActive)
	{
		ShowMessage();
	}
	bIsMessageActive = true;
	
	GetWorld()->GetTimerManager().SetTimer(Timer, [this]()
	{
		HideMessage();
	}, MessageLifeTime, false);
}
