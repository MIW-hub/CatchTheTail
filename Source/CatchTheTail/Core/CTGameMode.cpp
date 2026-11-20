// Copyright Moon Party Games. All Rights Reserved.
#include "CTGameMode.h"
#include "CTGameInstance.h"
#include "CTGameState.h"
#include "CTGameRule.h"
#include "CTRule_TailChase.h"
#include "CTRule_TreasureHunt.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "../Player/CTPlayerState.h"
#include "../Player/CTPlayerController.h"
#include "../Player/CTAIController.h"
#include "CatchTheTail/Player/CTCharacter.h"
#include "../Player/CTGrappleComponent.h"
#include "../World/CTTailPickup.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"
#include "../World/CTThrowable.h"
#include "../World/CTBroadcastCamera.h"

#if !UE_BUILD_SHIPPING
// 개발용: 1이면 보물찾기 경기에서 던지기 시험 시나리오를 자동으로 실행하고 [CT-Test] 로그를 남김
static TAutoConsoleVariable<float> CVarTestMatchSeconds(TEXT("ct.TestMatchSeconds"), 0.f, TEXT("0보다 크면 경기 길이를 이 초로 줄임 (개발용 시험)"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarThrowSyncTest(TEXT("ct.ThrowSyncTest"), 0, TEXT("던지기 동기화 자동 시험 (개발용)"), ECVF_Default);
#endif


ACTGameMode::ACTGameMode()
{
	// 기본 클래스. BP GameMode에서 교체 가능
	DefaultPawnClass = ACTCharacter::StaticClass();
	PlayerControllerClass = ACTPlayerController::StaticClass();
	GameStateClass = ACTGameState::StaticClass();
	PlayerStateClass = ACTPlayerState::StaticClass();

	// 로비 / 게임 이동은 Seamless Travel (PlayerState 유지)
	bUseSeamlessTravel = true;
}

void ACTGameMode::InitGameState()
{
	Super::InitGameState();

	// 로비 없이 시작한 테스트는 BP 값 사용
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	ECTGameModeType Mode = GameInstance && GameInstance->HasStoredMatchSettings() ? GameInstance->GetStoredMatchSettings().GetMode() : ECTGameModeType::TailChase;
#if !UE_BUILD_SHIPPING
	// 개발용: 로비 없이 시작할 때 명령줄 -CTMode=1 로 보물찾기를 켤 수 있음 (출시 빌드에서는 제외)
	int32 ModeOverride = INDEX_NONE;
	if (FParse::Value(FCommandLine::Get(), TEXT("CTMode="), ModeOverride) && ModeOverride >= 0 && ModeOverride < static_cast<int32>(ECTGameModeType::Count))
	{
		Mode = static_cast<ECTGameModeType>(ModeOverride);
	}
#endif
	CreateRule(Mode);
	if (!GameInstance || !GameInstance->HasStoredMatchSettings()) { return; }

	const FCTMatchSettings& Settings = GameInstance->GetStoredMatchSettings();
	GameDurationSeconds = Settings.GameMinutes * 60.f;
	RoundDurationSeconds = static_cast<float>(Settings.RoundSeconds);
#if !UE_BUILD_SHIPPING
	// 개발용 시험: 경기 길이를 짧게 줄여 한 판을 끝까지 돌려 봄 (Tools/CTEosMatchTest.ps1)
	if (CVarTestMatchSeconds.GetValueOnGameThread() > 0.f)
	{
		GameDurationSeconds = CVarTestMatchSeconds.GetValueOnGameThread();
		RoundDurationSeconds = FMath::Max(10.f, GameDurationSeconds / 3.f);
	}
#endif
	InitialTailCount = Settings.InitialTails;
	PhoneCooldownSeconds = static_cast<float>(Settings.PhoneUses);

	UE_LOG(LogTemp, Log, TEXT("[CT] 경기 설정 적용: %d분, 라운드 %d초, 시작 꼬리 %d, 전화 쿨타임 %d초, 부쉬 %s"),
		Settings.GameMinutes, Settings.RoundSeconds, Settings.InitialTails, Settings.PhoneUses, Settings.IsBushEnabled() ? TEXT("켜기") : TEXT("끄기"));
}

void ACTGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty()) { return; }

	// 게임/결과 중 입장 불가
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	const ECTGamePhase Phase = CTGameState ? CTGameState->GetPhase() : ECTGamePhase::WaitingForPlayers;
	if (Phase == ECTGamePhase::Playing || Phase == ECTGamePhase::Finished)
	{
		ErrorMessage = CTJoinReject::GameInProgress;
		UE_LOG(LogTemp, Log, TEXT("[CT] 입장 거절: 게임 진행 중 (%s)"), *Address);
	}
}

void ACTGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	InitPlayerForGame(NewPlayer);
}

void ACTGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);
	InitPlayerForGame(Cast<APlayerController>(C));
}

void ACTGameMode::InitPlayerForGame(APlayerController* NewPlayer)
{
	// 접속 즉시 시작 꼬리 수 부여 (시작 전에도 표시)
	if (ACTPlayerState* NewPlayerState = NewPlayer ? NewPlayer->GetPlayerState<ACTPlayerState>() : nullptr)
	{
		NewPlayerState->SetTailCount(InitialTailCount);

		// listen 서버 로컬 플레이어가 방장
		NewPlayerState->SetIsHost(NewPlayer->IsLocalController());
	}

#if !UE_BUILD_SHIPPING
	// 개발용 중계 촬영: AI 들만 경기를 하고 로컬 플레이어는 관전자로 카메라만 잡음 (Tools/CTRenderBroadcast.ps1)
	int32 BroadcastStyle = 0;
	if (!bBroadcastSetup && NewPlayer && NewPlayer->IsLocalController() && FParse::Value(FCommandLine::Get(), TEXT("CTBroadcast="), BroadcastStyle) && BroadcastStyle > 0)
	{
		bBroadcastSetup = true;
		if (APawn* OwnPawn = NewPlayer->GetPawn()) { OwnPawn->Destroy(); }
		if (APlayerState* PS = NewPlayer->PlayerState) { PS->SetIsSpectator(true); PS->SetIsOnlyASpectator(true); }
		if (ACTBroadcastCamera* Camera = GetWorld()->SpawnActor<ACTBroadcastCamera>(FVector(0.f, 0.f, 2000.f), FRotator::ZeroRotator))
		{
			Camera->Style = BroadcastStyle >= 2 ? 1 : 0;
			{ float S = 1.f; FParse::Value(FCommandLine::Get(), TEXT("CTBroadcastScale="), S); Camera->AerialScale = S; }
			Camera->bHideHud = !FParse::Param(FCommandLine::Get(), TEXT("CTBroadcastHud"));
			NewPlayer->SetViewTarget(Camera);
		}
		UE_LOG(LogTemp, Log, TEXT("[CT] 중계 촬영 시작: 스타일 %d"), BroadcastStyle);
	}
#endif

	// 테스트용 자동 시작. 접속마다 타이머 재설정
	if (bAutoStartForTesting)
	{
		const ACTGameState* CTGameState = GetGameState<ACTGameState>();
		const ECTGamePhase Phase = CTGameState ? CTGameState->GetPhase() : ECTGamePhase::Finished;
		if (Phase == ECTGamePhase::WaitingForPlayers || Phase == ECTGamePhase::Countdown)
		{
			BeginStartCountdown();
		}
	}
}

void ACTGameMode::Logout(AController* Exiting)
{
	// Super 이후 정리될 수 있어 주소만 보관
	const ACTPlayerState* LeavingState = Exiting ? Exiting->GetPlayerState<ACTPlayerState>() : nullptr;

	Super::Logout(Exiting);

	HandlePlayerLeftDuringGame(LeavingState);
}

void ACTGameMode::HandlePlayerLeftDuringGame(const ACTPlayerState* LeavingPlayer)
{
	// PIE 종료, 맵 이동으로 월드가 정리되는 중의 로그아웃은 게임 종료로 처리하지 않음 (결과 위젯 생성 중 크래시 방지)
	if (!GetWorld() || GetWorld()->bIsTearingDown) { return; }

	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }

	TArray<ACTPlayerState*> Remaining;
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		if (PlayerState != LeavingPlayer) { Remaining.Add(PlayerState); }
	}

	// 혼자 남으면 게임을 이어갈 수 없으므로 종료
	if (Remaining.Num() < 2)
	{
		UE_LOG(LogTemp, Log, TEXT("[CT] 플레이어 이탈로 남은 인원 %d명: 게임 종료"), Remaining.Num());
		EndGame();
		return;
	}

	// 목표를 잃지 않게 남은 색끼리 재연결 (목표 색 규칙을 쓰는 모드만)
	if (!bTargetFromActiveColorsOnly || !GetRule()->UsesTargetColors()) { return; }

	TArray<uint8> UsedColors;
	for (const ACTPlayerState* PlayerState : Remaining)
	{
		if (PlayerState->GetAssignedColor() != ECTColor::None)
		{
			UsedColors.Add(static_cast<uint8>(PlayerState->GetAssignedColor()));
		}
	}
	UsedColors.Sort();

	for (ACTPlayerState* PlayerState : Remaining)
	{
		const ECTColor MyColor = PlayerState->GetAssignedColor();
		if (MyColor == ECTColor::None) { continue; }

		ECTColor Target = ECTColor::None;
		if (UsedColors.Num() > 1)
		{
			const int32 MyIndex = UsedColors.IndexOfByKey(static_cast<uint8>(MyColor));
			Target = static_cast<ECTColor>(UsedColors[(MyIndex + 1) % UsedColors.Num()]);
		}
		PlayerState->SetColors(MyColor, Target);
	}
}

