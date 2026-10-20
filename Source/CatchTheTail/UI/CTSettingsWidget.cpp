// Copyright Moon Party Games. All Rights Reserved.
#include "CTSettingsWidget.h"
#include "CTKeyBindEntryWidget.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTSettingsSubsystem.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"

UCTSettingsSubsystem* UCTSettingsWidget::GetSettings() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
}

void UCTSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Slider_Music)          { Slider_Music->OnValueChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleMusicChanged); }
	if (Slider_Sfx)            { Slider_Sfx->OnValueChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleSfxChanged); }
	if (CheckBox_Mute)         { CheckBox_Mute->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleMuteChanged); }
	if (CheckBox_Fullscreen)   { CheckBox_Fullscreen->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleFullscreenChanged); }
	if (CheckBox_ScreenShake)  { CheckBox_ScreenShake->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleScreenShakeChanged); }
	if (CheckBox_TeamColors)   { CheckBox_TeamColors->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleTeamColorsChanged); }
	if (CheckBox_TeamMarker)   { CheckBox_TeamMarker->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleTeamMarkerChanged); }
	if (Button_ChangeName)     { Button_ChangeName->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleChangeNameClicked); }
	if (Button_Reset)          { Button_Reset->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleResetClicked); }
	if (Button_Close)          { Button_Close->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleCloseClicked); }

	RefreshAll();
}

void UCTSettingsWidget::NativeDestruct()
{
	if (Slider_Music)          { Slider_Music->OnValueChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleMusicChanged); }
	if (Slider_Sfx)            { Slider_Sfx->OnValueChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleSfxChanged); }
	if (CheckBox_Mute)         { CheckBox_Mute->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleMuteChanged); }
	if (CheckBox_Fullscreen)   { CheckBox_Fullscreen->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleFullscreenChanged); }
	if (CheckBox_ScreenShake)  { CheckBox_ScreenShake->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleScreenShakeChanged); }
	if (CheckBox_TeamColors)   { CheckBox_TeamColors->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleTeamColorsChanged); }
	if (CheckBox_TeamMarker)   { CheckBox_TeamMarker->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleTeamMarkerChanged); }
	if (Button_ChangeName)     { Button_ChangeName->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleChangeNameClicked); }
	if (Button_Reset)          { Button_Reset->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleResetClicked); }
	if (Button_Close)          { Button_Close->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleCloseClicked); }

	Super::NativeDestruct();
}

// ============================================================
// 화면 채우기
// ============================================================

void UCTSettingsWidget::SetPercentText(UTextBlock* Text, float Value)
{
	if (Text)
	{
		Text->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
	}
}

