// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptionWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlideOutFinished);

class URecorderComponent;
/**
 * 
 */
UCLASS()
class FUSION_API UCaptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
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
	// Hovering
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
	bool bIsQuestion = false;

	// 지금까지 출력된 텍스트 & 인덱스
	FString CurrentText;
	int32 CurrentIndex;

	FTimerHandle TypingTimerHandle;

	void PrintStart(UTextBlock* _TextBlock, const FString& NewText);
	void TypeNextCharacter(UTextBlock* _TextBlock);
	void InitializeTextBlock(UTextBlock* _TextBlock);

public:
	// UI animation
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* SlideIn;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* SlideOut;

	UPROPERTY(BlueprintAssignable)
	FOnSlideOutFinished OnSlideOutFinished;
	
	UFUNCTION(BlueprintCallable)
	void EnterCaption(const FString& Q, const FString& A);

	UFUNCTION(BlueprintCallable)
	void ExitCaption();

	UFUNCTION()
	void HandleSlideOutFinished();

	UFUNCTION(BlueprintCallable)
	void PlaySlideInAnimation();

protected:
	// 맵에서 동물 클릭하면 동물 이름 받아서
	// {참치}에 대해 알아볼까요? 이렇게 뜨게 해야 함
	//void SetIntroductionText();
};
