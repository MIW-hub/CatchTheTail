// Copyright Moon Party Games. All Rights Reserved.
#include "CTPlayerController.h"
#include "../Core/CTSettingsSubsystem.h"
#include "../Core/CTGameState.h"
#include "../UI/CTHUDWidget.h"
#include "../UI/CTResultWidget.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraShakeBase.h"
#include "TimerManager.h"

void ACTPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 내 화면을 조작하는 컨트롤러에서만 입력을 등록합니다.
	// (서버에 있는 다른 사람의 컨트롤러는 건너뜁니다)
	if (!IsLocalController())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			// 설정 창에서 바꾼 키가 있으면 그 키가 들어간 복사본을 등록합니다.
			UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr;

			for (UInputMappingContext* Context : DefaultMappingContexts)
			{
				if (Context)
				{
					UInputMappingContext* Effective = Settings ? Settings->GetEffectiveMappingContext(Context) : Context;
					Subsystem->AddMappingContext(Effective, MappingPriority);
				}
			}
		}
	}

	// 맵이 바뀌면 소리 설정(볼륨)을 다시 적용합니다.
	if (UCTSettingsSubsystem* SoundSettings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr)
	{
		SoundSettings->ApplyAudio();
	}

	// 마우스 커서 숨기고 게임 입력만 받기
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = ViewPitchMin;
		PlayerCameraManager->ViewPitchMax = ViewPitchMax;
	}

	CreateUI();
	TryBindGameState();
}

void ACTPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BindGameStateTimerHandle);

	if (ACTGameState* GameState = BoundGameState.Get())
	{
		GameState->OnPhaseChanged.RemoveDynamic(this, &ACTPlayerController::HandlePhaseChanged);
	}

	Super::EndPlay(EndPlayReason);
}

// ============================================================
// UI
// ============================================================

void ACTPlayerController::CreateUI()
{
	if (HUDWidgetClass && !HUDWidget)
	{
		HUDWidget = CreateWidget<UCTHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
		}
	}

	if (ResultWidgetClass && !ResultWidget)
	{
		// 결과 화면은 미리 만들어 두고 경기가 끝날 때 화면에 붙입니다.
		ResultWidget = CreateWidget<UCTResultWidget>(this, ResultWidgetClass);
	}
}

void ACTPlayerController::TryBindGameState()
{
	ACTGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	if (!GameState)
	{
		GetWorldTimerManager().SetTimer(
			BindGameStateTimerHandle, this, &ACTPlayerController::TryBindGameState, 0.2f, false);
		return;
	}

	BoundGameState = GameState;
	GameState->OnPhaseChanged.AddUniqueDynamic(this, &ACTPlayerController::HandlePhaseChanged);

	// 이미 끝난 경기에 들어온 경우도 처리
	HandlePhaseChanged(GameState->GetPhase());
}

void ACTPlayerController::HandlePhaseChanged(ECTGamePhase NewPhase)
{
	if (NewPhase == ECTGamePhase::Finished)
	{
		ShowResultScreen();
	}
	else
	{
		HideResultScreen();
	}
}

void ACTPlayerController::ShowResultScreen()
{
	if (!ResultWidget) { return; }

	if (!ResultWidget->IsInViewport())
	{
		ResultWidget->AddToViewport(10); // HUD보다 위에
	}
	ResultWidget->ShowResult();

	// 결과 화면 버튼을 누를 수 있도록 마우스 커서 표시
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ResultWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ACTPlayerController::HideResultScreen()
{
	if (!ResultWidget || !ResultWidget->IsInViewport()) { return; }

	ResultWidget->RemoveFromParent();

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void ACTPlayerController::PlayCTCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass || !IsLocalController() || !PlayerCameraManager) { return; }

	// 설정에서 화면 흔들림을 껐으면 재생하지 않습니다.
	const UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
	if (Settings && !Settings->IsScreenShakeEnabled()) { return; }

	PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
}
