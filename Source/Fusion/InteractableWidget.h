// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractableWidget.generated.h"

class AAnimalActor;
/**
 * 
 */
UCLASS()
class FUSION_API UInteractableWidget : public UUserWidget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName AnimalName;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<AAnimalActor> Animal;

	virtual void NativeConstruct() override;
public:
	UFUNCTION()
	void OnSelecting(bool bIsSelecting);
	UFUNCTION()
	AAnimalActor* OnInteract();
};
