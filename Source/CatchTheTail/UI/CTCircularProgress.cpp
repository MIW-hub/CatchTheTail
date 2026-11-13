// Copyright Moon Party Games. All Rights Reserved.
#include "CTCircularProgress.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

class SCTCircularProgress : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCTCircularProgress) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) {}

	void SetValues(float InPercent, const FLinearColor& InFill, const FLinearColor& InTrack, float InThickness)
	{
		Percent = FMath::Clamp(InPercent, 0.f, 1.f);
		Fill = InFill;
		Track = InTrack;
		Thickness = InThickness;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(64.f, 64.f); }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const FVector2D Center = Size * 0.5f;
		const float Radius = FMath::Max(1.f, FMath::Min(Size.X, Size.Y) * 0.5f - Thickness * 0.5f);

		auto DrawArc = [&](float FractionStart, float FractionEnd, const FLinearColor& Color, int32 Layer)
		{
			const int32 Segments = FMath::Max(2, FMath::CeilToInt((FractionEnd - FractionStart) * 64.f));
			TArray<FVector2f> Points;
			Points.Reserve(Segments + 1);
			for (int32 Index = 0; Index <= Segments; ++Index)
			{
				const float Fraction = FMath::Lerp(FractionStart, FractionEnd, static_cast<float>(Index) / Segments);
				const float Angle = Fraction * 2.f * PI - HALF_PI; // 위에서 시계 방향
				Points.Add(FVector2f(Center.X + Radius * FMath::Cos(Angle), Center.Y + Radius * FMath::Sin(Angle)));
			}
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color * InWidgetStyle.GetColorAndOpacityTint(), true, Thickness);
		};

		DrawArc(0.f, 1.f, Track, LayerId);
		if (Percent > 0.001f) { DrawArc(0.f, Percent, Fill, LayerId + 1); }
		return LayerId + 1;
	}

private:
	float Percent = 1.f;
	FLinearColor Fill = FLinearColor::White;
	FLinearColor Track = FLinearColor::Gray;
	float Thickness = 8.f;
};

UCTCircularProgress::UCTCircularProgress()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UCTCircularProgress::RebuildWidget()
{
	SlateProgress = SNew(SCTCircularProgress);
	return SlateProgress.ToSharedRef();
}

void UCTCircularProgress::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SlateProgress.IsValid()) { SlateProgress->SetValues(Percent, FillColor, TrackColor, Thickness); }
}

void UCTCircularProgress::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateProgress.Reset();
}

void UCTCircularProgress::SetPercent(float NewPercent)
{
	Percent = FMath::Clamp(NewPercent, 0.f, 1.f);
	if (SlateProgress.IsValid()) { SlateProgress->SetValues(Percent, FillColor, TrackColor, Thickness); }
}
