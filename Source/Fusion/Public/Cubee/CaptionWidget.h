// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptionWidget.generated.h"

/**
 * 
 */
UCLASS()
class FUSION_API UCaptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	
public:
	// UI
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_Record;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_Q;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_A;

protected:
	// Button hover
	bool bIsHovering = false;
	
	UFUNCTION()
	void OnButtonHovered();

	UFUNCTION()
	void OnButtonUnhovered();

public:
	// Recorder
	UPROPERTY()
	URecorderComponent* RecordingComponent;
	
	void OnRecordStart();
	void OnRecordEnd();
	
	UFUNCTION(BlueprintCallable)
	void SetRecordingComponent(URecorderComponent* InComponent);
	
};
