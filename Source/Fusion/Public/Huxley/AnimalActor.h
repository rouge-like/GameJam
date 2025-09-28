#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "AnimalActor.generated.h"

// EAnimalState enum 정의
UENUM(BlueprintType)
enum class EAnimalState : uint8
{
	Idle,
	Hover,
	Clicked
};

UCLASS()
class FUSION_API AAnimalActor : public APawn
{
	GENERATED_BODY()

public:
	AAnimalActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* AnimalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* OutlineMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* AnimalCamera;

	UPROPERTY(BlueprintReadWrite, Category = "State")
	EAnimalState CurrentState;

public:
	UFUNCTION(BlueprintCallable)
	void SetHoverState(bool bIsHovered);

	UFUNCTION(BlueprintCallable)
	void SetClickState(bool bIsClicked);

	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetAnimalCamera() const { return AnimalCamera; }

	UFUNCTION(BlueprintCallable)
	EAnimalState GetCurrentState() const { return CurrentState; }

private:
	void UpdateOutline();
	void UpdateAnimationState();

	// AnimInstance 헬퍼 함수들
	class UAnimalAnimInstance* GetAnimalAnimInstance() const;
};