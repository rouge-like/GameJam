#include "HandViewportMapperComponent.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWidget.h"
#include "Widgets/SWindow.h"
#include "Slate/SObjectWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "HandViewportMapperComponent.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWidget.h"
#include "Widgets/SWindow.h"
#include "Slate/SObjectWidget.h"
#include "Framework/Application/SlateApplication.h"

DEFINE_LOG_CATEGORY_STATIC(LogHandViewportMapper, Log, All);

UHandViewportMapperComponent::UHandViewportMapperComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bHasValidHomography = false;
	FMemory::Memset(Homography, 0, sizeof(Homography));
	Homography[8] = 1.0;
	WidgetSearchSamples = 32;
	WidgetSearchStep = 24.f;
}

void UHandViewportMapperComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitializeTarget)
	{
		AutoSetTargetQuadFromViewport();
	}

	RebuildHomography();
}

void UHandViewportMapperComponent::SetSourceQuad(const FFusionScreenQuad& InQuad)
{
	SourceQuad = InQuad;
	RebuildHomography();
}

void UHandViewportMapperComponent::SetTargetQuad(const FFusionScreenQuad& InQuad)
{
	TargetQuad = InQuad;
	RebuildHomography();
}

void UHandViewportMapperComponent::SetCalibration(const FFusionScreenQuad& InSource, const FFusionScreenQuad& InTarget)
{
	SourceQuad = InSource;
	TargetQuad = InTarget;
	RebuildHomography();
}

void UHandViewportMapperComponent::SetSourceCorner(EFusionScreenQuadCorner Corner, const FVector2D& Position)
{
	if (FVector2D* CornerPtr = ResolveCorner(SourceQuad, Corner))
	{
		*CornerPtr = Position;
		RebuildHomography();
	}
}

void UHandViewportMapperComponent::SetTargetCorner(EFusionScreenQuadCorner Corner, const FVector2D& Position)
{
	if (FVector2D* CornerPtr = ResolveCorner(TargetQuad, Corner))
	{
		*CornerPtr = Position;
		RebuildHomography();
	}
}

void UHandViewportMapperComponent::AutoSetTargetQuadFromViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		TargetQuad.TopLeft = FVector2D(0.f, 0.f);
		TargetQuad.TopRight = FVector2D(ViewportSize.X, 0.f);
		TargetQuad.BottomRight = FVector2D(ViewportSize.X, ViewportSize.Y);
		TargetQuad.BottomLeft = FVector2D(0.f, ViewportSize.Y);
	}
}

bool UHandViewportMapperComponent::MapPointToViewport(const FVector2D& SourcePoint, FVector2D& OutViewportPoint) const
{
	if (!bHasValidHomography)
	{
		return false;
	}

	return ApplyHomography(SourcePoint, OutViewportPoint);
}

bool UHandViewportMapperComponent::MapLandmarkToViewport(const FFusionHandSnapshot& Hand, int32 LandmarkId, FVector2D& OutViewportPoint) const
{
	return TryExtractHandLandmark(Hand, LandmarkId, OutViewportPoint);
}

bool UHandViewportMapperComponent::MapDirectionToViewport(const FFusionHandSnapshot& Hand, int32 StartLandmarkId, int32 EndLandmarkId, FVector2D& OutOrigin, FVector2D& OutDirection) const
{
	FVector2D StartViewport;
	FVector2D EndViewport;
	if (!TryExtractHandLandmark(Hand, StartLandmarkId, StartViewport) || !TryExtractHandLandmark(Hand, EndLandmarkId, EndViewport))
	{
		return false;
	}

	OutOrigin = StartViewport;
	OutDirection = EndViewport - StartViewport;
	return !OutDirection.IsNearlyZero();
}

