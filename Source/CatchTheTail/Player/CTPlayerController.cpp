// Copyright Moon Party Games. All Rights Reserved.
#include "CTPlayerController.h"
#include "../Core/CTSettingsSubsystem.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameState.h"
#include "GameFramework/PlayerState.h"
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

	// 로컬만 입력 등록
	if (!IsLocalController())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			// 변경된 키 반영한 복사본 등록
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

	// 맵 변경 시 볼륨 재적용
	if (UCTSettingsSubsystem* SoundSettings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr)
	{
		SoundSettings->ApplyAudio();
	}

	// 커서 숨김, 게임 입력만
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = ViewPitchMin;
		PlayerCameraManager->ViewPitchMax = ViewPitchMax;
	}

	// 이름을 컴퓨터 이름 대신 저장된 이름으로
	if (const UCTGameInstance* NameGameInstance = GetGameInstance<UCTGameInstance>())
	{
		if (NameGameInstance->HasPlayerName())
		{
			ServerSetPlayerName(NameGameInstance->GetPlayerName());
		}
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

void ACTPlayerController::RequestReturnToLobby()
{
	ServerReturnToLobby();
}

void ACTPlayerController::ServerReturnToLobby_Implementation()
{
	if (ACTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTGameMode>() : nullptr)
	{
		GameMode->RequestReturnToLobby(this);
	}
}

void ACTPlayerController::ServerSetPlayerName_Implementation(const FString& NewName)
{
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	if (!GameInstance || !PlayerState) { return; }

	// 서버에서도 같은 규칙으로 검사
	const FString Trimmed = NewName.TrimStartAndEnd();
	FText Error;
	if (!GameInstance->ValidatePlayerName(Trimmed, Error)) { return; }

	PlayerState->SetPlayerName(Trimmed);
}

// UI

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
		// 결과 화면 미리 생성, 종료 시 표시
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

	// 이미 끝난 경기 대응
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
		ResultWidget->AddToViewport(10); // HUD 위
	}
	ResultWidget->ShowResult();

	// 커서 표시
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

	// 설정에서 끔이면 생략
	const UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
	if (Settings && !Settings->IsScreenShakeEnabled()) { return; }

	PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
}
