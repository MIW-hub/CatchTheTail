// Copyright Moon Party Games. All Rights Reserved.
#include "CTPlayerController.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTSettingsSubsystem.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameState.h"
#include "GameFramework/PlayerState.h"
#include "CTPlayerState.h"
#include "../UI/CTHUDWidget.h"
#include "../UI/CTResultWidget.h"
#include "../UI/CTPauseMenuWidget.h"
#include "../UI/CTPhonePopupWidget.h"
#include "../World/CTPhoneBooth.h"
#include "CTCharacter.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraShakeBase.h"
#include "TimerManager.h"

ACTPlayerController::ACTPlayerController()
{
	// 기본 ESC 메뉴 (WBP_CTPauseMenuWidget). 컨트롤러 블루프린트에서 바꿀 수 있음
	static ConstructorHelpers::FClassFinder<UCTPauseMenuWidget> PauseMenuFinder(TEXT("/Game/UI/WBP_CTPauseMenuWidget"));
	if (PauseMenuFinder.Succeeded())
	{
		PauseMenuWidgetClass = PauseMenuFinder.Class;
	}

	PhonePopupWidgetClass = UCTPhonePopupWidget::StaticClass();
}

void ACTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		// 메뉴가 열려 있어도 같은 키로 닫을 수 있게 일시 정지 중에도 받음
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACTPlayerController::HandlePauseKey).bExecuteWhenPaused = true;
		InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ACTPlayerController::HandlePhoneKey);
	}
}

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
			// 변경 키 반영 복사본 등록
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

	// 배경음
	if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
	{
		Audio->PlayMusic(GetLevelMusic());
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
		GameState->OnRoundChanged.RemoveDynamic(this, &ACTPlayerController::HandleRoundChanged);
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

	// 서버 재검사
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
	GameState->OnRoundChanged.AddUniqueDynamic(this, &ACTPlayerController::HandleRoundChanged);

	// 이미 끝난 경기 대응
	HandlePhaseChanged(GameState->GetPhase());
}

void ACTPlayerController::HandleRoundChanged(int32 NewRoundNumber)
{
	if (!IsLocalController() || NewRoundNumber <= 0) { return; }

	if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
	{
		Audio->PlaySfx(ECTSfx::RoundStart);
	}
}

void ACTPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalController()) { return; }

	// 부스를 벗어나거나 경기가 끝나면 전화 팝업을 닫음
	if (IsPhonePopupOpen() && !CanKeepPhonePopup()) { ClosePhonePopup(); }

	// 카운트다운 숫자 바뀔 때 1회 소리
	const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	int32 Number = 0;
	if (CTGameState)
	{
		if (CTGameState->IsStartCountdown()) { Number = FMath::CeilToInt(CTGameState->GetStartCountdownRemainingSeconds()); }
		else if (CTGameState->GetLobbyStartRemainingSeconds() > 0.f) { Number = FMath::CeilToInt(CTGameState->GetLobbyStartRemainingSeconds()); }
		else if (CTGameState->IsCountdownActive()) { Number = CTGameState->GetCountdownNumber(); }
	}

	if (Number != LastCountdownSoundNumber)
	{
		LastCountdownSoundNumber = Number;
		if (Number > 0)
		{
			if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
			{
				Audio->PlaySfx(ECTSfx::CountdownTick);
			}
		}
	}
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
	// 월드가 정리되는 중에는 위젯을 만들 수 없음
	if (!ResultWidget || !GetWorld() || GetWorld()->bIsTearingDown) { return; }

	// 열려 있던 ESC 메뉴와 전화 팝업은 닫고 결과 화면이 이어받음
	ClosePhonePopup();
	ClosePauseMenu();

	// 내가 승자면 승리 소리
	const ACTGameState* ResultGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	if (ResultGameState && ResultGameState->IsWinner(PlayerState))
	{
		if (UCTAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTAudioSubsystem>() : nullptr)
		{
			Audio->PlaySfx(ECTSfx::Victory);
		}
	}

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

// ESC 메뉴

bool ACTPlayerController::CanOpenPauseMenu() const
{
	return true;
}

bool ACTPlayerController::IsPauseMenuOpen() const
{
	return PauseMenuWidget && PauseMenuWidget->IsInViewport();
}

void ACTPlayerController::HandlePauseKey()
{
	if (!IsLocalController()) { return; }

	// 전화 팝업이 떠 있으면 ESC는 팝업만 닫음
	if (IsPhonePopupOpen())
	{
		ClosePhonePopup();
		return;
	}

	if (IsPauseMenuOpen())
	{
		// 설정이나 확인 창이 떠 있으면 한 단계만 뒤로
		if (!PauseMenuWidget->HandleBack()) { ClosePauseMenu(); }
		return;
	}

	OpenPauseMenu();
}

