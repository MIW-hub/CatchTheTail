// Copyright Moon Party Games. All Rights Reserved.
#include "CTMenuPlayerController.h"
#include "../Core/CTGameInstance.h"
#include "../UI/CTIntroWidget.h"
#include "Blueprint/UserWidget.h"

void ACTMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 내 화면을 조작하는 컨트롤러에서만 UI를 만듭니다.
	if (!IsLocalController()) { return; }

	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	const bool bAlreadyShown = GameInstance && GameInstance->HasShownIntro();

	if (IntroWidgetClass && !bAlreadyShown)
	{
		ShowIntro();
	}
	else
	{
		ShowMainMenu();
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
