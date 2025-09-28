// Fill out your copyright notice in the Description page of Project Settings.


#include "Cubee/CaptionWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Cubee/MainWidget.h"
#include "Cubee/RecorderComponent.h"
#include "Fusion/FusionMode.h"
#include "Huxley/CameraManager.h"
#include "Kismet/GameplayStatics.h"

void UCaptionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlideOut)
	{
		FWidgetAnimationDynamicEvent EndEvent;
		EndEvent.BindDynamic(this, &UCaptionWidget::HandleSlideOutFinished);

		BindToAnimationFinished(SlideOut, EndEvent);
	}

	/*
	AGameModeBase* GM =UGameplayStatics::GetGameMode(GetWorld());
	if (GM)
	{
		AFusionMode* FM = Cast<AFusionMode>(GM);

		if (FM)
		{
			FM->OnVoiceAnswerReceived.AddDynamic(this, &UCaptionWidget::EnterCaption);
		}
	}
	*/
}

bool UCaptionWidget::Initialize()
{
	if (!Super::Initialize())
	return false;
	
	if (Btn_Record)
	{
		Btn_Record->OnHovered.AddDynamic(this, &UCaptionWidget::UCaptionWidget::OnButtonHovered);
		Btn_Record->OnUnhovered.AddDynamic(this, &UCaptionWidget::UCaptionWidget::OnButtonUnhovered);
	}

	// Typing init
	CurrentText = "";
	CurrentIndex = 0;

	Txt_Q->SetVisibility(ESlateVisibility::Hidden);
	Txt_A->SetVisibility(ESlateVisibility::Hidden);
	
	return true;
}

void UCaptionWidget::OnButtonHovered()
{
	if (bIsHovering) return;

	OnRecordStart();
}

void UCaptionWidget::OnButtonUnhovered()
{
	OnRecordEnd();

	/////////////// Test
	ExitCaption();
	//////////////
}

void UCaptionWidget::OnRecordStart()
{
	if (RecordingComponent)
	{
		RecordingComponent->StartRecording();
	}
}

void UCaptionWidget::OnRecordEnd()
{
	if (RecordingComponent)
	{
		RecordingComponent->StopRecordingAndSave(TEXT("Test"));
	}
}

void UCaptionWidget::SetRecordingComponent(URecorderComponent* InComponent)
{
	RecordingComponent = InComponent;
}

void UCaptionWidget::PrintStart(UTextBlock* _TextBlock, const FString& NewText)
{
	FullText = NewText;

	InitializeTextBlock(_TextBlock);
	
	GetWorld()->GetTimerManager().SetTimer(TypingTimerHandle, FTimerDelegate::CreateLambda([&]
	{
		TypeNextCharacter(_TextBlock);
	}), 0.05f, true);
}

void UCaptionWidget::TypeNextCharacter(UTextBlock* _TextBlock)
{
	if (CurrentIndex < FullText.Len())
	{
		CurrentText.AppendChar(FullText[CurrentIndex]);
		if (_TextBlock)
		{
			_TextBlock->SetText(FText::FromString(CurrentText));
		}
		CurrentIndex++;
	}
	else
	{
		// 다 출력되면 타이머 정지
		GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
	}
}

void UCaptionWidget::InitializeTextBlock(UTextBlock* _TextBlock)
{
	CurrentIndex = 0;
	CurrentText = "";

	// 텍스트 초기화
	if (_TextBlock)
	{
		_TextBlock->SetText(FText::FromString(""));
	}
}

void UCaptionWidget::EnterCaption(const FString& Q, const FString& A)
{
	Txt_Q->SetVisibility(ESlateVisibility::Visible);
	Txt_A->SetVisibility(ESlateVisibility::Visible);

	PrintStart(Txt_Q, Q);
	PrintStart(Txt_A, A);
	
	//Txt_Q->SetText(FText::FromString(Q));
	//Txt_A->SetText(FText::FromString(A));
	
	if (SlideIn)
	{
		PlayAnimation(SlideIn);
	}
}

void UCaptionWidget::ExitCaption()
{
	GetWorld()->GetTimerManager().ClearTimer(TypingTimerHandle);
	
	if (SlideOut)
	{
		PlayAnimation(SlideOut);
	}
}

void UCaptionWidget::HandleSlideOutFinished()
{
	OnSlideOutFinished.Broadcast();
}



