// Fill out your copyright notice in the Description page of Project Settings.


#include "Cubee/MainWidget.h"

#include "Components/TextBlock.h"

void UMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ViewGuideText();

	if (SlideOut)
	{
		FWidgetAnimationDynamicEvent EndEvent;
		EndEvent.BindDynamic(this, &UMainWidget::HandleSlideOutFinished);

		BindToAnimationFinished(SlideOut, EndEvent);
	}

	/////////////// Test
	//FTimerHandle TimerHandle;
	//GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([&]
	//{
	//	HideGuideText();
	//	
	//}), 3.f, false);
	//////////////
}

void UMainWidget::UpdateGestureState(const FString& NewState)
{
	Txt_Gesture->SetText(FText::FromString(NewState));
}

void UMainWidget::ViewGuideText()
{
	if (SlideIn)
	{
		PlayAnimation(SlideIn);
	}
}

void UMainWidget::HideGuideText()
{
	if (SlideOut)
	{
		PlayAnimation(SlideOut);
	}
}

void UMainWidget::HandleSlideOutFinished()
{
	OnTextHideFinished.Broadcast();
}