bool UHandViewportMapperComponent::FindWidgetAlongDirection(const FFusionHandSnapshot& Hand, int32 StartLandmarkId, int32 EndLandmarkId, float MaxDistance, FFusionWidgetHitResult& OutHitResult) const
{
	OutHitResult = FFusionWidgetHitResult();

	FVector2D Origin;
	FVector2D Direction;
	if (!MapDirectionToViewport(Hand, StartLandmarkId, EndLandmarkId, Origin, Direction))
	{
		return false;
	}

	const FVector2D NormalizedDirection = Direction.GetSafeNormal();
	if (NormalizedDirection.IsNearlyZero())
	{
		return false;
	}

	const float StepLength = FMath::Max(1.f, WidgetSearchStep);
	const int32 StepCount = FMath::Max(1, WidgetSearchSamples > 0 ? WidgetSearchSamples : FMath::CeilToInt(MaxDistance / StepLength));
	for (int32 StepIndex = 1; StepIndex <= StepCount; ++StepIndex)
	{
		const float Distance = StepLength * StepIndex;
		if (Distance > MaxDistance)
		{
			break;
		}

		const FVector2D SamplePoint = Origin + NormalizedDirection * Distance;
		FFusionWidgetHitResult HitResult;
		if (HitTestWidgetAt(SamplePoint, HitResult))
		{
			OutHitResult = HitResult;
			const FString WidgetLabel = OutHitResult.Widget
				? OutHitResult.Widget->GetName()
				: OutHitResult.WidgetTag != NAME_None ? OutHitResult.WidgetTag.ToString() : FString(TEXT("Unknown"));
			UE_LOG(LogHandViewportMapper, Log, TEXT("Widget hit: %s at %s"), *WidgetLabel, *OutHitResult.ViewportPosition.ToString());
			return true;
		}
	}

	return false;
}

bool UHandViewportMapperComponent::RebuildHomography()
{
	TArray<FVector2D> SourcePoints;
	SourceQuad.ToArray(SourcePoints);

	TArray<FVector2D> TargetPoints;
	TargetQuad.ToArray(TargetPoints);

	if (SourcePoints.Num() != 4 || TargetPoints.Num() != 4)
	{
		bHasValidHomography = false;
		return false;
	}

	bHasValidHomography = ComputeHomography(SourcePoints, TargetPoints);
	return bHasValidHomography;
}

bool UHandViewportMapperComponent::ComputeHomography(const TArray<FVector2D>& SourcePoints, const TArray<FVector2D>& TargetPoints)
{
	check(SourcePoints.Num() == 4 && TargetPoints.Num() == 4);

	double A[8][8] = {};
	double B[8] = {};

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const double x = SourcePoints[Index].X;
		const double y = SourcePoints[Index].Y;
		const double X = TargetPoints[Index].X;
		const double Y = TargetPoints[Index].Y;

		const int32 RowX = Index * 2;
		const int32 RowY = RowX + 1;

		A[RowX][0] = x;
		A[RowX][1] = y;
		A[RowX][2] = 1.0;
		A[RowX][3] = 0.0;
		A[RowX][4] = 0.0;
		A[RowX][5] = 0.0;
		A[RowX][6] = -X * x;
		A[RowX][7] = -X * y;
		B[RowX] = X;

		A[RowY][0] = 0.0;
		A[RowY][1] = 0.0;
		A[RowY][2] = 0.0;
		A[RowY][3] = x;
		A[RowY][4] = y;
		A[RowY][5] = 1.0;
		A[RowY][6] = -Y * x;
		A[RowY][7] = -Y * y;
		B[RowY] = Y;
	}

	double X[8];
	if (!SolveLinearSystem8x8(A, B, X))
	{
		return false;
	}

	Homography[0] = X[0];
	Homography[1] = X[1];
	Homography[2] = X[2];
	Homography[3] = X[3];
	Homography[4] = X[4];
	Homography[5] = X[5];
	Homography[6] = X[6];
	Homography[7] = X[7];
	Homography[8] = 1.0;

	return true;
}

bool UHandViewportMapperComponent::SolveLinearSystem8x8(double A[8][8], double B[8], double X[8]) const
{
	const double Epsilon = 1e-8;

	for (int32 Pivot = 0; Pivot < 8; ++Pivot)
	{
		int32 MaxRow = Pivot;
		double MaxValue = FMath::Abs(A[Pivot][Pivot]);
		for (int32 Row = Pivot + 1; Row < 8; ++Row)
		{
			double Value = FMath::Abs(A[Row][Pivot]);
			if (Value > MaxValue)
			{
				MaxValue = Value;
				MaxRow = Row;
			}
		}

		if (MaxValue < Epsilon)
		{
			return false;
		}

		if (MaxRow != Pivot)
		{
			for (int32 Column = Pivot; Column < 8; ++Column)
			{
				Swap(A[Pivot][Column], A[MaxRow][Column]);
			}
			Swap(B[Pivot], B[MaxRow]);
		}

		double PivotValue = A[Pivot][Pivot];
		for (int32 Column = Pivot; Column < 8; ++Column)
		{
			A[Pivot][Column] /= PivotValue;
		}
		B[Pivot] /= PivotValue;

		for (int32 Row = 0; Row < 8; ++Row)
		{
			if (Row == Pivot)
			{
				continue;
			}

			double Factor = A[Row][Pivot];
			if (FMath::IsNearlyZero(Factor, Epsilon))
			{
				continue;
			}

			for (int32 Column = Pivot; Column < 8; ++Column)
			{
				A[Row][Column] -= Factor * A[Pivot][Column];
			}
			B[Row] -= Factor * B[Pivot];
		}
	}

	for (int32 Row = 0; Row < 8; ++Row)
	{
		X[Row] = B[Row];
	}

	return true;
}

