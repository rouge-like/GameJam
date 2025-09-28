#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FusionMode.h"
#include "HandViewportMapperComponent.generated.h"

class UWidget;

USTRUCT(BlueprintType)
struct FFusionScreenQuad
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Calibration")
	FVector2D TopLeft = FVector2D(0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Calibration")
	FVector2D TopRight = FVector2D(1.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Calibration")
	FVector2D BottomRight = FVector2D(1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Calibration")
	FVector2D BottomLeft = FVector2D(0.f, 1.f);

	void ToArray(TArray<FVector2D>& OutPoints) const
	{
		OutPoints.Reset(4);
		OutPoints.Add(TopLeft);
		OutPoints.Add(TopRight);
		OutPoints.Add(BottomRight);
		OutPoints.Add(BottomLeft);
	}
};

UENUM(BlueprintType)
enum class EFusionScreenQuadCorner : uint8
{
	TopLeft,
	TopRight,
	BottomRight,
	BottomLeft
};

UENUM(BlueprintType)
enum class EFusionState : uint8
{
	SetTopLeft,
	SetTopRight,
	SetBottomRight,
	SetBottomLeft,
	World,
	Description
};
USTRUCT(BlueprintType)
struct FFusionWidgetHitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Fusion|UI")
	UWidget* Widget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Fusion|UI")
	FVector2D ViewportPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Fusion|UI")
	FString WidgetType;

	UPROPERTY(BlueprintReadOnly, Category="Fusion|UI")
	FName WidgetTag = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStateChanged, EFusionState, State);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FUSION_API UHandViewportMapperComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHandViewportMapperComponent();

	UFUNCTION(BlueprintCallable, Category="Fusion|Calibration")
	void SetSourceQuad(const FFusionScreenQuad& InQuad);

	UFUNCTION(BlueprintCallable, Category="Fusion|Calibration")
	void SetTargetQuad(const FFusionScreenQuad& InQuad);

	UFUNCTION(BlueprintCallable, Category="Fusion|Calibration")
	void SetCalibration(const FFusionScreenQuad& InSource, const FFusionScreenQuad& InTarget);

	UFUNCTION(BlueprintCallable, Category="Fusion|Calibration")
	void SetSourceCorner(EFusionScreenQuadCorner Corner, const FVector2D& Position);

	UFUNCTION(BlueprintCallable, Category="Fusion|Calibration")
	void SetTargetCorner(EFusionScreenQuadCorner Corner, const FVector2D& Position);

	UFUNCTION(BlueprintCallable, Category="Fusion|Calibration")
	bool MapPointToViewport(const FVector2D& SourcePoint, FVector2D& OutViewportPoint) const;

	UFUNCTION(BlueprintCallable, Category="Fusion|Mapping")
	bool MapLandmarkToViewport(const FFusionHandSnapshot& Hand, int32 LandmarkId, FVector2D& OutViewportPoint) const;

	UFUNCTION(BlueprintCallable, Category="Fusion|Mapping")
	bool MapDirectionToViewport(const FFusionHandSnapshot& Hand, int32 StartLandmarkId, int32 EndLandmarkId, FVector2D& OutOrigin, FVector2D& OutDirection) const;

	UFUNCTION(BlueprintCallable, Category="Fusion|Mapping")
	bool FindWidgetAlongDirection(const FFusionHandSnapshot& Hand, int32 StartLandmarkId, int32 EndLandmarkId, float MaxDistance, FFusionWidgetHitResult& OutHitResult) const;

	UFUNCTION(BlueprintCallable, Category="Fusion|Calibration")
	void AutoSetTargetQuadFromViewport();

	UPROPERTY(BlueprintAssignable)
	FOnStateChanged OnStateChanged;

protected:
	virtual void BeginPlay() override;

private:
	bool RebuildHomography();
	bool ComputeHomography(const TArray<FVector2D>& SourcePoints, const TArray<FVector2D>& TargetPoints);
	bool SolveLinearSystem8x8(double A[8][8], double B[8], double X[8]) const;
	bool ApplyHomography(const FVector2D& SourcePoint, FVector2D& OutViewportPoint) const;
	bool TryGetLandmarkLocation(const FFusionHandSnapshot& Hand, int32 LandmarkId, FVector& OutWorldLocation) const;
	bool TryExtractHandLandmark(const FFusionHandSnapshot& Hand, int32 LandmarkId, FVector2D& OutViewportPoint) const;
	bool TryExtractUWidget(const TSharedPtr<SWidget>& SlateWidget, UWidget*& OutWidget) const;
	bool HitTestWidgetAt(const FVector2D& ViewportPosition, FFusionWidgetHitResult& OutHitResult) const;
	FVector2D* ResolveCorner(FFusionScreenQuad& Quad, EFusionScreenQuadCorner Corner);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Calibration", meta=(AllowPrivateAccess="true"))
	FFusionScreenQuad SourceQuad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Calibration", meta=(AllowPrivateAccess="true"))
	FFusionScreenQuad TargetQuad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Calibration", meta=(AllowPrivateAccess="true"))
	bool bAutoInitializeTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Mapping", meta=(ClampMin="1.0", AllowPrivateAccess="true"))
	float WidgetSearchStep = 24.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion|Mapping", meta=(ClampMin="1.0", AllowPrivateAccess="true"))
	int32 WidgetSearchSamples = 32;

	double Homography[9];
	bool bHasValidHomography;

	EFusionState State = EFusionState::World;

	UFUNCTION()
	void HandleGestureFrame(const TArray<FFusionHandSnapshot>& Hands);

	UFUNCTION()
	void OnSelect(bool bIsSelecting) const;

	UFUNCTION()
	void OnClick(ACameraManager* CameraRef);
	
	FFusionWidgetHitResult WidgetHit;
	FVector2D FingerLocation;
};
