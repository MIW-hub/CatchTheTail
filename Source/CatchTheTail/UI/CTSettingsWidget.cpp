// Copyright Moon Party Games. All Rights Reserved.
#include "CTSettingsWidget.h"
#include "CTButtonHoverEffect.h"
#include "../Core/CTAudioSubsystem.h"
#include "CTKeyBindEntryWidget.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTSettingsSubsystem.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
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
	UCTButtonHoverEffect::Attach(this);

	// 버튼 소리 (올림 / 누름)
	UCTAudioSubsystem::ApplyUISounds(this);

	if (ComboBox_Language)     { ComboBox_Language->OnSelectionChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleLanguageChanged); }
	if (Button_LanguageKo)     { Button_LanguageKo->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleLanguageKoClicked); }
	if (Button_LanguageEn)     { Button_LanguageEn->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleLanguageEnClicked); }
	if (Button_LanguageJa)     { Button_LanguageJa->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleLanguageJaClicked); }
	if (Slider_Music)          { Slider_Music->OnValueChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleMusicChanged); }
	if (Slider_Sfx)            { Slider_Sfx->OnValueChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleSfxChanged); }
	if (CheckBox_Mute)         { CheckBox_Mute->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleMuteChanged); }
	if (CheckBox_Fullscreen)   { CheckBox_Fullscreen->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleFullscreenChanged); }
	if (CheckBox_ScreenShake)  { CheckBox_ScreenShake->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleScreenShakeChanged); }
	if (CheckBox_TeamColors)   { CheckBox_TeamColors->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleTeamColorsChanged); }
	if (CheckBox_TeamMarker)   { CheckBox_TeamMarker->OnCheckStateChanged.AddUniqueDynamic(this, &UCTSettingsWidget::HandleTeamMarkerChanged); }
	if (Button_ChangeName)     { Button_ChangeName->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleChangeNameClicked); }
	if (Button_ChangeNameOpen) { Button_ChangeNameOpen->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleChangeNameOpenClicked); }
	if (Button_Reset)          { Button_Reset->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleResetClicked); }
	if (Button_Close)          { Button_Close->OnClicked.AddUniqueDynamic(this, &UCTSettingsWidget::HandleCloseClicked); }

	SetNameChangeOpen(false); // 이름 입력 칸은 버튼을 눌러야 펼쳐짐
	RefreshAll();
}

void UCTSettingsWidget::NativeDestruct()
{
	if (ComboBox_Language)     { ComboBox_Language->OnSelectionChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleLanguageChanged); }
	if (Button_LanguageKo)     { Button_LanguageKo->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleLanguageKoClicked); }
	if (Button_LanguageEn)     { Button_LanguageEn->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleLanguageEnClicked); }
	if (Button_LanguageJa)     { Button_LanguageJa->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleLanguageJaClicked); }
	if (Slider_Music)          { Slider_Music->OnValueChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleMusicChanged); }
	if (Slider_Sfx)            { Slider_Sfx->OnValueChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleSfxChanged); }
	if (CheckBox_Mute)         { CheckBox_Mute->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleMuteChanged); }
	if (CheckBox_Fullscreen)   { CheckBox_Fullscreen->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleFullscreenChanged); }
	if (CheckBox_ScreenShake)  { CheckBox_ScreenShake->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleScreenShakeChanged); }
	if (CheckBox_TeamColors)   { CheckBox_TeamColors->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleTeamColorsChanged); }
	if (CheckBox_TeamMarker)   { CheckBox_TeamMarker->OnCheckStateChanged.RemoveDynamic(this, &UCTSettingsWidget::HandleTeamMarkerChanged); }
	if (Button_ChangeName)     { Button_ChangeName->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleChangeNameClicked); }
	if (Button_ChangeNameOpen) { Button_ChangeNameOpen->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleChangeNameOpenClicked); }
	if (Button_Reset)          { Button_Reset->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleResetClicked); }
	if (Button_Close)          { Button_Close->OnClicked.RemoveDynamic(this, &UCTSettingsWidget::HandleCloseClicked); }

	Super::NativeDestruct();
}

