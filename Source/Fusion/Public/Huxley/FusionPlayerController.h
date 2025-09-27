#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FusionPlayerController.generated.h"

class AAnimalActor;
class ACameraManager;

UCLASS()
class GAMEJAM_API AFusionPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFusionPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	ACameraManager* CameraManagerRef;

	UPROPERTY()
	AAnimalActor* CurrentHoveredAnimal;

	UPROPERTY()
	AAnimalActor* CurrentSelectedAnimal;

	void HandleMouseMovement();
	void OnMouseClick();
	void OnCancelKey();

	AAnimalActor* GetAnimalUnderCursor();
};