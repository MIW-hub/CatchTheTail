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

	// 거절되면 ShowKey로 되돌림
	OnKeyRebound.Broadcast(ActionId, SelectedKey.Key);
}