// 게임 시작 / 종료

void ACTGameMode::BeginStartCountdown()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState) { return; }

	float Delay = FMath::Max(GameStartCountdownSeconds, 0.1f);
#if !UE_BUILD_SHIPPING
	// 개발용 시험: 두 번째 클라이언트가 접속할 때까지 시작을 늦춤 (접속할 때마다 다시 시작됨)
	if (CVarThrowSyncTest.GetValueOnGameThread() > 0) { Delay = FMath::Max(Delay, 30.f); }
#endif

	// 시작 시각 먼저, 단계 나중. 클라 남은 시간 정확
	CTGameState->SetStartCountdown(CTGameState->GetServerWorldTimeSeconds() + Delay);
	CTGameState->SetPhase(ECTGamePhase::Countdown);

	// 카운트다운 동안 이미 시작 위치에 서 있게 함 (게임이 시작되는 순간 위치가 바뀌어 보이지 않게)
	GetRule()->OnCountdownStart();

	GetWorldTimerManager().SetTimer(AutoStartTimerHandle, this, &ACTGameMode::StartGame, Delay, false);
}

void ACTGameMode::StartGame()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() == ECTGamePhase::Playing)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AutoStartTimerHandle);
	CTGameState->SetStartCountdown(0.0);

	// 이전 판의 로비 복귀 예약 취소
	GetWorldTimerManager().ClearTimer(ReturnToLobbyTimerHandle);
	CTGameState->SetReturnToLobbyTime(0.0);

	// 이전 결과 초기화
	CTGameState->SetGameResult({}, false);

	// 이전 판의 기권 기록 해제 (기권한 플레이어는 CollectPlayers에서 빠지므로 따로 처리)
	for (APlayerState* PlayerState : CTGameState->PlayerArray)
	{
		if (ACTPlayerState* CTPlayerState = Cast<ACTPlayerState>(PlayerState)) { CTPlayerState->SetForfeited(false); }
	}

	// 모두 입장한 뒤라 사람 수가 정확함. AI는 여기서 채움
	SpawnAIPlayers();

	// 플레이어 초기화 (모드 규칙)
	GetRule()->OnMatchStart();

	const double Now = CTGameState->GetServerWorldTimeSeconds();
	const float MatchDurationSeconds = GetRule()->GetGameDurationSeconds(GameDurationSeconds);
	CTGameState->SetGameInfo(Now + MatchDurationSeconds, RoundDurationSeconds, CountdownSeconds);
	CTGameState->SetForfeitUnlockTime(Now + MatchDurationSeconds * ForfeitAvailableRatio);
	CTGameState->SetRoundInfo(0, Now);
	CTGameState->SetPhase(ECTGamePhase::Playing);

	OnGameStarted();
	StartNewRound();
#if !UE_BUILD_SHIPPING
	if (CVarThrowSyncTest.GetValueOnGameThread() > 0)
	{
		DevTestStep = 0;
		GetWorldTimerManager().SetTimer(DevTestTimerHandle, this, &ACTGameMode::DevThrowSyncTest, 0.5f, true);
	}
#endif
	RefreshAllChaseBuffs();

	GetWorldTimerManager().SetTimer(FlowTimerHandle, this, &ACTGameMode::TickGameFlow, 0.2f, true);

	UE_LOG(LogTemp, Log, TEXT("[CT] 게임 시작: 전체 %.0f초, 라운드 %.0f초"), MatchDurationSeconds, RoundDurationSeconds);
}

