#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimalAnimInstance.generated.h"

// 전방 선언 - enum은 AnimalActor.h에서 include됨
class AAnimalActor;
enum class EAnimalState : uint8;

UCLASS()
class FUSION_API UAnimalAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimalAnimInstance();

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	// 현재 동물의 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal State")
	EAnimalState CurrentAnimalState;

	// 이전 상태 (전환 감지용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal State")
	EAnimalState PreviousAnimalState;

	// 현재 상태에 머문 시간
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal State")
	float TimeInCurrentState;

	// 상태별 Boolean 변수 (Blueprint에서 사용하기 편함)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal State")
	bool bIsIdle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal State")
	bool bIsHovered;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animal State")
	bool bIsClicked;

	// 애니메이션 속도 조절
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
	float IdlePlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
	float HoverPlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Settings")
	float ClickedPlayRate;

private:
	// 소유자 동물 액터 참조
	UPROPERTY()
	AAnimalActor* OwnerAnimal;

	// 상태 업데이트 함수
	void UpdateAnimationState();
	void UpdateStateFlags();
	void UpdatePlayRates();

	// 상태 변경 감지
	bool HasStateChanged() const;
	void OnStateChanged();
};