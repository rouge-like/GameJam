#include "FusionPlayerController.h"
#include "AnimalActor.h"
#include "CameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"

AFusionPlayerController::AFusionPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AFusionPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CameraManagerRef = GetWorld()->SpawnActor<ACameraManager>();
	CurrentHoveredAnimal = nullptr;
	CurrentSelectedAnimal = nullptr;
}

void AFusionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("MouseClick", IE_Pressed, this, &AFusionPlayerController::OnMouseClick);
	InputComponent->BindAction("Cancel", IE_Pressed, this, &AFusionPlayerController::OnCancelKey);
}

void AFusionPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	HandleMouseMovement();
}

void AFusionPlayerController::HandleMouseMovement()
{
	AAnimalActor* HoveredAnimal = GetAnimalUnderCursor();

	if (HoveredAnimal != CurrentHoveredAnimal)
	{
		if (CurrentHoveredAnimal && CurrentHoveredAnimal != CurrentSelectedAnimal)
		{
			CurrentHoveredAnimal->SetHoverState(false);
		}

		CurrentHoveredAnimal = HoveredAnimal;

		if (CurrentHoveredAnimal && CurrentHoveredAnimal != CurrentSelectedAnimal)
		{
			CurrentHoveredAnimal->SetHoverState(true);
		}
	}
}

void AFusionPlayerController::OnMouseClick()
{
	if (CurrentHoveredAnimal)
	{
		if (CurrentSelectedAnimal)
		{
			CurrentSelectedAnimal->SetClickState(false);
		}

		CurrentSelectedAnimal = CurrentHoveredAnimal;
		CurrentSelectedAnimal->SetClickState(true);

		if (CameraManagerRef)
		{
			CameraManagerRef->SwitchToAnimalCamera(CurrentSelectedAnimal);
		}

		UE_LOG(LogTemp, Warning, TEXT("Animal selected and camera switched"));
	}
}

void AFusionPlayerController::OnCancelKey()
{
	if (CurrentSelectedAnimal)
	{
		CurrentSelectedAnimal->SetClickState(false);
		CurrentSelectedAnimal = nullptr;

		if (CameraManagerRef)
		{
			CameraManagerRef->SwitchToMainCamera();
		}

		UE_LOG(LogTemp, Warning, TEXT("Selection cancelled, returned to main camera"));
	}
}

AAnimalActor* AFusionPlayerController::GetAnimalUnderCursor()
{
	FHitResult HitResult;
	bool bHit = GetHitResultUnderCursor(ECollisionChannel::ECC_Pawn, false, HitResult);

	if (bHit && HitResult.GetActor())
	{
		return Cast<AAnimalActor>(HitResult.GetActor());
	}

	return nullptr;
}