#if !UE_BUILD_SHIPPING
void ACTGameMode::DevThrowSyncTest()
{
	UWorld* World = GetWorld();
	if (!World) { return; }
	const double Now = World->GetTimeSeconds();

	// 사람 둘의 캐릭터 찾기
	ACTCharacter* HostCharacter = nullptr;
	ACTCharacter* ClientCharacter = nullptr;
	ACTPlayerState* ClientState = nullptr;
	for (ACTPlayerState* Player : CollectPlayers())
	{
		if (Player->IsABot()) { continue; }
		ACTCharacter* Character = Cast<ACTCharacter>(Player->GetPawn());
		if (!Character) { continue; }
		if (Player->IsHost()) { HostCharacter = Character; } else { ClientCharacter = Character; ClientState = Player; }
	}

	auto NextStep = [&](int32 Step) { DevTestStep = Step; DevTestStepTime = Now; };

	switch (DevTestStep)
	{
	case 0: // 경기가 시작되고 두 사람이 모두 있으면 8초 뒤 시작
		if (!HostCharacter || !ClientCharacter) { return; }
		if (DevTestStepTime <= 0.0) { DevTestStepTime = Now; }
		if (Now - DevTestStepTime < 8.0) { return; }
		{
			ACTThrowable* Nearest = nullptr;
			float NearestDistance = TNumericLimits<float>::Max();
			for (TActorIterator<ACTThrowable> It(World); It; ++It)
			{
				if (!It->CanBePickedUp()) { continue; }
				const float Distance = FVector::Dist2D(It->GetActorLocation(), HostCharacter->GetActorLocation());
				if (Distance < NearestDistance) { Nearest = *It; NearestDistance = Distance; }
			}
			if (!Nearest) { UE_LOG(LogTemp, Log, TEXT("[CT-Test] 서버: 오브젝트가 없음")); NextStep(99); return; }

			// 클라이언트에게 에너지 10을 주고, 오브젝트를 방장 바로 앞 1.5m에 가져다 놓음 (맵 모양과 상관없이 같은 조건으로 시험)
			ClientState->SetEnergy(10);
			const FVector Forward = HostCharacter->GetActorForwardVector().GetSafeNormal2D();
			Nearest->SetActorLocation(HostCharacter->GetActorLocation() + Forward * 150.f + FVector(0.f, 0.f, 20.f), false, nullptr, ETeleportType::TeleportPhysics);
			UE_LOG(LogTemp, Log, TEXT("[CT-Test] 서버: 오브젝트를 방장 앞에 놓음, 클라 에너지 10"));
			NextStep(1);
		}
		break;
	case 1: // 집기
		if (Now - DevTestStepTime < 1.0) { return; }
		if (HostCharacter) { HostCharacter->AIGrab(); }
		NextStep(2);
		break;
	case 2: // 들었는지 확인
		if (Now - DevTestStepTime < 1.5) { return; }
		if (!HostCharacter || !HostCharacter->IsHoldingThrowable())
		{
			UE_LOG(LogTemp, Log, TEXT("[CT-Test] 서버: 방장이 오브젝트를 못 들었음 (실패)"));
			NextStep(99);
			return;
		}
		UE_LOG(LogTemp, Log, TEXT("[CT-Test] 서버: 방장이 오브젝트를 들었음"));
		NextStep(3);
		break;
	case 3: // 방장 주변에서 시야가 트이고 겹치지 않는 자리를 찾아 클라이언트를 세움
		if (Now - DevTestStepTime < 0.5) { return; }
		{
			const FVector HostLocation = HostCharacter->GetActorLocation();
			FCollisionQueryParams Params(SCENE_QUERY_STAT(CTDevThrowTest), false);
			Params.AddIgnoredActor(HostCharacter);
			Params.AddIgnoredActor(ClientCharacter);
			bool bPlaced = false;
			for (const float Distance : { 500.f, 400.f, 300.f })
			{
				for (int32 Index = 0; Index < 8 && !bPlaced; ++Index)
				{
					const FVector Direction = FVector(1.f, 0.f, 0.f).RotateAngleAxis(Index * 45.f, FVector::UpVector);
					const FVector Point = HostLocation + Direction * Distance;
					FHitResult Hit;
					if (World->LineTraceSingleByChannel(Hit, HostLocation, Point, ECC_Visibility, Params)) { continue; } // 가로막힘
					if (World->OverlapBlockingTestByChannel(Point, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(40.f, 90.f), Params)) { continue; }
					if (!World->LineTraceSingleByChannel(Hit, Point, Point - FVector(0.f, 0.f, 200.f), ECC_Visibility, Params)) { continue; } // 바닥 없음
					ClientCharacter->GetCharacterMovement()->StopMovementImmediately();
					ClientCharacter->TeleportTo(Point, FRotator::ZeroRotator);
					UE_LOG(LogTemp, Log, TEXT("[CT-Test] 서버: 클라이언트를 방장 %d도 방향 %.0fcm에 세움"), Index * 45, Distance);
					bPlaced = true;
				}
				if (bPlaced) { break; }
			}
			if (!bPlaced) { UE_LOG(LogTemp, Log, TEXT("[CT-Test] 서버: 클라이언트를 세울 자리가 없음 (실패)")); NextStep(99); return; }
		}
		NextStep(4);
		break;
	case 4: // 클라이언트를 바라보고 던지기
		if (Now - DevTestStepTime < 1.0) { return; }
		if (HostCharacter && ClientCharacter)
		{
			const FVector ToClient = (ClientCharacter->GetActorLocation() - HostCharacter->GetActorLocation()).GetSafeNormal2D();
			HostCharacter->SetActorRotation(ToClient.Rotation());
			HostCharacter->AIThrow();
		}
		NextStep(5);
		break;
	case 5: // 결과 확인
		if (Now - DevTestStepTime < 4.0) { return; }
		UE_LOG(LogTemp, Log, TEXT("[CT-Test] 서버: 시험 끝, 클라 에너지 %d"), ClientState ? ClientState->GetEnergy() : -1);
		NextStep(99);
		break;
	default:
		GetWorldTimerManager().ClearTimer(DevTestTimerHandle);
		break;
	}
}
#endif

