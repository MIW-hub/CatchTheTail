// Copyright Moon Party Games. All Rights Reserved.
#include "CTLanguageSelectWidget.h"
#include "CTButtonHoverEffect.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTSettingsSubsystem.h"
#include "Components/Button.h"

void UCTLanguageSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UCTButtonHoverEffect::Attach(this);

	// 버튼 소리 (올림 / 누름)
	UCTAudioSubsystem::ApplyUISounds(this);

	if (Button_LanguageKo) { Button_LanguageKo->OnClicked.AddUniqueDynamic(this, &UCTLanguageSelectWidget::HandleKoClicked); }
	if (Button_LanguageEn) { Button_LanguageEn->OnClicked.AddUniqueDynamic(this, &UCTLanguageSelectWidget::HandleEnClicked); }
	if (Button_LanguageJa) { Button_LanguageJa->OnClicked.AddUniqueDynamic(this, &UCTLanguageSelectWidget::HandleJaClicked); }
	if (Button_Confirm)    { Button_Confirm->OnClicked.AddUniqueDynamic(this, &UCTLanguageSelectWidget::HandleConfirmClicked); }

	RefreshSelection();
}

void UCTLanguageSelectWidget::NativeDestruct()
{
	if (Button_LanguageKo) { Button_LanguageKo->OnClicked.RemoveDynamic(this, &UCTLanguageSelectWidget::HandleKoClicked); }
	if (Button_LanguageEn) { Button_LanguageEn->OnClicked.RemoveDynamic(this, &UCTLanguageSelectWidget::HandleEnClicked); }
	if (Button_LanguageJa) { Button_LanguageJa->OnClicked.RemoveDynamic(this, &UCTLanguageSelectWidget::HandleJaClicked); }
	if (Button_Confirm)    { Button_Confirm->OnClicked.RemoveDynamic(this, &UCTLanguageSelectWidget::HandleConfirmClicked); }

	Super::NativeDestruct();
}

void UCTLanguageSelectWidget::HandleKoClicked() { SelectLanguage(TEXT("ko")); }
void UCTLanguageSelectWidget::HandleEnClicked() { SelectLanguage(TEXT("en")); }
void UCTLanguageSelectWidget::HandleJaClicked() { SelectLanguage(TEXT("ja")); }

void UCTLanguageSelectWidget::SelectLanguage(const FString& CultureCode)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (UCTSettingsSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<UCTSettingsSubsystem>() : nullptr)
	{
		// 미리보기: 바로 적용하되 확인 전까지는 "골랐음"으로 치지 않음
		Settings->SetLanguage(CultureCode, /*bConfirm=*/false);
	}

	RefreshSelection();
	BP_OnLanguageSelected(CultureCode);
}

void UCTLanguageSelectWidget::RefreshSelection()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UCTSettingsSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
	const FString Current = Settings ? Settings->GetLanguage() : FString(TEXT("en"));

	// 고른 언어만 또렷하게
	auto Mark = [this, &Current](UButton* Button, const TCHAR* Code)
	{
		if (Button) { Button->SetRenderOpacity(Current == Code ? 1.f : UnselectedOpacity); }
	};
	Mark(Button_LanguageKo, TEXT("ko"));
	Mark(Button_LanguageEn, TEXT("en"));
	Mark(Button_LanguageJa, TEXT("ja"));
}

void UCTLanguageSelectWidget::HandleConfirmClicked()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (UCTSettingsSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<UCTSettingsSubsystem>() : nullptr)
	{
		Settings->ConfirmLanguage();
	}

	OnLanguageConfirmed.Broadcast();
}