void ACTPlayerController::OpenPauseMenu()
{
	if (!IsLocalController() || !CanOpenPauseMenu() || IsPauseMenuOpen()) { return; }

	// 결과 화면 중에는 결과 화면의 버튼을 씀
	const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	if (CTGameState && CTGameState->GetPhase() == ECTGamePhase::Finished) { return; }

	if (!PauseMenuWidget && PauseMenuWidgetClass)
	{
		PauseMenuWidget = CreateWidget<UCTPauseMenuWidget>(this, PauseMenuWidgetClass);
		if (PauseMenuWidget)
		{
			PauseMenuWidget->OnCloseRequested.AddUniqueDynamic(this, &ACTPlayerController::ClosePauseMenu);
		}
	}
	if (!PauseMenuWidget) { return; }

	PauseMenuWidget->ResetToMain();
	PauseMenuWidget->AddToViewport(50); // HUD와 결과 화면 위, 로딩 화면 아래

	// 게임은 계속 진행되므로 이동과 시점만 막고 마우스를 보여줌
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ACTPlayerController::ClosePauseMenu()
{
	if (!IsPauseMenuOpen()) { return; }

	PauseMenuWidget->RemoveFromParent();
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	if (ResultWidget && ResultWidget->IsInViewport())
	{
		return; // 결과 화면의 입력 모드를 유지
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

// 전화부스 팝업

bool ACTPlayerController::IsPhonePopupOpen() const
{
	return PhonePopupWidget && PhonePopupWidget->IsInViewport();
}

bool ACTPlayerController::CanKeepPhonePopup() const
{
	const ACTCharacter* CTCharacter = Cast<ACTCharacter>(GetPawn());
	return CTCharacter && ACTPhoneBooth::CanUsePhone(CTCharacter) && ACTPhoneBooth::FindBoothContaining(CTCharacter);
}

void ACTPlayerController::HandlePhoneKey()
{
	if (!IsLocalController()) { return; }

	if (IsPhonePopupOpen())
	{
		ClosePhonePopup();
		return;
	}

	// 유령은 전화를 쓸 수 없으므로 F키는 무작위 시작점으로 이동. 메뉴나 결과 화면이 떠 있으면 무시
	const ACTPlayerState* MyState = GetPlayerState<ACTPlayerState>();
	if (MyState && MyState->IsGhost())
	{
		if (!IsPauseMenuOpen() && !(ResultWidget && ResultWidget->IsInViewport()))
		{
			ServerRequestGhostRelocate();
		}
		return;
	}
	OpenPhonePopup();
}

void ACTPlayerController::ServerRequestGhostRelocate_Implementation()
{
	if (ACTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTGameMode>() : nullptr)
	{
		GameMode->HandleGhostRelocate(GetPlayerState<ACTPlayerState>());
	}
}

void ACTPlayerController::OpenPhonePopup()
{
	// ESC 메뉴나 결과 화면이 떠 있거나, 부스 밖이면 열지 않음
	if (!IsLocalController() || IsPhonePopupOpen() || IsPauseMenuOpen() || !PhonePopupWidgetClass) { return; }
	if (ResultWidget && ResultWidget->IsInViewport()) { return; }
	if (!CanKeepPhonePopup()) { return; }

	// 통화 중이면 다시 열 필요 없음
	const ACTCharacter* CTCharacter = Cast<ACTCharacter>(GetPawn());
	if (CTCharacter && CTCharacter->IsBoothLocked()) { return; }

	if (!PhonePopupWidget)
	{
		PhonePopupWidget = CreateWidget<UCTPhonePopupWidget>(this, PhonePopupWidgetClass);
		if (PhonePopupWidget)
		{
			PhonePopupWidget->OnQuerySelected.AddUniqueDynamic(this, &ACTPlayerController::HandlePhoneQuerySelected);
		}
	}
	if (!PhonePopupWidget) { return; }

	PhonePopupWidget->AddToViewport(40); // HUD 위, ESC 메뉴 아래

	// 게임은 계속 진행되므로 이동과 시점만 막고 마우스를 보여줌
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(PhonePopupWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ACTPlayerController::ClosePhonePopup()
{
	if (!IsPhonePopupOpen()) { return; }

	PhonePopupWidget->RemoveFromParent();
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	if (ResultWidget && ResultWidget->IsInViewport())
	{
		return; // 결과 화면의 입력 모드를 유지
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void ACTPlayerController::HandlePhoneQuerySelected(ECTPhoneQuery Query)
{
	ClosePhonePopup();
	ServerRequestPhoneCall(Query);
}

void ACTPlayerController::ServerRequestPhoneCall_Implementation(ECTPhoneQuery Query)
{
	// 어느 부스든 지금 서 있는 부스에서 처리. 쿨타임과 상태는 부스가 서버에서 다시 검사함
	ACTCharacter* CTCharacter = Cast<ACTCharacter>(GetPawn());
	if (ACTPhoneBooth* Booth = ACTPhoneBooth::FindBoothContaining(CTCharacter))
	{
		Booth->ServerRequestCall(CTCharacter, Query);
	}
}

void ACTPlayerController::RequestForfeit()
{
	ServerForfeit();
}

void ACTPlayerController::ServerForfeit_Implementation()
{
	if (ACTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTGameMode>() : nullptr)
	{
		GameMode->HandleForfeit(GetPlayerState<ACTPlayerState>());
	}
}

void ACTPlayerController::PlayCTCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass || !IsLocalController() || !PlayerCameraManager) { return; }

	// 설정에서 끔이면 생략
	const UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
	if (Settings && !Settings->IsScreenShakeEnabled()) { return; }

	PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
}