void ACTGameMode::SpawnAIPlayers()
{
	UWorld* World = GetWorld();
	if (bAIPlayersSpawned || !World) { return; }
	bAIPlayersSpawned = true;

	// 로비 설정 우선, 없으면 테스트용 값
	int32 WantedCount = DebugAIPlayerCount;
	int32 MaxPlayers = CTColorCount;
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	if (GameInstance && GameInstance->HasStoredMatchSettings())
	{
		const FCTMatchSettings& Settings = GameInstance->GetStoredMatchSettings();
		WantedCount = Settings.AiPlayers;
		MaxPlayers = FMath::Min(Settings.MaxPlayers, CTColorCount);
	}

#if !UE_BUILD_SHIPPING
	// 개발용: 명령줄 -CTAI=8 로 AI 수 지정 (로비 설정보다 우선)
	int32 AiOverride = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("CTAI="), AiOverride)) { WantedCount = AiOverride; MaxPlayers = CTColorCount; }
#endif

	// 사람 수: 관전자와 AI가 아닌 플레이어
	int32 HumanCount = 0;
	for (APlayerState* State : GameState->PlayerArray)
	{
		if (State && !State->IsOnlyASpectator() && !State->IsABot()) { ++HumanCount; }
	}

	const int32 Count = FMath::Clamp(WantedCount, 0, FMath::Max(MaxPlayers - HumanCount, 0));
	if (Count <= 0) { return; }

	const TSubclassOf<ACTAIController> ControllerClass = AIControllerClass ? AIControllerClass : TSubclassOf<ACTAIController>(ACTAIController::StaticClass());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACTAIController* AIController = World->SpawnActor<ACTAIController>(ControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (!AIController) { continue; }

		if (APlayerState* AIState = AIController->PlayerState)
		{
			AIState->SetIsABot(true);
			AIState->SetPlayerName(FString::Printf(TEXT("AI %d"), Index + 1));
		}

		// 시작 지점에서 폰 생성 후 조종
		RestartPlayer(AIController);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] AI 플레이어 %d명 추가 (사람 %d명, 요청 %d명)"), Count, HumanCount, WantedCount);
}

