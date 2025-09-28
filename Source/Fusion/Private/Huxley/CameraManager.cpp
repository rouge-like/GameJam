#include "Huxley/CameraManager.h"
#include "Huxley/AnimalActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ACameraManager::ACameraManager()
{
	PrimaryActorTick.bCanEverTick = false;

	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	RootComponent = MainCamera;

	AnimalCameraTransitionSpeed = 2.0f;
	MainCameraTransitionSpeed = 1.5f;
	bIsTransitioning = false;
}

void ACameraManager::BeginPlay()
{
	Super::BeginPlay();
	CurrentActiveCamera = MainCamera;
	SetActiveCamera(MainCamera, false); // 게임 시작 시 즉시 전환
}

void ACameraManager::SwitchToMainCamera()
{
	if (MainCamera)
	{
		SetActiveCamera(MainCamera, false); // 즉시 전환
		UE_LOG(LogTemp, Warning, TEXT("Switched to Main Camera (Immediate)"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MainCamera is null!"));
	}
}

void ACameraManager::SwitchToMainCameraSmooth()
{
	if (MainCamera)
	{
		SetActiveCamera(MainCamera, true, MainCameraTransitionSpeed); // 부드러운 전환
		UE_LOG(LogTemp, Warning, TEXT("Switched to Main Camera (Smooth)"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MainCamera is null!"));
	}
}

void ACameraManager::SwitchToAnimalCamera(AAnimalActor* TargetAnimal)
{
	if (!bIsTransitioning && TargetAnimal)
	{
		UCameraComponent* AnimalCamera = TargetAnimal->GetAnimalCamera();
		if (AnimalCamera && CurrentActiveCamera != AnimalCamera)
		{
			SetActiveCamera(AnimalCamera, true, AnimalCameraTransitionSpeed);
			UE_LOG(LogTemp, Warning, TEXT("Switched to Animal Camera"));
		}
	}
}

void ACameraManager::SetActiveCamera(UCameraComponent* NewCamera, bool bUseBlend, float CustomSpeed)
{
	if (NewCamera && GetWorld())
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			CurrentActiveCamera = NewCamera;

			// 전환 속도 결정
			float TransitionSpeed = CustomSpeed;
			if (TransitionSpeed < 0.0f)
			{
				TransitionSpeed = (NewCamera == MainCamera) ? MainCameraTransitionSpeed : AnimalCameraTransitionSpeed;
			}

			if (bUseBlend)
			{
				// 부드러운 전환
				bIsTransitioning = true;
				PC->SetViewTargetWithBlend(NewCamera->GetOwner(), TransitionSpeed);

				// 전환 완료 후 플래그 리셋
				FTimerHandle TransitionTimer;
				GetWorld()->GetTimerManager().SetTimer(TransitionTimer, [this]()
				{
					bIsTransitioning = false;
				}, TransitionSpeed + 0.1f, false);

				UE_LOG(LogTemp, Warning, TEXT("Smooth transition with speed: %.2f"), TransitionSpeed);
			}
			else
			{
				// 즉시 전환
				PC->SetViewTarget(NewCamera->GetOwner());
				bIsTransitioning = false;
				UE_LOG(LogTemp, Warning, TEXT("Immediate camera switch"));
			}
			
		}
	}
}