#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "FusionPlayerController.generated.h"

class AAnimalActor;
class ACameraManager;
class UInputMappingContext;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMouseClicked, ACameraManager*, CameraRef);

UCLASS()
class FUSION_API AFusionPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFusionPlayerController();

	UPROPERTY()
	FOnMouseClicked OnMouseClicked;
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	// Enhanced Input 관련
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> ClickAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> CancelAction;

private:
	UPROPERTY()
	ACameraManager* CameraManagerRef;

	UPROPERTY()
	AAnimalActor* CurrentHoveredAnimal;

	UPROPERTY()
	AAnimalActor* CurrentSelectedAnimal;

	void HandleMouseMovement();
	void OnMouseClick(const FInputActionValue& Value);
	void OnCancelKey(const FInputActionValue& Value);

	AAnimalActor* GetAnimalUnderCursor();

public:
	void OnSelectAction();
	void OnStopAction();
};