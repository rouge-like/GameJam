#include "Huxley/AnimalActor.h"
#include "Huxley/AnimalAnimInstance.h"
#include "Engine/Engine.h"

AAnimalActor::AAnimalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	AnimalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnimalMesh"));
	RootComponent = AnimalMesh;

	OutlineMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("OutlineMesh"));
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
	
}

void AAnimalActor::UpdateAnimationState()
{
	// AnimInstance가 매 프레임 CurrentState를 확인하여 자동으로 처리
	// C++ AnimInstance에서 상태 동기화가 이루어짐

	if (AnimalMesh && AnimalMesh->GetAnimInstance())
	{
		UE_LOG(LogTemp, Verbose, TEXT("Animation state updated: %d"), (int32)CurrentState);
	}
}

UAnimalAnimInstance* AAnimalActor::GetAnimalAnimInstance() const
{
	if (AnimalMesh)
	{
		return Cast<UAnimalAnimInstance>(AnimalMesh->GetAnimInstance());
	}
	return nullptr;
}