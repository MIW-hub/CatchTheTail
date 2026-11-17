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
#include "../UI/CTTreasureHudWidget.h"
#include "../UI/CTRoundBreakWidget.h"
#include "../UI/CTTreasureResultWidget.h"
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
#include "EngineUtils.h"

ACTPlayerController::ACTPlayerController()
{
	// 기본 ESC 메뉴 (WBP_CTPauseMenuWidget). 컨트롤러 블루프린트에서 바꿀 수 있음
	static ConstructorHelpers::FClassFinder<UCTPauseMenuWidget> PauseMenuFinder(TEXT("/Game/UI/WBP_CTPauseMenuWidget"));
	if (PauseMenuFinder.Succeeded())
	{
		PauseMenuWidgetClass = PauseMenuFinder.Class;
	}

	PhonePopupWidgetClass = UCTPhonePopupWidget::StaticClass();

	// 보물찾기 전용 HUD (WBP). 디자인은 WBP 에서 함
	static ConstructorHelpers::FClassFinder<UCTTreasureHudWidget> TreasureHudFinder(TEXT("/Game/UI/WBP_CTTreasureHudWidget"));
	if (TreasureHudFinder.Succeeded())
	{
		TreasureHudWidgetClass = TreasureHudFinder.Class;
	}

	// 보물찾기 결과 화면 (WBP). 없으면 꼬리잡기 결과 화면을 그대로 씀
	static ConstructorHelpers::FClassFinder<UCTTreasureResultWidget> TreasureResultFinder(TEXT("/Game/UI/WBP_CTTreasureResultWidget"));
	if (TreasureResultFinder.Succeeded())
	{
		TreasureResultWidgetClass = TreasureResultFinder.Class;
	}

	// 라운드 사이 결과 팝업 (WBP). 없으면 팝업 없이 진행됨
	static ConstructorHelpers::FClassFinder<UCTRoundBreakWidget> RoundBreakFinder(TEXT("/Game/UI/WBP_CTRoundBreakWidget"));
	if (RoundBreakFinder.Succeeded())
	{
		RoundBreakWidgetClass = RoundBreakFinder.Class;
	}
}

void ACTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		// 메뉴가 열려 있어도 같은 키로 닫을 수 있게 일시 정지 중에도 받음
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACTPlayerController::HandlePauseKey).bExecuteWhenPaused = true;
		// 전화 / 유령 이동 키는 설정에서 바꿀 수 있어 PlayerTick 에서 현재 키를 읽어 처리
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
	ApplyModeUI();
	GameState->OnPhaseChanged.AddUniqueDynamic(this, &ACTPlayerController::HandlePhaseChanged);
	GameState->OnRoundChanged.AddUniqueDynamic(this, &ACTPlayerController::HandleRoundChanged);

	// 이미 끝난 경기 대응
	HandlePhaseChanged(GameState->GetPhase());
}

void ACTPlayerController::ApplyModeUI()
{
	if (!IsLocalController()) { return; }

	const ACTGameState* GameState = BoundGameState.Get();
	if (!GameState || GameState->GetModeType() != ECTGameModeType::TreasureHunt) { return; } // 꼬리잡기는 기본 HUD 를 그대로 씀

	// 보물찾기: 기본 HUD 를 숨기고 전용 HUD 를 보여줌 (전용 HUD WBP 가 없으면 기본 HUD 유지)
	if (!TreasureHudWidgetClass) { return; }
	if (!TreasureHudWidget)
	{
		TreasureHudWidget = CreateWidget<UCTTreasureHudWidget>(this, TreasureHudWidgetClass);
	}
	if (!TreasureHudWidget) { return; }

	if (!TreasureHudWidget->IsInViewport()) { TreasureHudWidget->AddToViewport(0); }
	if (HUDWidget) { HUDWidget->SetVisibility(ESlateVisibility::Collapsed); }
}

void ACTPlayerController::CTStoreShots(int32 Count)
{
#if !UE_BUILD_SHIPPING
	StoreShotsLeft = Count;
	StoreShotIndex = 0;
	// AI 가 들어오고 경기가 시작된 뒤(약 15초)부터 3초 간격
	GetWorldTimerManager().SetTimer(StoreShotTimer, this, &ACTPlayerController::StoreShotStep, 3.f, true, 15.f);
#endif
}