void ACTGameMode::EndGame()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(FlowTimerHandle);

	// 마지막 정산 (모드 규칙)
	GetRule()->OnGameEnding();

	// 승자 계산 (모드 규칙)
	TArray<ACTPlayerState*> Winners;
	bool bIsTie = false;
	GetRule()->ComputeResult(Winners, bIsTie);
	UE_LOG(LogTemp, Log, TEXT("[CT] 게임 종료: 승자 %d명, 동점=%s"), Winners.Num(), bIsTie ? TEXT("예") : TEXT("아니오"));

	// 결과 먼저 저장, 단계 나중. 결과 화면에 승자 동시 도착
	CTGameState->SetGameResult(Winners, bIsTie);
	CTGameState->SetPhase(ECTGamePhase::Finished);
	RefreshAllChaseBuffs(); // 추격 버프 해제

	// 일정 시간 뒤 대기 로비로 자동 복귀
	if (ReturnToLobbyDelaySeconds > 0.f)
	{
		CTGameState->SetReturnToLobbyTime(CTGameState->GetServerWorldTimeSeconds() + ReturnToLobbyDelaySeconds);
		GetWorldTimerManager().SetTimer(ReturnToLobbyTimerHandle, this, &ACTGameMode::ReturnToLobby, ReturnToLobbyDelaySeconds, false);
	}

	OnGameFinished(Winners, bIsTie);
}

bool ACTGameMode::RequestReturnToLobby(APlayerController* Requester)
{
	// listen 서버의 로컬 플레이어(방장)만 가능
	if (!Requester || !Requester->IsLocalController()) { return false; }

	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Finished) { return false; }

	ReturnToLobby();
	return true;
}

void ACTGameMode::ReturnToLobby()
{
	GetWorldTimerManager().ClearTimer(ReturnToLobbyTimerHandle);

	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	UWorld* World = GetWorld();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Finished || !GameInstance || !World) { return; }

	const FName LobbyMap = GameInstance->GetLobbyMapName();
	if (LobbyMap.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 로비 맵이 지정되지 않아 로비로 돌아갈 수 없습니다."));
		return;
	}

	// ServerTravel 은 짧은 맵 이름(L_Lobby)을 찾지 못해 조용히 멈춤. 전체 패키지 경로(/Game/Levels/L_Lobby)로 바꿔서 이동
	FString TravelName = LobbyMap.ToString();
	if (!TravelName.StartsWith(TEXT("/")))
	{
		FString LongPackageName;
		if (FPackageName::SearchForPackageOnDisk(TravelName, &LongPackageName) && !LongPackageName.IsEmpty())
		{
			TravelName = LongPackageName;
		}
		else
		{
			TravelName = TEXT("/Game/Levels/") + TravelName;
		}
	}

	// AI 플레이어는 게임에서만 존재함. 이동 전에 정리하지 않으면 로비 인원 목록에 이전 AI가 남아 쌓임
	TArray<ACTAIController*> BotControllers;
	for (TActorIterator<ACTAIController> It(World); It; ++It) { BotControllers.Add(*It); }
	for (ACTAIController* Bot : BotControllers)
	{
		if (APawn* BotPawn = Bot->GetPawn()) { BotPawn->Destroy(); }
		if (APlayerState* BotState = Bot->PlayerState) { BotState->Destroy(); }
		Bot->Destroy();
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 대기 로비로 복귀: %s (AI %d명 정리)"), *TravelName, BotControllers.Num());
	World->ServerTravel(TravelName);
}

// 라운드 진행

void ACTGameMode::TickGameFlow()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing)
	{
		GetWorldTimerManager().ClearTimer(FlowTimerHandle);
		return;
	}

	const double Now = CTGameState->GetServerWorldTimeSeconds();

	if (Now >= CTGameState->GetGameEndServerTime())
	{
		EndGame();
	}
	else if (Now >= CTGameState->GetRoundEndServerTime())
	{
		// 라운드 정산 (모드 규칙). 규칙이 경기를 끝내라고 하면 종료
		if (GetRule()->OnRoundTimeUp(CTGameState->GetRoundNumber()))
		{
			EndGame();
		}
		else if (const float BreakSeconds = GetRule()->GetRoundBreakSeconds(); BreakSeconds > 0.f)
		{
			BeginRoundBreak(CTGameState->GetRoundNumber(), BreakSeconds);
		}
		else
		{
			StartNewRound();
		}
	}
}

