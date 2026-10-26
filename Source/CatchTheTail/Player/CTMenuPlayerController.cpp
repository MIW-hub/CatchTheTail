// Copyright Moon Party Games. All Rights Reserved.
#include "CTMenuPlayerController.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTSettingsSubsystem.h"
#include "../UI/CTIntroWidget.h"
#include "../UI/CTNameEntryWidget.h"
#include "../UI/CTLanguageSelectWidget.h"
#include "Blueprint/UserWidget.h"

void ACTMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 컨트롤러만 UI
	if (!IsLocalController()) { return; }

	// 메뉴 배경음
	if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
	{
		Audio->PlayMusic(ECTMusic::Menu);
	}

	// 저장된 소리 설정 적용
	if (UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr)
	{
		Settings->ApplyAudio();
	}

	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	const bool bAlreadyShown = GameInstance && GameInstance->HasShownIntro();

	if (IntroWidgetClass && !bAlreadyShown)
	{
		ShowIntro();
	}
	else
	{
		ShowLanguageSelectOrMenu();
	}
}

void ACTMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IntroWidget)
	{
		IntroWidget->OnIntroFinished.RemoveAll(this);
		IntroWidget->RemoveFromParent();
		IntroWidget = nullptr;
	}

	if (LanguageSelectWidget)
	{
		LanguageSelectWidget->OnLanguageConfirmed.RemoveAll(this);
		LanguageSelectWidget->RemoveFromParent();
		LanguageSelectWidget = nullptr;
	}

	if (NameEntryWidget)
	{
		NameEntryWidget->OnNameConfirmed.RemoveAll(this);
		NameEntryWidget->RemoveFromParent();
		NameEntryWidget = nullptr;
	}

	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ACTMenuPlayerController::ShowIntro()
{
	IntroWidget = CreateWidget<UCTIntroWidget>(this, IntroWidgetClass);
	if (!IntroWidget)
	{
		ShowMainMenu();
		return;
	}

	IntroWidget->OnIntroFinished.AddUniqueDynamic(this, &ACTMenuPlayerController::HandleIntroFinished);
	ShowWidgetWithUIInput(IntroWidget);
}

void ACTMenuPlayerController::HandleIntroFinished()
{
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->MarkIntroShown();
	}

	if (IntroWidget)
	{
		IntroWidget->OnIntroFinished.RemoveAll(this);
		IntroWidget->RemoveFromParent();
		IntroWidget = nullptr;
	}

	ShowLanguageSelectOrMenu();
}

void ACTMenuPlayerController::ShowNameEntryOrMenu()
{
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();

	// 언어 선택 다음 단계: 이름 없으면 입력 (위젯 없으면 생략)
	if (NameEntryWidgetClass && GameInstance && !GameInstance->HasPlayerName())
	{
		ShowNameEntry();
	}
	else
	{
		ShowMainMenu();
	}
}

void ACTMenuPlayerController::ShowLanguageSelectOrMenu()
{
	const UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr;

	// 언어를 아직 안 골랐으면 먼저 선택 (위젯 없으면 생략)
	if (!LanguageSelectWidgetClass || !Settings || Settings->HasChosenLanguage())
	{
		ShowNameEntryOrMenu();
		return;
	}

	if (LanguageSelectWidget)
	{
		LanguageSelectWidget->OnLanguageConfirmed.RemoveAll(this);
		LanguageSelectWidget->RemoveFromParent();
		LanguageSelectWidget = nullptr;
	}

	LanguageSelectWidget = CreateWidget<UCTLanguageSelectWidget>(this, LanguageSelectWidgetClass);
	if (!LanguageSelectWidget)
	{
		ShowNameEntryOrMenu();
		return;
	}

	LanguageSelectWidget->OnLanguageConfirmed.AddUniqueDynamic(this, &ACTMenuPlayerController::HandleLanguageConfirmed);
	ShowWidgetWithUIInput(LanguageSelectWidget);
}

void ACTMenuPlayerController::HandleLanguageConfirmed()
{
	if (LanguageSelectWidget)
	{
		LanguageSelectWidget->OnLanguageConfirmed.RemoveAll(this);
		LanguageSelectWidget->RemoveFromParent();
		LanguageSelectWidget = nullptr;
	}

	ShowNameEntryOrMenu();
}

void ACTMenuPlayerController::ShowNameEntry()
{
	if (!NameEntryWidgetClass) { return; }

	// 메뉴 내렸다가 이름 저장 후 복귀
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}
	if (NameEntryWidget)
	{
		NameEntryWidget->OnNameConfirmed.RemoveAll(this);
		NameEntryWidget->RemoveFromParent();
		NameEntryWidget = nullptr;
	}

	NameEntryWidget = CreateWidget<UCTNameEntryWidget>(this, NameEntryWidgetClass);
	if (!NameEntryWidget)
	{
		ShowMainMenu();
		return;
	}

	NameEntryWidget->OnNameConfirmed.AddUniqueDynamic(this, &ACTMenuPlayerController::HandleNameConfirmed);
	ShowWidgetWithUIInput(NameEntryWidget);
}

void ACTMenuPlayerController::HandleNameConfirmed()
{
	if (NameEntryWidget)
	{
		NameEntryWidget->OnNameConfirmed.RemoveAll(this);
		NameEntryWidget->RemoveFromParent();
		NameEntryWidget = nullptr;
	}

	ShowMainMenu();
}

void ACTMenuPlayerController::ShowMainMenu()
{
	if (!MainMenuWidgetClass) { return; }

	MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
	if (MainMenuWidget)
	{
		ShowWidgetWithUIInput(MainMenuWidget);
	}
}

void ACTMenuPlayerController::ShowWidgetWithUIInput(UUserWidget* Widget)
{
	Widget->AddToViewport();

	// 커서 표시 + UI 입력 모드
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}
