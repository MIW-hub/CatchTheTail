// Copyright Moon Party Games. All Rights Reserved.
#include "CTKeyBindEntryWidget.h"
#include "../Core/CTSettingsSubsystem.h"
#include "Components/Image.h"
#include "Components/InputKeySelector.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

void UCTKeyBindEntryWidget::ApplyProjectStyle()
{
	// 프로젝트 UI 그림으로 꾸밈: 분홍 줄(UI_Row_Pink) + 크림색 알약 키 칸(UI_Pill_Status), 글자는 짙은 보라
	const FLinearColor Ink(0.24f, 0.03f, 0.24f, 1.f);
	UTexture2D* RowTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/Style/UI_Row_Pink.UI_Row_Pink"));
	UTexture2D* PillTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/Style/UI_Pill_Status.UI_Pill_Status"));
	StyleTextures = { RowTexture, PillTexture };

	auto MakeBox = [](UTexture2D* Texture, const FMargin& Margin) -> FSlateBrush
	{
		FSlateBrush Brush;
		if (Texture)
		{
			Brush.SetResourceObject(Texture);
			Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.Margin = Margin;
		}
		return Brush;
	};

	// 줄 배경 (원래 어두운 회색 사각형)
	if (UImage* RowImage = Cast<UImage>(GetWidgetFromName(TEXT("Image_KeyRow"))))
	{
		if (RowTexture)
		{
			RowImage->SetBrush(MakeBox(RowTexture, FMargin(0.10f, 0.28f, 0.10f, 0.25f)));
			RowImage->SetColorAndOpacity(FLinearColor::White);
		}
	}

	if (Text_ActionName) { Text_ActionName->SetColorAndOpacity(FSlateColor(Ink)); }
	if (Text_Keys)       { Text_Keys->SetColorAndOpacity(FSlateColor(Ink)); }

	// 키 칸: 크림색 알약 + 짙은 보라 글자
	if (KeySelector && PillTexture)
	{
		const FSlateBrush Normal = MakeBox(PillTexture, FMargin(0.20f, 0.30f, 0.20f, 0.25f));
		FSlateBrush Hovered = Normal;
		Hovered.TintColor = FSlateColor(FLinearColor(1.f, 0.96f, 0.82f, 1.f));
		FSlateBrush Pressed = Normal;
		Pressed.TintColor = FSlateColor(FLinearColor(0.88f, 0.82f, 0.62f, 1.f));

		FButtonStyle Style;
		Style.SetNormal(Normal).SetHovered(Hovered).SetPressed(Pressed);
		Style.SetNormalPadding(FMargin(6.f, 1.f, 6.f, 4.f));
		Style.SetPressedPadding(FMargin(6.f, 3.f, 6.f, 2.f));
		KeySelector->SetButtonStyle(Style);

		FTextBlockStyle TextStyle = KeySelector->GetTextStyle();
		TextStyle.SetColorAndOpacity(FSlateColor(Ink));
		KeySelector->SetTextStyle(TextStyle);
	}
}

void UCTKeyBindEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyProjectStyle();

	if (KeySelector)
	{
		KeySelector->SetAllowModifierKeys(false); // 조합키 불가
		KeySelector->OnKeySelected.AddUniqueDynamic(this, &UCTKeyBindEntryWidget::HandleKeySelected);
	}
}

void UCTKeyBindEntryWidget::NativeDestruct()
{
	if (KeySelector)
	{
		KeySelector->OnKeySelected.RemoveDynamic(this, &UCTKeyBindEntryWidget::HandleKeySelected);
	}

	Super::NativeDestruct();
}

void UCTKeyBindEntryWidget::Setup(const FCTControlRow& Row)
{
	ActionId = Row.Id;
	bRebindable = Row.bRebindable;
	CurrentKey = Row.Keys.IsEmpty() ? FKey() : Row.Keys[0];

	if (Text_ActionName)
	{
		Text_ActionName->SetText(Row.DisplayName);
	}

	// 변경 가능이면 선택기, 아니면 글자
	if (KeySelector)
	{
		KeySelector->SetVisibility(bRebindable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bRebindable)
		{
			ShowKey(CurrentKey);
		}
	}

	if (Text_Keys)
	{
		Text_Keys->SetVisibility(bRebindable ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		if (!bRebindable)
		{
			TArray<FString> Names;
			for (const FKey& Key : Row.Keys)
			{
				Names.Add(Key.GetDisplayName().ToString());
			}
			Text_Keys->SetText(Row.KeysLabel.IsEmpty() ? FText::FromString(FString::Join(Names, TEXT(" "))) : Row.KeysLabel);
		}
	}
}

void UCTKeyBindEntryWidget::ShowKey(FKey Key)
{
	CurrentKey = Key;

	if (KeySelector)
	{
		KeySelector->SetSelectedKey(FInputChord(Key));
	}
}

void UCTKeyBindEntryWidget::HandleKeySelected(FInputChord SelectedKey)
{
	if (!bRebindable) { return; }

	// 거절되면 ShowKey로 되돌림
	OnKeyRebound.Broadcast(ActionId, SelectedKey.Key);
}
