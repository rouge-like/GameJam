// Fill out your copyright notice in the Description page of Project Settings.


#include "Cubee/CaptionWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Cubee/RecorderComponent.h"

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

void UCaptionWidget::PrintStart(const FString& NewText)
{
	FullText = NewText;

	InitializeTextBlock(bIsQuestion? Txt_Q : Txt_A);
	
	GetWorld()->GetTimerManager().SetTimer(TypingTimerHandle, FTimerDelegate::CreateLambda([&]
	{
		TypeNextCharacter(bIsQuestion? Txt_Q : Txt_A);
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

void UCaptionWidget::EnterCaption()
{
	// 동물 확대되면 
}

void UCaptionWidget::ExitCaption()
{
}



