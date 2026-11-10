// Copyright Moon Party Games. All Rights Reserved.
#include "CTEmoteBubbleWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 말풍선 그림(680x660) 안 동그라미 중심 비율과 그림이 들어갈 크기 (위젯 256x248 기준)
	const FVector2D IconCenter(133.f, 121.f);
	constexpr float IconSize = 160.f;
}

UCTEmoteBubbleWidget::UCTEmoteBubbleWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> BubbleFinder(TEXT("/Game/2D/UI/Emote/UI_Emote_Bubble.UI_Emote_Bubble"));
	if (BubbleFinder.Succeeded()) { BubbleTexture = BubbleFinder.Object; }
}

void UCTEmoteBubbleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget) { return; }

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas_Emote"));
	WidgetTree->RootWidget = Canvas;

	// 말풍선 그림: 위젯 전체를 채움
	Image_Bubble = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_EmoteBubble"));
	Image_Bubble->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UTexture2D* Texture = BubbleTexture.Get())
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Image_Bubble->SetBrush(Brush);
	}
	UCanvasPanelSlot* BubbleSlot = Canvas->AddChildToCanvas(Image_Bubble);
	BubbleSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BubbleSlot->SetOffsets(FMargin(0.f));

	// 이모지 그림: 말풍선 동그라미 가운데
	Image_EmoteIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_EmoteIcon"));
	Image_EmoteIcon->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* IconSlot = Canvas->AddChildToCanvas(Image_EmoteIcon);
	IconSlot->SetAnchors(FAnchors(0.f, 0.f));
	IconSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	IconSlot->SetPosition(IconCenter);
	IconSlot->SetSize(FVector2D(IconSize, IconSize));

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCTEmoteBubbleWidget::SetEmoteIcon(UTexture2D* Icon, FVector2D UVMin, FVector2D UVMax)
{
	if (!Image_EmoteIcon) { return; }

	if (!Icon)
	{
		Image_EmoteIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Icon);
	Brush.ImageSize = FVector2D(IconSize, IconSize);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.SetUVRegion(FBox2f(FVector2f(UVMin), FVector2f(UVMax)));
	Image_EmoteIcon->SetBrush(Brush);
	Image_EmoteIcon->SetColorAndOpacity(FLinearColor::White);
	Image_EmoteIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCTEmoteBubbleWidget::SetBubbleOpacity(float Opacity)
{
	SetRenderOpacity(FMath::Clamp(Opacity, 0.f, 1.f));
}
