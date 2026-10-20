// Copyright Moon Party Games. All Rights Reserved.
#include "CTKeyBindEntryWidget.h"
#include "../Core/CTSettingsSubsystem.h"
#include "Components/InputKeySelector.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UCTKeyBindEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (KeySelector)
	{
		KeySelector->SetAllowModifierKeys(false); // Shift+X 같은 조합은 받지 않습니다.
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

	// 키를 바꿀 수 있으면 선택기, 아니면 키 안내 글자를 보여 줍니다.
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
			Text_Keys->SetText(FText::FromString(FString::Join(Names, TEXT(" "))));
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

	// 설정 창이 키를 바꿔도 되는지 확인하고, 거절되면 ShowKey로 이전 키를 되돌립니다.
	OnKeyRebound.Broadcast(ActionId, SelectedKey.Key);
}