void ACTGameMode::BeginRoundBreak(int32 FinishedRound, float Seconds)
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState) { return; }

	// 정산 결과를 팝업용으로 정리한 뒤 쉬는 시간 단계로 넘어감 (이 단계에서는 이동, 잡기, 에너지 먹기가 모두 멈춤)
	FCTRoundBreakInfo Info;
	GetRule()->FillRoundBreakInfo(Info, FinishedRound);

	const double Now = CTGameState->GetServerWorldTimeSeconds();
	CTGameState->ExtendGameTimes(Seconds);
	CTGameState->SetRoundBreak(Info, Now + Seconds);
	CTGameState->SetPhase(ECTGamePhase::RoundBreak);

	// 팝업이 뜨고 나서 잠시 뒤에 다음 라운드를 준비 (플레이어 이동, 에너지와 오브젝트 재배치)
	GetWorldTimerManager().SetTimer(RoundBreakPrepareHandle, this, &ACTGameMode::PrepareBreakRound, FMath::Min(1.0f, Seconds * 0.25f), false);
	GetWorldTimerManager().SetTimer(RoundBreakEndHandle, this, &ACTGameMode::EndRoundBreak, Seconds, false);
	UE_LOG(LogTemp, Log, TEXT("[CT] 라운드 %d 종료, 쉬는 시간 %.1f초 (결과 팝업)"), FinishedRound, Seconds);
}

void ACTGameMode::PrepareBreakRound()
{
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::RoundBreak) { return; }
	GetRule()->PrepareNextRound(CTGameState->GetRoundNumber() + 1);
}

void ACTGameMode::EndRoundBreak()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::RoundBreak) { return; }

	// 준비가 아직 안 됐다면(타이머 지연) 여기서 보장됨: 규칙의 OnRoundStart 가 준비되지 않았으면 직접 준비함
	GetWorldTimerManager().ClearTimer(RoundBreakPrepareHandle);
	CTGameState->SetPhase(ECTGamePhase::Playing);
	StartNewRound();
	GetWorldTimerManager().SetTimer(FlowTimerHandle, this, &ACTGameMode::TickGameFlow, 0.2f, true);
}

void ACTGameMode::StartNewRound()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState) { return; }

	const double Now = CTGameState->GetServerWorldTimeSeconds();
	const int32 NewRoundNumber = CTGameState->GetRoundNumber() + 1;

	// 꼬리 아이템 정리, 유령 부활, 꼬리 보충, 색과 목표 배정 (모드 규칙)
	GetRule()->OnRoundStart(NewRoundNumber);

	// 3) 라운드 종료 시각. 게임 종료 시각 초과 금지
	const double RoundEnd = FMath::Min(Now + RoundDurationSeconds, CTGameState->GetGameEndServerTime());
	CTGameState->SetRoundInfo(NewRoundNumber, RoundEnd);

	UE_LOG(LogTemp, Log, TEXT("[CT] 라운드 %d 시작"), NewRoundNumber);
	OnRoundStarted(NewRoundNumber);
}

// 꼬리 잡기

bool ACTGameMode::HandleTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	// 잡기 처리는 모드 규칙이 함
	return GetRule()->HandleGrab(Grabber, Victim);
}

bool ACTGameMode::ModeUsesTailPickups()
{
	return GetRule()->UsesTailPickups();
}

int32 ACTGameMode::HandleHazardHit(ACTCharacter* Victim, const FVector& Direction)
{
	return GetRule()->OnHazardHit(Victim, Direction);
}

bool ACTGameMode::HandleEnergyPickup(ACTCharacter* Picker, int32 Amount)
{
	// 에너지 처리는 모드 규칙이 함
	return GetRule()->OnEnergyPicked(Picker, Amount);
}

void ACTGameMode::HandleThrowableHit(ACTCharacter* Thrower, ACTCharacter* Victim, const FVector& Velocity)
{
	// 맞았을 때의 처리는 모드 규칙이 함
	GetRule()->OnThrowableHit(Thrower, Victim, Velocity);
}

bool ACTGameMode::FinalizeTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	// 점수 변경과 유령 처리는 모드 규칙이 함
	return GetRule()->FinalizeGrab(Grabber, Victim);
}

// 게임 모드 규칙 객체

UCTGameRule* ACTGameMode::GetRule()
{
	// InitGameState 전에 불릴 일은 없지만, 없으면 꼬리잡기로 만들어 둠
	if (!Rule) { CreateRule(ECTGameModeType::TailChase); }
	return Rule;
}

void ACTGameMode::CreateRule(ECTGameModeType Mode)
{
	if (Mode == ECTGameModeType::TreasureHunt)
	{
		Rule = NewObject<UCTRule_TreasureHunt>(this);
	}
	else
	{
		Rule = NewObject<UCTRule_TailChase>(this);
	}
	Rule->Init(this);

	// 클라이언트 화면이 모드를 알 수 있게 GameState 에 기록
	if (ACTGameState* CTGameState = GetGameState<ACTGameState>())
	{
		CTGameState->SetModeType(Rule->GetModeType());
	}
	UE_LOG(LogTemp, Log, TEXT("[CT] 게임 모드 규칙 생성: %s"), Mode == ECTGameModeType::TreasureHunt ? TEXT("보물찾기") : TEXT("꼬리잡기"));
}

