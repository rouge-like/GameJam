// Fill out your copyright notice in the Description page of Project Settings.


#include "Cubee/CaptionWidget.h"

#include "Components/Button.h"

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

	UE_LOG(LogTemp, Warning, TEXT("Hover start"));
}

void UCaptionWidget::OnButtonUnhovered()
{
	UE_LOG(LogTemp, Warning, TEXT("Hover end"));
}