// 화면 채우기

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

	// 채우는 중 변경 알림 저장 방지
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

	// 변경 영역은 바꿀 수 있을 때만 표시
	if (Text_PlayerName)
	{
		const UCTGameInstance* GameInstance = Cast<UCTGameInstance>(GetGameInstance());
		Text_PlayerName->SetText(FText::FromString(GameInstance ? GameInstance->GetPlayerName() : FString()));
	}
	if (Panel_ChangeName)
	{
		Panel_ChangeName->SetVisibility(Settings->CanChangePlayerName() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	RefreshLanguage();
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

// 값 변경

void UCTSettingsWidget::RefreshLanguage()
{
	UCTSettingsSubsystem* Settings = GetSettings();
	if (!Settings) { return; }

	// 언어 버튼: 지금 언어만 또렷하게
	const FString CurrentLanguage = Settings->GetLanguage();
	auto Mark = [this, &CurrentLanguage](UButton* Button, const TCHAR* Code)
	{
		if (Button) { Button->SetRenderOpacity(CurrentLanguage == Code ? 1.f : UnselectedLanguageOpacity); }
	};
	Mark(Button_LanguageKo, TEXT("ko"));
	Mark(Button_LanguageEn, TEXT("en"));
	Mark(Button_LanguageJa, TEXT("ja"));

	if (!ComboBox_Language) { return; }

	TGuardValue<bool> Guard(bRefreshing, true);

	ComboBox_Language->ClearOptions();
	const FString Current = Settings->GetLanguage();
	int32 CurrentIndex = 0;
	int32 Index = 0;
	for (const FString& Code : UCTSettingsSubsystem::GetSupportedLanguages())
	{
		ComboBox_Language->AddOption(UCTSettingsSubsystem::GetLanguageDisplayName(Code));
		if (Code == Current) { CurrentIndex = Index; }
		++Index;
	}
	// 이름이 아니라 순서로 골라서, 지금 쓰는 언어가 확실히 선택된 상태로 보이게 함
	ComboBox_Language->SetSelectedIndex(CurrentIndex);
}

void UCTSettingsWidget::HandleLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UCTSettingsSubsystem* Settings = GetSettings();
	if (bRefreshing || !Settings) { return; }

	// 선택한 표시 이름에 해당하는 언어 코드를 찾음
	for (const FString& Code : UCTSettingsSubsystem::GetSupportedLanguages())
	{
		if (UCTSettingsSubsystem::GetLanguageDisplayName(Code) == SelectedItem)
		{
			Settings->SetLanguage(Code);
			RefreshAll(); // 조작 목록 등 문구를 새 언어로 다시 채움
			return;
		}
	}
}

void UCTSettingsWidget::HandleLanguageKoClicked() { ChooseLanguage(TEXT("ko")); }
void UCTSettingsWidget::HandleLanguageEnClicked() { ChooseLanguage(TEXT("en")); }
void UCTSettingsWidget::HandleLanguageJaClicked() { ChooseLanguage(TEXT("ja")); }

void UCTSettingsWidget::ChooseLanguage(const FString& CultureCode)
{
	if (bRefreshing) { return; }
	if (UCTSettingsSubsystem* Settings = GetSettings())
	{
		Settings->SetLanguage(CultureCode);
		RefreshAll(); // 조작 안내 등 문구를 새 언어로 다시 채움
	}
}

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

	// 거절 시 이유 표시, 목록 다시 그려 키 복원
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
	SetNameChangeOpen(false);
	RefreshAll();
}

void UCTSettingsWidget::HandleChangeNameOpenClicked()
{
	// 누를 때마다 펼침/접힘
	SetNameChangeOpen(!Row_NameChange || Row_NameChange->GetVisibility() == ESlateVisibility::Collapsed);
}

void UCTSettingsWidget::SetNameChangeOpen(bool bOpen)
{
	if (Row_NameChange)
	{
		Row_NameChange->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (bOpen)
	{
		ShowNameMessage(FText::GetEmpty());
		if (EditableTextBox_NewName) { EditableTextBox_NewName->SetKeyboardFocus(); }
	}
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
