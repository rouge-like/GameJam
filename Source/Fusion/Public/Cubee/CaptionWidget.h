// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptionWidget.generated.h"

class URecorderComponent;
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
	
public:
	// Print
	// 텍스트 전문
	UPROPERTY(EditAnywhere, Category = Text)
	FString FullText;
	
protected:
	// 지금까지 출력된 텍스트
	bool bIsQuestion = false;
	
	FString CurrentText;
	int32 CurrentIndex;

	FTimerHandle TypingTimerHandle;

	void PrintStart(const FString& NewText);
	void TypeNextCharacter(UTextBlock* _TextBlock);
	void InitializeTextBlock(UTextBlock* _TextBlock);

public:
	// UI animation
	UFUNCTION(BlueprintCallable)
	void EnterCaption();

	UFUNCTION(BlueprintCallable)
	void ExitCaption();
};