bool ACTGameMode::HandleForfeit(ACTPlayerState* Forfeiter)
{
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || !Forfeiter || Forfeiter->IsOnlyASpectator()) { return false; }

	// 경기 중, 기권이 열린 뒤, 한 번만
	if (!CTGameState->CanForfeitNow() || Forfeiter->IsForfeited()) { return false; }

	// 꼬리를 모두 잃고 유령으로 남음. 이후 CollectPlayers에서 빠져 색 배정, 순위, 승자 계산에서 제외됨
	Forfeiter->SetForfeited(true);
	Forfeiter->SetTailCount(0);
	Forfeiter->SetColors(ECTColor::None, ECTColor::None);
	Forfeiter->SetIsGhost(true);

	UE_LOG(LogTemp, Log, TEXT("[CT] %s 기권"), *Forfeiter->GetPlayerName());

	RefreshAllChaseBuffs();

	// 이탈과 같이 처리: 남은 사람끼리 목표 색 재연결, 혼자 남으면 종료
	HandlePlayerLeftDuringGame(Forfeiter);
	return true;
}

bool ACTGameMode::HandleGhostRelocate(ACTPlayerState* Ghost)
{
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }

	// 유령이고 기권하지 않은 플레이어만
	if (!Ghost || !Ghost->IsGhost() || Ghost->IsForfeited() || Ghost->IsOnlyASpectator()) { return false; }

	ACTCharacter* Character = Cast<ACTCharacter>(Ghost->GetPawn());
	UWorld* World = GetWorld();
	if (!Character || !World) { return false; }

	// 유령 상태마다 한 번만
	if (Ghost->HasUsedGhostRelocate())
	{
		Character->ClientShowNotice(NSLOCTEXT("CT", "Ghost_RelocateUsed", "이동은 유령 상태에서 한 번만 쓸 수 있어요."), 3.f);
		return false;
	}

	// 다른 캐릭터가 서 있지 않은 시작점을 우선 무작위로 고름, 모두 사용 중이면 아무 곳이나
	TArray<APlayerStart*> Starts;
	TArray<APlayerStart*> FreeStarts;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* Start = *It;
		Starts.Add(Start);

		bool bOccupied = false;
		for (TActorIterator<ACTCharacter> CharIt(World); CharIt; ++CharIt)
		{
			if (*CharIt != Character && FVector::DistSquared(CharIt->GetActorLocation(), Start->GetActorLocation()) < FMath::Square(GhostRelocateClearRadius))
			{
				bOccupied = true;
				break;
			}
		}
		if (!bOccupied) { FreeStarts.Add(Start); }
	}

	const TArray<APlayerStart*>& Candidates = FreeStarts.Num() > 0 ? FreeStarts : Starts;
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] PlayerStart 가 없어 유령 부활 위치를 고를 수 없습니다."));
		return false;
	}

	const APlayerStart* Spot = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	Character->TeleportTo(Spot->GetActorLocation(), Spot->GetActorRotation(), false, true);
	Character->GetCharacterMovement()->StopMovementImmediately();
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(Spot->GetActorRotation());
	}

	// 유령 상태는 그대로, 부활은 다음 라운드 시작 때 (이동한 자리에서)
	Ghost->MarkGhostRelocateUsed();
	UE_LOG(LogTemp, Log, TEXT("[CT] %s 유령 이동: %s"), *Ghost->GetPlayerName(), *Spot->GetName());
	return true;
}

// 도우미

void ACTGameMode::RefreshAllChaseBuffs()
{
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		if (ACTCharacter* CTCharacter = Cast<ACTCharacter>(PlayerState->GetPawn()))
		{
			CTCharacter->RefreshChaseBuff();
		}
	}
}

TArray<ACTPlayerState*> ACTGameMode::CollectPlayers() const
{
	TArray<ACTPlayerState*> Result;

	if (GameState)
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			ACTPlayerState* CTPlayerState = Cast<ACTPlayerState>(PlayerState);
			if (CTPlayerState && !CTPlayerState->IsOnlyASpectator() && !CTPlayerState->IsForfeited())
			{
				Result.Add(CTPlayerState);
			}
		}
	}

	return Result;
}
