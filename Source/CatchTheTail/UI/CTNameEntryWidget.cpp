// Copyright Moon Party Games. All Rights Reserved.
#include "CTNameEntryWidget.h"
#include "../Core/CTGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"

void UCTNameEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.AddUniqueDynamic(this, &UCTNameEntryWidget::HandleConfirmClicked);
	}

	if (EditableTextBox_Name)
	{
		EditableTextBox_Name->OnTextCommitted.AddUniqueDynamic(this, &UCTNameEntryWidget::HandleTextCommitted);
		EditableTextBox_Name->OnTextChanged.AddUniqueDynamic(this, &UCTNameEntryWidget::HandleTextChanged);

		// 바로 입력할 수 있게 포커스를 줍니다.
		EditableTextBox_Name->SetKeyboardFocus();
	}

	if (Text_Hint)
	{
		if (const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
		{
			Text_Hint->SetText(FText::Format(
				NSLOCTEXT("CT", "UI_Name_Hint", "{0}~{1}자, 한글 / 영문 / 숫자 / _ / -"),
				FText::AsNumber(GameInstance->GetMinNameLength()),
				FText::AsNumber(GameInstance->GetMaxNameLength())));
		}
	}

	ClearError();
}

void UCTNameEntryWidget::NativeDestruct()
{
	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.RemoveDynamic(this, &UCTNameEntryWidget::HandleConfirmClicked);
	}
	if (EditableTextBox_Name)
	{
		EditableTextBox_Name->OnTextCommitted.RemoveDynamic(this, &UCTNameEntryWidget::HandleTextCommitted);
		EditableTextBox_Name->OnTextChanged.RemoveDynamic(this, &UCTNameEntryWidget::HandleTextChanged);
	}

	Super::NativeDestruct();
}

void UCTNameEntryWidget::ConfirmName()
{
	UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	if (!GameInstance) { return; }

	const FString Input = EditableTextBox_Name ? EditableTextBox_Name->GetText().ToString() : FString();

	FText Error;
	if (!GameInstance->SetPlayerName(Input, Error))
	{
		ShowError(Error);
		return;
	}

	ClearError();
	BP_OnNameSaved(GameInstance->GetPlayerName());
	OnNameConfirmed.Broadcast();
}

void UCTNameEntryWidget::HandleConfirmClicked()
{
	ConfirmName();
}

void UCTNameEntryWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	// Enter로 확인합니다. (포커스를 잃어서 생기는 커밋은 무시)
	if (CommitMethod == ETextCommit::OnEnter)
	{
		ConfirmName();
	}
}

void UCTNameEntryWidget::HandleTextChanged(const FText& Text)
{
	ClearError();

	// 최대 글자 수를 넘으면 잘라 냅니다.
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	if (!GameInstance || !EditableTextBox_Name) { return; }

	const FString Current = Text.ToString();
	if (Current.Len() > GameInstance->GetMaxNameLength())
	{
		EditableTextBox_Name->SetText(FText::FromString(Current.Left(GameInstance->GetMaxNameLength())));
	}
}

void UCTNameEntryWidget::ShowError(const FText& Reason)
{
	if (Text_Error)
	{
		Text_Error->SetText(Reason);
		Text_Error->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	BP_OnNameError(Reason);
}

void UCTNameEntryWidget::ClearError()
{
	if (Text_Error)
	{
		Text_Error->SetText(FText::GetEmpty());
		Text_Error->SetVisibility(ESlateVisibility::Collapsed);
	}
}
