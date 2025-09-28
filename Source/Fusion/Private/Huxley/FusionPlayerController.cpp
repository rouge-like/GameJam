#include "Huxley/FusionPlayerController.h"
#include "Huxley/AnimalActor.h"
#include "Huxley/CameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

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

	// Enhanced Input 설정
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// CameraManager를 레벨에서 찾거나 생성
	CameraManagerRef = Cast<ACameraManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ACameraManager::StaticClass()));
	if (!CameraManagerRef)
	{
		CameraManagerRef = GetWorld()->SpawnActor<ACameraManager>();
	}

	CurrentHoveredAnimal = nullptr;
	CurrentSelectedAnimal = nullptr;

	// 게임 시작 시 즉시 MainCamera로 설정
	if (CameraManagerRef)
	{
		CameraManagerRef->SwitchToMainCamera();
		UE_LOG(LogTemp, Warning, TEXT("Game started - Immediately set to Main Camera"));
	}
}

void AFusionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input 바인딩
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ClickAction)
		{
			EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Triggered, this, &AFusionPlayerController::OnMouseClick);
		}

		if (CancelAction)
		{
			EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Triggered, this, &AFusionPlayerController::OnCancelKey);
		}
	}
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

void AFusionPlayerController::OnMouseClick(const FInputActionValue& Value)
{
	OnMouseClicked.Broadcast(CameraManagerRef);
	// 마우스 위치에서 직접 감지
	AAnimalActor* ClickedAnimal = GetAnimalUnderCursor();
	
	if (ClickedAnimal)
	{
		// 이전에 선택된 동물이 있다면 해제
		if (CurrentSelectedAnimal && CurrentSelectedAnimal != ClickedAnimal)
		{
			CurrentSelectedAnimal->SetClickState(false);
		}
	
		CurrentSelectedAnimal = ClickedAnimal;
		CurrentSelectedAnimal->SetClickState(true);
	
		if (CameraManagerRef)
		{
			CameraManagerRef->SwitchToAnimalCamera(CurrentSelectedAnimal);
		}
	
		UE_LOG(LogTemp, Warning, TEXT("Animal clicked and camera switched: %s"),
			*CurrentSelectedAnimal->GetName());
		
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No animal found under cursor"));
		
	}
}

void AFusionPlayerController::OnCancelKey(const FInputActionValue& Value)
{
	if (CurrentSelectedAnimal)
	{
		CurrentSelectedAnimal->SetClickState(false);
		CurrentSelectedAnimal = nullptr;

		if (CameraManagerRef)
		{
			CameraManagerRef->SwitchToMainCameraSmooth(); // 부드러운 전환 사용
		}

		UE_LOG(LogTemp, Warning, TEXT("Selection cancelled, smoothly returning to main camera"));
		
	}
}

AAnimalActor* AFusionPlayerController::GetAnimalUnderCursor()
{
	FHitResult HitResult;

	// 여러 채널에서 감지 시도
	bool bHit = GetHitResultUnderCursor(ECollisionChannel::ECC_Pawn, false, HitResult);

	if (!bHit)
	{
		// Pawn 채널에서 감지되지 않으면 WorldStatic으로도 시도
		bHit = GetHitResultUnderCursor(ECollisionChannel::ECC_WorldStatic, false, HitResult);
	}

	if (!bHit)
	{
		// WorldDynamic으로도 시도
		bHit = GetHitResultUnderCursor(ECollisionChannel::ECC_WorldDynamic, false, HitResult);
	}

	if (bHit && HitResult.GetActor())
	{
		UE_LOG(LogTemp, Log, TEXT("Hit actor: %s"), *HitResult.GetActor()->GetName());

		AAnimalActor* Animal = Cast<AAnimalActor>(HitResult.GetActor());
		if (Animal)
		{
			UE_LOG(LogTemp, Log, TEXT("Found AnimalActor: %s"), *Animal->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Actor is not AnimalActor"));
		}

		return Animal;
	}

	UE_LOG(LogTemp, Log, TEXT("No hit result"));
	return nullptr;
}

void AFusionPlayerController::OnSelectAction()
{
	OnMouseClicked.Broadcast(CameraManagerRef);
}

void AFusionPlayerController::OnStopAction()
{
	if (CameraManagerRef)
	{
		CameraManagerRef->SwitchToMainCameraSmooth(); // 부드러운 전환 사용
	}
}
