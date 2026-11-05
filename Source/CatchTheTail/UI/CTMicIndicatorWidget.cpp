// Copyright Moon Party Games. All Rights Reserved.
#include "CTMicIndicatorWidget.h"
#include "../Core/CTSettingsSubsystem.h"
#include "../Player/CTCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor Magenta = FLinearColor::FromSRGBColor(FColor(0xFF, 0x27, 0x83));
	const FLinearColor OffPanel(0.62f, 0.62f, 0.62f, 1.f);
	const FLinearColor OffIcon(0.3f, 0.3f, 0.3f, 1.f);
	constexpr float WaveMinHeight = 8.f;
	constexpr float WaveMaxHeight = 34.f;
	constexpr float WaveSpeed = 11.f;

	UImage* MakeImage(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* TexturePath, const FVector2D& Size)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (TexturePath)
		{
			if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath))
			{
				FSlateBrush Brush;
				Brush.SetResourceObject(Texture);
				Brush.ImageSize = Size;
				Brush.DrawAs = ESlateBrushDrawType::Image;
				Image->SetBrush(Brush);
			}
		}
		return Image;
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Child, FVector2D Position, FVector2D Size)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Child);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetSize(Size);
		return CanvasSlot;
	}
}

void UCTMicIndicatorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget) { return; }

	const FVector2D PanelSize(124.f, 65.f);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Layout_Mic"));
	WidgetTree->RootWidget = Column;

	// 패널 124x65 위에 아이콘과 파장 막대
	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_MicGroup"));
	Box->SetWidthOverride(PanelSize.X);
	Box->SetHeightOverride(PanelSize.Y);
	UVerticalBoxSlot* BoxSlot = Column->AddChildToVerticalBox(Box);
	BoxSlot->SetHorizontalAlignment(HAlign_Center);
	BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	UCanvasPanel* Group = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Group_Mic"));
	Box->SetContent(Group);

	// 새 스타일: 흰 상자(평소, 꺼짐)와 분홍 상자(말하는 중). 상태가 바뀔 때 NativeTick에서 바꿈
	PanelWhite = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/Style/UI_Btn_White_L.UI_Btn_White_L"));
	PanelPink = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/Style/UI_Btn_Pink_L.UI_Btn_Pink_L"));
	Image_MicPanel = MakeImage(WidgetTree, TEXT("Image_MicPanel"), nullptr, PanelSize);
	Place(Group, Image_MicPanel, FVector2D::ZeroVector, PanelSize);
	Image_MicIcon = MakeImage(WidgetTree, TEXT("Image_MicIcon"), TEXT("/Game/2D/UI/ico/Ico_Mic.Ico_Mic"), FVector2D(40.f, 40.f));
	Place(Group, Image_MicIcon, FVector2D(14.f, 6.f), FVector2D(40.f, 40.f));

	UHorizontalBox* Waves = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row_MicWave"));
	Waves->SetVisibility(ESlateVisibility::HitTestInvisible);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UImage* Bar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Image_MicWave%d"), Index + 1));
		Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		Bar->SetColorAndOpacity(Magenta);
		USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Size_MicWave%d"), Index + 1));
		BarBox->SetWidthOverride(8.f);
		BarBox->SetHeightOverride(Index == 0 ? 14.f : (Index == 1 ? 24.f : 34.f));
		BarBox->SetContent(Bar);
		UHorizontalBoxSlot* BarSlot = Waves->AddChildToHorizontalBox(BarBox);
		BarSlot->SetPadding(FMargin(0.f, 0.f, Index < 2 ? 7.f : 0.f, 0.f));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		WaveBoxes[Index] = BarBox;
		WaveImages[Index] = Bar;
	}
	Place(Group, Waves, FVector2D(66.f, 4.f), FVector2D(40.f, 42.f));

	// 아래 키 글자
	Text_MicKey = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_MicKey"));
	Text_MicKey->SetText(FText::FromString(TEXT("V")));
	Text_MicKey->SetJustification(ETextJustify::Center);
	Text_MicKey->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text_MicKey->SetShadowOffset(FVector2D(1.5f, 1.5f));
	Text_MicKey->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	if (UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Font/NotoSansKR-Medium_Font.NotoSansKR-Medium_Font")))
	{
		Text_MicKey->SetFont(FSlateFontInfo(Font, 18, FName(TEXT("Default"))));
	}
	Column->AddChildToVerticalBox(Text_MicKey)->SetHorizontalAlignment(HAlign_Center);

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCTMicIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
	const bool bMicEnabled = !Settings || Settings->IsMicEnabled();

	const APlayerController* PlayerController = GetOwningPlayer();
	const ACTCharacter* MyCharacter = PlayerController ? Cast<ACTCharacter>(PlayerController->GetPawn()) : nullptr;
	const bool bTalking = bMicEnabled && MyCharacter && MyCharacter->IsVoiceTransmitting();

	const FLinearColor PanelColor = !bMicEnabled ? OffPanel : (bTalking ? Magenta : FLinearColor::White);
	const FLinearColor ForeColor = !bMicEnabled ? OffIcon : (bTalking ? FLinearColor::White : Magenta);
	// 패널 이미지는 상태가 바뀔 때만 교체 (0 꺼짐, 1 평소, 2 말하는 중)
	const int32 NewPanelState = !bMicEnabled ? 0 : (bTalking ? 2 : 1);
	if (Image_MicPanel && NewPanelState != PanelState)
	{
		PanelState = NewPanelState;
		UTexture2D* PanelTexture = (NewPanelState == 2 && PanelPink) ? PanelPink.Get() : PanelWhite.Get();
		if (PanelTexture)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(PanelTexture);
			Brush.ImageSize = FVector2D(PanelTexture->GetSizeX(), PanelTexture->GetSizeY());
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.Margin = FMargin(0.1f, 0.3f, 0.1f, 0.35f);
			Image_MicPanel->SetBrush(Brush);
		}
		// 꺼짐은 흰 상자를 회색으로 눌러 표시
		Image_MicPanel->SetColorAndOpacity(NewPanelState == 0 ? OffPanel : FLinearColor::White);
	}
	if (Image_MicIcon) { Image_MicIcon->SetColorAndOpacity(ForeColor); }

	WaveTime += InDeltaTime * WaveSpeed;
	WaveBlend = FMath::FInterpTo(WaveBlend, bTalking ? 1.f : 0.f, InDeltaTime, 14.f);

	static const float RestRatios[3] = { 0.35f, 0.65f, 1.f };
	static const float Phases[3] = { 0.f, 2.1f, 4.2f };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		float Ratio = RestRatios[Index];
		if (!bMicEnabled)
		{
			Ratio = 0.2f;
		}
		else if (WaveBlend > KINDA_SMALL_NUMBER)
		{
			const float Wave = 0.5f + 0.32f * FMath::Sin(WaveTime + Phases[Index]) + 0.18f * FMath::Sin(WaveTime * 1.9f + Phases[Index] * 1.7f);
			Ratio = FMath::Lerp(Ratio, FMath::Clamp(Wave, 0.12f, 1.f), WaveBlend);
		}
		if (WaveBoxes[Index]) { WaveBoxes[Index]->SetHeightOverride(FMath::Lerp(WaveMinHeight, WaveMaxHeight, Ratio)); }
		if (WaveImages[Index]) { WaveImages[Index]->SetColorAndOpacity(ForeColor); }
	}

	if (Text_MicKey && Settings)
	{
		const FString KeyName = Settings->IsMicAlwaysOn() ? FString() : Settings->GetActionKey(TEXT("Voice")).GetDisplayName().ToString();
		if (KeyName != LastKeyName)
		{
			LastKeyName = KeyName;
			Text_MicKey->SetText(FText::FromString(KeyName));
		}
	}
}
