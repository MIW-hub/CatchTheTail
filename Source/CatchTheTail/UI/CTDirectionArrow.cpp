// Copyright Moon Party Games. All Rights Reserved.
#include "CTDirectionArrow.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

// 화살표를 직접 그리는 Slate 위젯
class SCTDirectionArrow : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCTDirectionArrow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) {}

	void SetStyle(const FLinearColor& NewColor, float NewThickness, float NewAngleDegrees)
	{
		Color = NewColor;
		Thickness = NewThickness;
		AngleDegrees = NewAngleDegrees;
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
	{
		return FVector2D(120.0, 120.0);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const FVector2D Center = Size * 0.5;
		const double Radius = FMath::Min(Size.X, Size.Y) * 0.5;

		// 위쪽(-Y)을 향한 화살표 모양을 중심 기준으로 만든 뒤 각도만큼 돌림 (화면은 아래로 갈수록 Y가 커서 시계 방향)
		const double Radians = FMath::DegreesToRadians(AngleDegrees);
		const double Cos = FMath::Cos(Radians);
		const double Sin = FMath::Sin(Radians);
		auto Place = [&](double X, double Y)
		{
			return Center + FVector2D(X * Cos - Y * Sin, X * Sin + Y * Cos);
		};

		const FVector2D Tip = Place(0.0, -Radius * 0.9);
		const FVector2D Tail = Place(0.0, Radius * 0.75);
		const FVector2D LeftWing = Place(-Radius * 0.5, -Radius * 0.25);
		const FVector2D RightWing = Place(Radius * 0.5, -Radius * 0.25);

		const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();

		// 어두운 배경에서도 보이도록 그림자를 먼저 그림
		const FLinearColor ShadowColor(0.f, 0.f, 0.f, 0.6f * Tint.A);
		const FVector2D ShadowOffset(2.0, 2.0);
		const TArray<FVector2D> ShadowShaft = { Tail + ShadowOffset, Tip + ShadowOffset };
		const TArray<FVector2D> ShadowHead = { LeftWing + ShadowOffset, Tip + ShadowOffset, RightWing + ShadowOffset };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), ShadowShaft, ESlateDrawEffect::None, ShadowColor, true, Thickness + 4.f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), ShadowHead, ESlateDrawEffect::None, ShadowColor, true, Thickness + 4.f);

		const FLinearColor LineColor = Color * Tint;
		const TArray<FVector2D> Shaft = { Tail, Tip };
		const TArray<FVector2D> Head = { LeftWing, Tip, RightWing };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Shaft, ESlateDrawEffect::None, LineColor, true, Thickness);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Head, ESlateDrawEffect::None, LineColor, true, Thickness);

		return LayerId + 1;
	}

private:
	FLinearColor Color = FLinearColor::White;
	float Thickness = 8.f;
	float AngleDegrees = 0.f;
};

void UCTDirectionArrow::SetAngleDegrees(float NewAngleDegrees)
{
	AngleDegrees = NewAngleDegrees;
	if (MyArrow.IsValid())
	{
		MyArrow->SetStyle(ArrowColor, LineThickness, AngleDegrees);
	}
}

void UCTDirectionArrow::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyArrow.IsValid())
	{
		MyArrow->SetStyle(ArrowColor, LineThickness, AngleDegrees);
	}
}

void UCTDirectionArrow::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyArrow.Reset();
}

TSharedRef<SWidget> UCTDirectionArrow::RebuildWidget()
{
	MyArrow = SNew(SCTDirectionArrow);
	MyArrow->SetStyle(ArrowColor, LineThickness, AngleDegrees);
	return MyArrow.ToSharedRef();
}
