#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "AnimalActor.generated.h"

UENUM(BlueprintType)
enum class EAnimalState : uint8
{
	Idle,
	Hover,
	Clicked
};

UCLASS()
class GAMEJAM_API AAnimalActor : public APawn
{
	GENERATED_BODY()

public:
	AAnimalActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* AnimalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* OutlineMesh;

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

private:
	void UpdateOutline();
	void UpdateAnimationState();
};