void ACTPlayerController::StoreShotStep()
{
#if !UE_BUILD_SHIPPING
	if (StoreShotsLeft <= 0)
	{
		GetWorldTimerManager().ClearTimer(StoreShotTimer);
		return;
	}

	ACTCharacter* Me = Cast<ACTCharacter>(GetPawn());
	TArray<ACTCharacter*> Others;
	for (TActorIterator<ACTCharacter> It(GetWorld()); It; ++It)
	{
		if (*It != Me) { Others.Add(*It); }
	}
	if (!Me || Others.IsEmpty()) { return; }

	// 다른 캐릭터 한 명을 골라 그 뒤쪽 가까이에서 그 캐릭터를 보게 함
	ACTCharacter* Target = Others[StoreShotIndex % Others.Num()];
	const FVector Forward = Target->GetActorForwardVector().GetSafeNormal2D();
	Me->SetActorLocation(Target->GetActorLocation() - Forward * 420.f + FVector(0.f, 0.f, 20.f), false, nullptr, ETeleportType::TeleportPhysics);
	SetControlRotation(FRotator(-8.f, Forward.Rotation().Yaw, 0.f));

	const int32 Index = StoreShotIndex++;
	--StoreShotsLeft;
	FTimerHandle ShotHandle;
	GetWorldTimerManager().SetTimer(ShotHandle, FTimerDelegate::CreateWeakLambda(this, [this, Index]()
	{
		ConsoleCommand(FString::Printf(TEXT("HighResShot 1920x1080 filename=StoreShot_%d"), Index));
	}), 1.4f, false);
#endif
}

void ACTPlayerController::CTPreviewTreasureHud()
{
	if (!IsLocalController() || !TreasureHudWidgetClass) { return; }

	if (!bTreasureHudPreview)
	{
		if (!TreasureHudWidget) { TreasureHudWidget = CreateWidget<UCTTreasureHudWidget>(this, TreasureHudWidgetClass); }
		if (!TreasureHudWidget) { return; }

		bTreasureHudPreview = true;
		if (!TreasureHudWidget->IsInViewport()) { TreasureHudWidget->AddToViewport(0); }
		if (HUDWidget)
		{
			SavedHudVisibility = HUDWidget->GetVisibility();
			HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		bTreasureHudPreview = false;
		if (TreasureHudWidget) { TreasureHudWidget->RemoveFromParent(); }
		if (HUDWidget) { HUDWidget->SetVisibility(SavedHudVisibility); }
	}
}

void ACTPlayerController::CTShotAfter(int32 Seconds)
{
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { ConsoleCommand(TEXT("Shot ShowUI")); }), static_cast<float>(FMath::Max(Seconds, 1)), false);
}

