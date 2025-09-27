#include "CameraManager.h"
#include "AnimalActor.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

ACameraManager::ACameraManager()
{
	PrimaryActorTick.bCanEverTick = false;

	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	RootComponent = MainCamera;

	CameraTransitionSpeed = 2.0f;
	bIsTransitioning = false;
}

void ACameraManager::BeginPlay()
{
	Super::BeginPlay();
	CurrentActiveCamera = MainCamera;
	SetActiveCamera(MainCamera);
}

void ACameraManager::SwitchToMainCamera()
{
	if (!bIsTransitioning && CurrentActiveCamera != MainCamera)
	{
		SetActiveCamera(MainCamera);
		UE_LOG(LogTemp, Warning, TEXT("Switched to Main Camera"));
	}
}

void ACameraManager::SwitchToAnimalCamera(AAnimalActor* TargetAnimal)
{
	if (!bIsTransitioning && TargetAnimal)
	{
		UCameraComponent* AnimalCamera = TargetAnimal->GetAnimalCamera();
		if (AnimalCamera && CurrentActiveCamera != AnimalCamera)
		{
			SetActiveCamera(AnimalCamera);
			UE_LOG(LogTemp, Warning, TEXT("Switched to Animal Camera"));
		}
	}
}

void ACameraManager::SetActiveCamera(UCameraComponent* NewCamera)
{
	if (NewCamera && GetWorld())
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			bIsTransitioning = true;
			CurrentActiveCamera = NewCamera;

			// 부드러운 전환을 위해 블렌드 시간 설정
			PC->SetViewTargetWithBlend(NewCamera->GetOwner(), CameraTransitionSpeed);

			// 전환 완료 후 플래그 리셋 (타이머 사용)
			FTimerHandle TransitionTimer;
			GetWorld()->GetTimerManager().SetTimer(TransitionTimer, [this]()
			{
				bIsTransitioning = false;
			}, CameraTransitionSpeed + 0.1f, false);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue,
					TEXT("Camera Transition Started"));
			}
		}
	}
}