void UCTSettingsWidget::RefreshAll()
{
	UCTSettingsSubsystem* Settings = GetSettings();
	if (!Settings) { return; }

	// 값을 채우는 동안 생기는 변경 알림이 다시 저장되지 않도록 막습니다.
	TGuardValue<bool> Guard(bRefreshing, true);

	if (Slider_Music)         { Slider_Music->SetValue(Settings->GetMusicVolume()); }
	if (Slider_Sfx)           { Slider_Sfx->SetValue(Settings->GetSfxVolume()); }
	SetPercentText(Text_MusicValue, Settings->GetMusicVolume());
	SetPercentText(Text_SfxValue, Settings->GetSfxVolume());

	if (CheckBox_Mute)        { CheckBox_Mute->SetIsChecked(Settings->IsMuted()); }
	if (CheckBox_Fullscreen)  { CheckBox_Fullscreen->SetIsChecked(Settings->IsFullscreen()); }
	if (CheckBox_ScreenShake) { CheckBox_ScreenShake->SetIsChecked(Settings->IsScreenShakeEnabled()); }
	if (CheckBox_TeamColors)  { CheckBox_TeamColors->SetIsChecked(Settings->IsTeamColorAssistEnabled()); }
	if (CheckBox_TeamMarker)  { CheckBox_TeamMarker->SetIsChecked(Settings->IsTeamMarkerEnabled()); }

	// 개인 설정: 이름은 항상 보여 주고, 바꿀 수 있는 설정일 때만 변경 영역을 보여 줍니다.
	if (Text_PlayerName)
	{
		const UCTGameInstance* GameInstance = Cast<UCTGameInstance>(GetGameInstance());
		Text_PlayerName->SetText(FText::FromString(GameInstance ? GameInstance->GetPlayerName() : FString()));
	}
	if (Panel_ChangeName)
	{
		Panel_ChangeName->SetVisibility(Settings->CanChangePlayerName() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	BuildControlRows();
	BP_OnRefreshed();
}

void UCTSettingsWidget::BuildControlRows()
{
	UCTSettingsSubsystem* Settings = GetSettings();
	if (!VerticalBox_Controls || !KeyBindEntryClass || !Settings) { return; }

	VerticalBox_Controls->ClearChildren();

	for (const FCTControlRow& Row : Settings->GetControlRows())
	{
		UCTKeyBindEntryWidget* Entry = CreateWidget<UCTKeyBindEntryWidget>(this, KeyBindEntryClass);
		if (!Entry) { continue; }

		Entry->Setup(Row);
		Entry->OnKeyRebound.AddUniqueDynamic(this, &UCTSettingsWidget::HandleKeyRebound);
		VerticalBox_Controls->AddChildToVerticalBox(Entry);
	}
}

void UCTSettingsWidget::ShowMessage(const FText& Message)
{
	if (Text_Message)
	{
		Text_Message->SetText(Message);
	}
	if (Text_ControlsMessage)
	{
		Text_ControlsMessage->SetText(Message);
	}
	if (!Message.IsEmpty())
	{
		BP_OnMessage(Message);
	}
}

void UCTSettingsWidget::ShowNameMessage(const FText& Message)
{
	if (Text_NameMessage)
	{
		Text_NameMessage->SetText(Message);
	}
	if (!Message.IsEmpty())
	{
		BP_OnMessage(Message);
	}
}

// ============================================================
// 값 변경 (바꾸면 바로 적용하고 저장)
// ============================================================

void UCTSettingsWidget::HandleMusicChanged(float Value)
{
	if (bRefreshing) { return; }
	SetPercentText(Text_MusicValue, Value);
	if (UCTSettingsSubsystem* Settings = GetSettings()) { Settings->SetMusicVolume(Value); }
}

void UCTSettingsWidget::HandleSfxChanged(float Value)
{
	if (bRefreshing) { return; }
	SetPercentText(Text_SfxValue, Value);
	if (UCTSettingsSubsystem* Settings = GetSettings()) { Settings->SetSfxVolume(Value); }
}

void UCTSettingsWidget::HandleMuteChanged(bool bIsChecked)
{
	if (bRefreshing) { return; }
	if (UCTSettingsSubsystem* Settings = GetSettings()) { Settings->SetMuted(bIsChecked); }
}

void UCTSettingsWidget::HandleFullscreenChanged(bool bIsChecked)
{
	if (bRefreshing) { return; }
	if (UCTSettingsSubsystem* Settings = GetSettings()) { Settings->SetFullscreen(bIsChecked); }
}

void UCTSettingsWidget::HandleScreenShakeChanged(bool bIsChecked)
{
	if (bRefreshing) { return; }
	if (UCTSettingsSubsystem* Settings = GetSettings()) { Settings->SetScreenShakeEnabled(bIsChecked); }
}

void UCTSettingsWidget::HandleTeamColorsChanged(bool bIsChecked)
{
	if (bRefreshing) { return; }
	if (UCTSettingsSubsystem* Settings = GetSettings()) { Settings->SetTeamColorAssistEnabled(bIsChecked); }
}

void UCTSettingsWidget::HandleTeamMarkerChanged(bool bIsChecked)
{
	if (bRefreshing) { return; }
	if (UCTSettingsSubsystem* Settings = GetSettings()) { Settings->SetTeamMarkerEnabled(bIsChecked); }
}

void UCTSettingsWidget::HandleKeyRebound(FName ActionId, FKey NewKey)
{
	UCTSettingsSubsystem* Settings = GetSettings();
	if (!Settings) { return; }

	FText Error;
	if (Settings->SetKeyBinding(ActionId, NewKey, Error))
	{
		ShowMessage(FText::GetEmpty());
		return;
	}

	// 거절되면 이유를 보여 주고, 목록을 다시 그려서 키 표시를 원래대로 돌립니다.
	ShowMessage(Error);
	BuildControlRows();
}

void UCTSettingsWidget::HandleChangeNameClicked()
{
	UCTSettingsSubsystem* Settings = GetSettings();
	if (!Settings) { return; }

	const FString Input = EditableTextBox_NewName ? EditableTextBox_NewName->GetText().ToString() : FString();

	FText Error;
	if (!Settings->ChangePlayerName(Input, Error))
	{
		ShowNameMessage(Error);
		return;
	}

	ShowNameMessage(NSLOCTEXT("CT", "Settings_NameChanged", "이름을 바꿨습니다."));
	if (EditableTextBox_NewName) { EditableTextBox_NewName->SetText(FText::GetEmpty()); }
	RefreshAll();
}

void UCTSettingsWidget::ResetSettings()
{
	if (UCTSettingsSubsystem* Settings = GetSettings())
	{
		Settings->ResetToDefaults();
		RefreshAll();
		ShowMessage(NSLOCTEXT("CT", "Settings_ResetDone", "기본 설정으로 되돌렸습니다."));
	}
}

void UCTSettingsWidget::HandleResetClicked()
{
	ResetSettings();
}

void UCTSettingsWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}
