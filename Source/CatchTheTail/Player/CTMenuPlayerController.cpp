// Copyright Moon Party Games. All Rights Reserved.
#include "CTMenuPlayerController.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTSettingsSubsystem.h"
#include "../UI/CTIntroWidget.h"
#include "../UI/CTNameEntryWidget.h"
#include "Blueprint/UserWidget.h"

void ACTMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 내 화면을 조작하는 컨트롤러에서만 UI를 만듭니다.
	if (!IsLocalController()) { return; }

	// 저장한 소리 설정을 이 맵에 적용합니다.
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
		ShowNameEntryOrMenu();
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

	ShowNameEntryOrMenu();
}

void ACTMenuPlayerController::ShowNameEntryOrMenu()
{
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();

	// 저장된 이름이 없으면 먼저 이름을 입력받습니다. (이름 입력 위젯이 없으면 건너뜀)
	if (NameEntryWidgetClass && GameInstance && !GameInstance->HasPlayerName())
	{
		ShowNameEntry();
	}
	else
	{
		ShowMainMenu();
	}
}

void ACTMenuPlayerController::ShowNameEntry()
{
	if (!NameEntryWidgetClass) { return; }

	// 메인 메뉴가 떠 있으면 내리고, 이름을 저장하면 다시 올립니다.
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

	// 버튼과 입력창을 쓸 수 있도록 마우스 커서를 보이고 UI 입력 모드로 바꿉니다.
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}
