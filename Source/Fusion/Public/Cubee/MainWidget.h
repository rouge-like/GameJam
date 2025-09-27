// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTextHideFinished);

/**
 * 
 */
UCLASS()
class FUSION_API UMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

public:
	// UI
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Txt_Gesture;

public:
	// Gesture
	void UpdateGestureState(const FString& NewState);
	
public:
	// UI animation
	UPROPERTY(BlueprintAssignable)
	FOnTextHideFinished OnTextHideFinished;
	
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* SlideIn;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* SlideOut;
	
public:
	UFUNCTION(BlueprintCallable)
	void ViewGuideText();

	UFUNCTION(BlueprintCallable)
	void HideGuideText();

protected:
	UFUNCTION()
	void HandleSlideOutFinished();
};
