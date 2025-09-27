#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "CameraManager.generated.h"

class AAnimalActor;

UCLASS()
class GAMEJAM_API ACameraManager : public AActor
{
	GENERATED_BODY()

public:
	ACameraManager();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* MainCamera;

	UPROPERTY(BlueprintReadWrite, Category = "Camera")
	float CameraTransitionSpeed;

	UPROPERTY()
	UCameraComponent* CurrentActiveCamera;

	UPROPERTY()
	bool bIsTransitioning;

public:
	UFUNCTION(BlueprintCallable)
	void SwitchToMainCamera();

	UFUNCTION(BlueprintCallable)
	void SwitchToAnimalCamera(AAnimalActor* TargetAnimal);

private:
	void SetActiveCamera(UCameraComponent* NewCamera);
};