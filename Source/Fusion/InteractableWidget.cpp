// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableWidget.h"
#include "Huxley/AnimalActor.h"
#include "Huxley/CameraManager.h"
#include "Huxley/FusionPlayerController.h"
#include "Kismet/GameplayStatics.h"

void UInteractableWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	TArray<AActor*> MyActors;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), AAnimalActor::StaticClass(), AnimalName, MyActors);

	if (MyActors.Num() > 0)
		Animal = Cast<AAnimalActor>(MyActors[0]);
}

void UInteractableWidget::OnSelecting(bool bIsSelecting)
{
	if (Animal)
	{
		Animal->SetHoverState(bIsSelecting);
	}
}

AAnimalActor* UInteractableWidget::OnInteract()
{
	if (Animal)
	{
		Animal->SetClickState(true);
	}

	return Animal;
}