void ACTPlayerController::CTPreviewRoundBreak(int32 ScreenshotAfterSeconds)
{
	// 시작 직후에는 화면이 준비되지 않았을 수 있어 잠시 뒤에 보임
	if (!bRoundBreakPreviewStarted)
	{
		bRoundBreakPreviewStarted = true;
		FTimerHandle Delay;
		GetWorldTimerManager().SetTimer(Delay, FTimerDelegate::CreateWeakLambda(this, [this, ScreenshotAfterSeconds]() { CTPreviewRoundBreak(ScreenshotAfterSeconds); }), 12.f, false);
		return;
	}
	if (!IsLocalController() || !RoundBreakWidgetClass) { return; }
	if (!RoundBreakWidget) { RoundBreakWidget = CreateWidget<UCTRoundBreakWidget>(this, RoundBreakWidgetClass); }
	if (!RoundBreakWidget) { return; }

	FCTRoundBreakInfo Info;
	Info.RoundNumber = 1;
	Info.TeamEnergy = { 47, 31 };
	Info.WinsBefore = { 0, 0 };
	Info.WinsAfter = { 1, 0 };
	const TCHAR* Names[] = { TEXT("Chick"), TEXT("Momo"), TEXT("Tori"), TEXT("Pipi"), TEXT("Coco"), TEXT("Lulu"), TEXT("Kiki"), TEXT("Yaya") };
	const int32 Scores[] = { 21, 14, 8, 4, 12, 9, 6, 4 };
	for (int32 Index = 0; Index < 8; ++Index)
	{
		FCTRoundBreakEntry Entry;
		Entry.Name = Names[Index];
		Entry.TeamId = Index / 4;
		Entry.Score = Scores[Index];
		Info.Entries.Add(Entry);
	}
	RoundBreakWidget->Show(Info, 0);
	if (!RoundBreakWidget->IsInViewport()) { RoundBreakWidget->AddToViewport(40); }

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { HideRoundBreakPopup(); }), 8.f, false);
	if (ScreenshotAfterSeconds > 0)
	{
		FTimerHandle ShotHandle;
		GetWorldTimerManager().SetTimer(ShotHandle, FTimerDelegate::CreateWeakLambda(this, [this]() { ConsoleCommand(TEXT("Shot ShowUI Name=RoundBreakPreview")); }), static_cast<float>(ScreenshotAfterSeconds), false);
	}
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

	// 전화 / 유령 이동 키 (설정에서 바꾼 키 우선, 기본 F)
	if (const UGameInstance* PhoneGameInstance = GetGameInstance())
	{
		if (const UCTSettingsSubsystem* KeySettings = PhoneGameInstance->GetSubsystem<UCTSettingsSubsystem>())
		{
			if (WasInputKeyJustPressed(KeySettings->GetCodeKey(CTCodeKeys::Phone))) { HandlePhoneKey(); }
		}
	}

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

void ACTPlayerController::ShowRoundBreakPopup()
{
	const ACTGameState* GameState = BoundGameState.Get();
	if (!IsLocalController() || !RoundBreakWidgetClass || !GameState || !GetWorld() || GetWorld()->bIsTearingDown) { return; }

	if (!RoundBreakWidget) { RoundBreakWidget = CreateWidget<UCTRoundBreakWidget>(this, RoundBreakWidgetClass); }
	if (!RoundBreakWidget) { return; }

	const ACTPlayerState* MyState = GetPlayerState<ACTPlayerState>();
	const int32 MyTeam = (MyState && MyState->GetTeamId() >= 0) ? FMath::Clamp(MyState->GetTeamId(), 0, 1) : 0;
	RoundBreakWidget->Show(GameState->GetRoundBreakInfo(), MyTeam);
	if (!RoundBreakWidget->IsInViewport()) { RoundBreakWidget->AddToViewport(40); }
	UE_LOG(LogTemp, Log, TEXT("[CT-Test] 라운드 결과 팝업 표시: 라운드 %d, 줄 %d개"), GameState->GetRoundBreakInfo().RoundNumber, GameState->GetRoundBreakInfo().Entries.Num());
}

void ACTPlayerController::HideRoundBreakPopup()
{
	if (RoundBreakWidget && RoundBreakWidget->IsInViewport()) { RoundBreakWidget->RemoveFromParent(); }
}

void ACTPlayerController::HandlePhaseChanged(ECTGamePhase NewPhase)
{
	if (NewPhase == ECTGamePhase::RoundBreak) { ShowRoundBreakPopup(); }
	else { HideRoundBreakPopup(); }

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
	if (!GetWorld() || GetWorld()->bIsTearingDown) { return; }

	// 보물찾기는 전용 결과 화면 (A팀 / B팀 / 가운데 승리 팀)
	if (TreasureResultWidgetClass && BoundGameState.IsValid() && BoundGameState->GetModeType() == ECTGameModeType::TreasureHunt && !Cast<UCTTreasureResultWidget>(ResultWidget))
	{
		if (ResultWidget && ResultWidget->IsInViewport()) { ResultWidget->RemoveFromParent(); }
		ResultWidget = CreateWidget<UCTTreasureResultWidget>(this, TreasureResultWidgetClass);
		UE_LOG(LogTemp, Log, TEXT("[CT-Test] 보물찾기 결과 화면 표시"));
	}
	if (!ResultWidget) { return; }

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
