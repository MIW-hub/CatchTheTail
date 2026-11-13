// Copyright Moon Party Games. All Rights Reserved.
#include "CTDirectionArrow.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

// 화살표를 직접 그리는 Slate 위젯
class SCTDirectionArrow : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCTDirectionArrow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) {}

	void SetStyle(const FLinearColor& NewColor, float NewThickness, float NewAngleDegrees, const FSlateBrush* NewBrush)
	{
		Brush = NewBrush;
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
		// 화살표 이미지가 있으면 그 이미지를 돌려서 그림 (위쪽을 향한 그림)
		if (Brush && Brush->GetResourceObject())
		{
			const FVector2D ImageSize = Brush->ImageSize;
			const FVector2D BoxSize = AllottedGeometry.GetLocalSize();
			const double Scale = FMath::Min(BoxSize.X / ImageSize.X, BoxSize.Y / ImageSize.Y) * 0.95;
			const FVector2D DrawSize = ImageSize * Scale;
			const FVector2D TopLeft = (BoxSize - DrawSize) * 0.5;
			const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
			const FLinearColor WidgetTint = InWidgetStyle.GetColorAndOpacityTint();

			// 어두운 배경에서도 보이도록 그림자를 먼저 그림
			FSlateDrawElement::MakeRotatedBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(DrawSize), FSlateLayoutTransform(FVector2f(TopLeft + FVector2D(2.0, 3.0)))),
				Brush, ESlateDrawEffect::None, AngleRadians, TOptional<FVector2f>(FVector2f(DrawSize * 0.5)), FSlateDrawElement::RelativeToElement,
				FLinearColor(0.f, 0.f, 0.f, 0.55f * WidgetTint.A));
			FSlateDrawElement::MakeRotatedBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(FVector2f(DrawSize), FSlateLayoutTransform(FVector2f(TopLeft))),
				Brush, ESlateDrawEffect::None, AngleRadians, TOptional<FVector2f>(FVector2f(DrawSize * 0.5)), FSlateDrawElement::RelativeToElement,
				Color * WidgetTint);
			return LayerId + 1;
		}

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
	const FSlateBrush* Brush = nullptr;
	FLinearColor Color = FLinearColor::White;
	float Thickness = 8.f;
	float AngleDegrees = 0.f;
};

void UCTDirectionArrow::SetAngleDegrees(float NewAngleDegrees)
{
	AngleDegrees = NewAngleDegrees;
	if (MyArrow.IsValid())
	{
		MyArrow->SetStyle(ArrowColor, LineThickness, AngleDegrees, &ArrowBrush);
	}
}

void UCTDirectionArrow::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyArrow.IsValid())
	{
		MyArrow->SetStyle(ArrowColor, LineThickness, AngleDegrees, &ArrowBrush);
	}
}

void UCTDirectionArrow::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyArrow.Reset();
}

TSharedRef<SWidget> UCTDirectionArrow::RebuildWidget()
{
	// 화살표 그림 (위쪽을 향함). 없으면 선으로 그림
	if (!ArrowTexture)
	{
		ArrowTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/ico/ico_Arrow.ico_Arrow"));
	}
	ArrowBrush.SetResourceObject(ArrowTexture);
	ArrowBrush.DrawAs = ESlateBrushDrawType::Image;
	ArrowBrush.ImageSize = ArrowTexture ? FVector2D(ArrowTexture->GetSizeX(), ArrowTexture->GetSizeY()) : FVector2D(58.0, 85.0);

	MyArrow = SNew(SCTDirectionArrow);
	MyArrow->SetStyle(ArrowColor, LineThickness, AngleDegrees, &ArrowBrush);
	return MyArrow.ToSharedRef();
}
