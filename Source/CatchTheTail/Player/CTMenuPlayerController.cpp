// Copyright Moon Party Games. All Rights Reserved.
// [보류 중] 켜는 방법은 CTGameInstance.h 맨 위 설명을 보세요.
#if 0
#include "CTMenuPlayerController.h"
#include "Blueprint/UserWidget.h"

void ACTMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 내 화면을 조작하는 컨트롤러에서만 메뉴를 만듭니다.
	if (!IsLocalController() || !MainMenuWidgetClass) { return; }

	MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
	if (!MainMenuWidget) { return; }

	MainMenuWidget->AddToViewport();

	// 버튼과 입력창을 쓸 수 있도록 마우스 커서를 보이고 UI 입력 모드로 바꿉니다.
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ACTMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
#endif // 보류: 메인 메뉴 / 방 만들기
