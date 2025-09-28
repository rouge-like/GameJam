#include "Huxley/AnimalAnimInstance.h"
#include "Huxley/AnimalActor.h"
#include "Engine/Engine.h"
#include "Components/SkeletalMeshComponent.h"

UAnimalAnimInstance::UAnimalAnimInstance()
{
    CurrentAnimalState = EAnimalState::Idle;
    PreviousAnimalState = EAnimalState::Idle;
    TimeInCurrentState = 0.0f;

    bIsIdle = true;
    bIsHovered = false;
    bIsClicked = false;

    // 기본 플레이 속도 설정
    IdlePlayRate = 1.0f;
    HoverPlayRate = 1.1f;
    ClickedPlayRate = 1.2f;

    OwnerAnimal = nullptr;
}

void UAnimalAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    // 소유자 동물 액터 참조 설정
    OwnerAnimal = Cast<AAnimalActor>(TryGetPawnOwner());

    if (OwnerAnimal)
    {
        CurrentAnimalState = OwnerAnimal->GetCurrentState();
        PreviousAnimalState = CurrentAnimalState;
        UpdateStateFlags();

        UE_LOG(LogTemp, Log, TEXT("AnimalAnimInstance initialized for: %s"), *OwnerAnimal->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AnimalAnimInstance: Failed to get AnimalActor owner"));
    }
}

void UAnimalAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);

    if (!OwnerAnimal)
    {
        OwnerAnimal = Cast<AAnimalActor>(TryGetPawnOwner());
        if (!OwnerAnimal)
        {
            return;
        }
    }

    // 상태 업데이트
    UpdateAnimationState();

    // 시간 추적
    TimeInCurrentState += DeltaTime;

    // 상태 변경 감지
    if (HasStateChanged())
    {
        OnStateChanged();
    }

    // 디버깅 정보는 패키징을 위해 제거됨
}

void UAnimalAnimInstance::UpdateAnimationState()
{
    if (OwnerAnimal)
    {
        // AnimalActor의 현재 상태를 가져옴
        EAnimalState NewState = OwnerAnimal->GetCurrentState();

        // 상태가 변경되었다면 업데이트
        if (NewState != CurrentAnimalState)
        {
            PreviousAnimalState = CurrentAnimalState;
            CurrentAnimalState = NewState;
            TimeInCurrentState = 0.0f;

            UpdateStateFlags();
            UpdatePlayRates();

            UE_LOG(LogTemp, Log, TEXT("Animation state changed: %d -> %d"),
                (int32)PreviousAnimalState, (int32)CurrentAnimalState);
        }
    }
}

void UAnimalAnimInstance::UpdateStateFlags()
{
    // Boolean 플래그 업데이트 (Blueprint State Machine에서 사용)
    bIsIdle = (CurrentAnimalState == EAnimalState::Idle);
    bIsHovered = (CurrentAnimalState == EAnimalState::Hover);
    bIsClicked = (CurrentAnimalState == EAnimalState::Clicked);
}

void UAnimalAnimInstance::UpdatePlayRates()
{
    // 현재 상태에 따른 플레이 속도 설정은 Blueprint에서 처리
    // 여기서는 로깅만 수행
    float CurrentPlayRate = 1.0f;
    switch (CurrentAnimalState)
    {
    case EAnimalState::Idle:
        CurrentPlayRate = IdlePlayRate;
        break;
    case EAnimalState::Hover:
        CurrentPlayRate = HoverPlayRate;
        break;
    case EAnimalState::Clicked:
        CurrentPlayRate = ClickedPlayRate;
        break;
    }

    UE_LOG(LogTemp, Verbose, TEXT("Play rate for state %d: %.2f"),
        (int32)CurrentAnimalState, CurrentPlayRate);
}

bool UAnimalAnimInstance::HasStateChanged() const
{
    return CurrentAnimalState != PreviousAnimalState;
}

void UAnimalAnimInstance::OnStateChanged()
{
    UpdateStateFlags();
    UpdatePlayRates();

    // 상태 변경 시 추가 로직 (필요시)
    switch (CurrentAnimalState)
    {
    case EAnimalState::Idle:
        UE_LOG(LogTemp, Log, TEXT("Entered Idle state"));
        break;
    case EAnimalState::Hover:
        UE_LOG(LogTemp, Log, TEXT("Entered Hover state"));
        break;
    case EAnimalState::Clicked:
        UE_LOG(LogTemp, Log, TEXT("Entered Clicked state"));
        break;
    }

}