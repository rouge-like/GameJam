#include "AnimalActor.h"
#include "Engine/Engine.h"

AAnimalActor::AAnimalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	AnimalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnimalMesh"));
	RootComponent = AnimalMesh;

	OutlineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OutlineMesh"));
	OutlineMesh->SetupAttachment(RootComponent);
	OutlineMesh->SetVisibility(false);

	AnimalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("AnimalCamera"));
	AnimalCamera->SetupAttachment(RootComponent);

	CurrentState = EAnimalState::Idle;
}

void AAnimalActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateOutline();
	UpdateAnimationState();
}

void AAnimalActor::SetHoverState(bool bIsHovered)
{
	if (bIsHovered && CurrentState == EAnimalState::Idle)
	{
		CurrentState = EAnimalState::Hover;
		UE_LOG(LogTemp, Log, TEXT("Animal entered hover state"));
	}
	else if (!bIsHovered && CurrentState == EAnimalState::Hover)
	{
		CurrentState = EAnimalState::Idle;
		UE_LOG(LogTemp, Log, TEXT("Animal exited hover state"));
	}

	UpdateOutline();
	UpdateAnimationState();
}

void AAnimalActor::SetClickState(bool bIsClicked)
{
	if (bIsClicked)
	{
		CurrentState = EAnimalState::Clicked;
		UE_LOG(LogTemp, Warning, TEXT("Animal clicked"));
	}
	else
	{
		CurrentState = EAnimalState::Idle;
		UE_LOG(LogTemp, Log, TEXT("Animal unclicked"));
	}

	UpdateOutline();
	UpdateAnimationState();
}

void AAnimalActor::UpdateOutline()
{
	bool bShouldShowOutline = (CurrentState == EAnimalState::Hover || CurrentState == EAnimalState::Clicked);
	OutlineMesh->SetVisibility(bShouldShowOutline);

	if (GEngine && bShouldShowOutline)
	{
		FString StateString;
		switch (CurrentState)
		{
		case EAnimalState::Hover:
			StateString = TEXT("Hover");
			break;
		case EAnimalState::Clicked:
			StateString = TEXT("Clicked");
			break;
		default:
			StateString = TEXT("Idle");
			break;
		}

		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green,
			FString::Printf(TEXT("Current State: %s"), *StateString));
	}
}

void AAnimalActor::UpdateAnimationState()
{
	// 애니메이션 블루프린트에서 CurrentState를 참조하여 애니메이션 전환 처리
	// Blueprint에서 구현할 예정
}