#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "CameraManager.generated.h"

class AAnimalActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSwitchToMainCam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSwitchToAnimalCam);

UCLASS()
class FUSION_API ACameraManager : public AActor
{
	GENERATED_BODY()

public:
	ACameraManager();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* MainCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float AnimalCameraTransitionSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float MainCameraTransitionSpeed;

	UPROPERTY()
	UCameraComponent* CurrentActiveCamera;

	UPROPERTY()
	bool bIsTransitioning;

public:
	UFUNCTION(BlueprintCallable)
	void SwitchToMainCamera();

	UFUNCTION(BlueprintCallable)
	void SwitchToMainCameraSmooth();

	UFUNCTION(BlueprintCallable)
	void SwitchToAnimalCamera(AAnimalActor* TargetAnimal);

private:
	void SetActiveCamera(UCameraComponent* NewCamera, bool bUseBlend = true, float CustomSpeed = -1.0f);

public:
	// delegates
	UPROPERTY(BlueprintAssignable)
	FOnSwitchToMainCam OnSwitchToMainCam;

	UPROPERTY(BlueprintAssignable)
	FOnSwitchToAnimalCam OnSwitchToAnimalCam;

};