bool UHandViewportMapperComponent::ApplyHomography(const FVector2D& SourcePoint, FVector2D& OutViewportPoint) const
{
	const double x = SourcePoint.X;
	const double y = SourcePoint.Y;
	const double Denominator = Homography[6] * x + Homography[7] * y + Homography[8];
	if (FMath::IsNearlyZero(Denominator))
	{
		return false;
	}

	const double X = (Homography[0] * x + Homography[1] * y + Homography[2]) / Denominator;
	const double Y = (Homography[3] * x + Homography[4] * y + Homography[5]) / Denominator;
	OutViewportPoint = FVector2D(static_cast<float>(X), static_cast<float>(Y));
	return true;
}

bool UHandViewportMapperComponent::TryExtractHandLandmark(const FFusionHandSnapshot& Hand, int32 LandmarkId, FVector2D& OutViewportPoint) const
{
	for (const FFusionHandLandmark& Landmark : Hand.Landmarks)
	{
		if (Landmark.Id == LandmarkId)
		{
			return MapPointToViewport(FVector2D(Landmark.Location.X, Landmark.Location.Y), OutViewportPoint);
		}
	}

	return false;
}

bool UHandViewportMapperComponent::TryExtractUWidget(const TSharedPtr<SWidget>& SlateWidget, UWidget*& OutWidget) const
{
	OutWidget = nullptr;
	if (!SlateWidget.IsValid())
	{
		return false;
	}

	if (SlateWidget->GetTypeAsString().Contains(TEXT("SObjectWidget")))
	{
		if (TSharedPtr<SObjectWidget> ObjectWidget = StaticCastSharedPtr<SObjectWidget>(SlateWidget))
		{
			OutWidget = ObjectWidget->GetWidgetObject();
			return OutWidget != nullptr;
		}
	}

	return false;
}

bool UHandViewportMapperComponent::HitTestWidgetAt(const FVector2D& ViewportPosition, FFusionWidgetHitResult& OutHitResult) const
{
	OutHitResult = FFusionWidgetHitResult();

	if (!GetWorld() || !GEngine || !GEngine->GameViewport)
	{
		return false;
	}

	const FGeometry ViewportGeometry = UWidgetLayoutLibrary::GetViewportWidgetGeometry(GetWorld());
	const FVector2D AbsolutePosition = ViewportGeometry.LocalToAbsolute(ViewportPosition);

	const TArray<TSharedRef<SWindow>>& TopLevelWindows = FSlateApplication::Get().GetTopLevelWindows();
	const FWidgetPath WidgetPath = FSlateApplication::Get().LocateWindowUnderMouse(AbsolutePosition, TopLevelWindows, true);

	if (!WidgetPath.IsValid())
	{
		return false;
	}

	for (int32 Index = WidgetPath.Widgets.Num() - 1; Index >= 0; --Index)
	{
		const FArrangedWidget& ArrangedWidget = WidgetPath.Widgets[Index];
		if (TSharedPtr<SWidget> SlateWidget = ArrangedWidget.Widget)
		{
			UWidget* Widget = nullptr;
			OutHitResult.Widget = nullptr;
			OutHitResult.ViewportPosition = ViewportPosition;
			OutHitResult.WidgetType = SlateWidget->GetTypeAsString();
			OutHitResult.WidgetTag = SlateWidget->GetTag();

			if (TryExtractUWidget(SlateWidget, Widget))
			{
				OutHitResult.Widget = Widget;
				return true;
			}

			if (OutHitResult.WidgetTag != NAME_None)
			{
				return true;
			}
		}
	}

	return false;
}

FVector2D* UHandViewportMapperComponent::ResolveCorner(FFusionScreenQuad& Quad, EFusionScreenQuadCorner Corner)
{
	switch (Corner)
	{
	case EFusionScreenQuadCorner::TopLeft:
		return &Quad.TopLeft;
	case EFusionScreenQuadCorner::TopRight:
		return &Quad.TopRight;
	case EFusionScreenQuadCorner::BottomRight:
		return &Quad.BottomRight;
	case EFusionScreenQuadCorner::BottomLeft:
		return &Quad.BottomLeft;
	default:
		return nullptr;
	}
}
