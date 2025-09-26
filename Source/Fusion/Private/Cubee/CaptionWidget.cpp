// Fill out your copyright notice in the Description page of Project Settings.


#include "Cubee/CaptionWidget.h"

#include "Components/Button.h"
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
