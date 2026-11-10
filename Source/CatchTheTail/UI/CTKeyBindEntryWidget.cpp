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
	// 프로젝트 UI 그림으로 꾸밈: 분홍 줄(UI_Row_Pink) + 짙은 보라 글자
	// 주의: 키 칸(InputKeySelector)의 버튼 스타일은 바꾸지 않음. 커스텀 버튼 스타일을 주면
	// 위젯이 다시 만들어질 때 그리는 중 크래시(접근 위반)가 나서, 글자 색만 바꿈
	const FLinearColor Ink(0.24f, 0.03f, 0.24f, 1.f);
	UTexture2D* RowTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/2D/UI/Style/UI_Row_Pink.UI_Row_Pink"));
	StyleTextures = { RowTexture };

	// 줄 배경 (원래 어두운 회색 사각형)
	if (UImage* RowImage = Cast<UImage>(GetWidgetFromName(TEXT("Image_KeyRow"))))
	{
		if (RowTexture)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(RowTexture);
			Brush.ImageSize = FVector2D(RowTexture->GetSizeX(), RowTexture->GetSizeY());
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.Margin = FMargin(0.10f, 0.28f, 0.10f, 0.25f);
			RowImage->SetBrush(Brush);
			RowImage->SetColorAndOpacity(FLinearColor::White);
		}
	}

	if (Text_ActionName) { Text_ActionName->SetColorAndOpacity(FSlateColor(Ink)); }
	if (Text_Keys)       { Text_Keys->SetColorAndOpacity(FSlateColor(Ink)); }

	// 키 칸 글자 색만 짙은 보라로
	if (KeySelector)
	{
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
