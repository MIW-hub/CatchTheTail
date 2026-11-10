// Copyright Moon Party Games. All Rights Reserved.
#include "CTGameMode.h"
#include "CTGameInstance.h"
#include "CTGameState.h"
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
	if (!GameInstance || !GameInstance->HasStoredMatchSettings()) { return; }

	const FCTMatchSettings& Settings = GameInstance->GetStoredMatchSettings();
	GameDurationSeconds = Settings.GameMinutes * 60.f;
	RoundDurationSeconds = static_cast<float>(Settings.RoundSeconds);
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

	// 목표를 잃지 않게 남은 색끼리 재연결
	if (!bTargetFromActiveColorsOnly) { return; }

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

	const float Delay = FMath::Max(GameStartCountdownSeconds, 0.1f);

	// 시작 시각 먼저, 단계 나중. 클라 남은 시간 정확
	CTGameState->SetStartCountdown(CTGameState->GetServerWorldTimeSeconds() + Delay);
	CTGameState->SetPhase(ECTGamePhase::Countdown);

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

	// 플레이어 초기화
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		PlayerState->SetColors(ECTColor::None, ECTColor::None); // 직전 색 없음
		PlayerState->SetTailCount(InitialTailCount);
		PlayerState->SetIsGhost(false);
		PlayerState->SetPhoneReadyServerTime(0.0); // 시작하자마자 한 번은 걸 수 있음
	}

	const double Now = CTGameState->GetServerWorldTimeSeconds();
	CTGameState->SetGameInfo(Now + GameDurationSeconds, RoundDurationSeconds, CountdownSeconds);
	CTGameState->SetForfeitUnlockTime(Now + GameDurationSeconds * ForfeitAvailableRatio);
	CTGameState->SetRoundInfo(0, Now);
	CTGameState->SetPhase(ECTGamePhase::Playing);

	OnGameStarted();
	StartNewRound();
	RefreshAllChaseBuffs();

	GetWorldTimerManager().SetTimer(FlowTimerHandle, this, &ACTGameMode::TickGameFlow, 0.2f, true);

	UE_LOG(LogTemp, Log, TEXT("[CT] 게임 시작: 전체 %.0f초, 라운드 %.0f초"), GameDurationSeconds, RoundDurationSeconds);
}

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

	// 꼬리 최다 플레이어 찾기
	const TArray<ACTPlayerState*> Players = CollectPlayers();
	int32 MaxTailCount = -1;
	for (const ACTPlayerState* PlayerState : Players)
	{
		MaxTailCount = FMath::Max(MaxTailCount, PlayerState->GetTailCount());
	}

	TArray<ACTPlayerState*> Winners;
	for (ACTPlayerState* PlayerState : Players)
	{
		if (PlayerState->GetTailCount() == MaxTailCount)
		{
			Winners.Add(PlayerState);
		}
	}

	const bool bIsTie = Winners.Num() > 1;
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

	UE_LOG(LogTemp, Log, TEXT("[CT] 대기 로비로 복귀: %s"), *TravelName);
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
		StartNewRound();
	}
}

void ACTGameMode::StartNewRound()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState) { return; }

	const double Now = CTGameState->GetServerWorldTimeSeconds();
	const int32 NewRoundNumber = CTGameState->GetRoundNumber() + 1;

	// 0) 맵에 남은 꼬리 아이템을 모두 치움. 라운드가 끝나기 전에 일부러 기믹에 맞아 꼬리를 떨어뜨리고,
	//    새 라운드의 보충 꼬리와 함께 그 아이템까지 먹는 꼼수를 막음 (새 아이템은 스포너가 다시 만듦)
	if (UWorld* World = GetWorld())
	{
		TArray<ACTTailPickup*> LeftoverPickups;
		for (TActorIterator<ACTTailPickup> It(World); It; ++It) { LeftoverPickups.Add(*It); }
		for (ACTTailPickup* Pickup : LeftoverPickups) { Pickup->Destroy(); }
		if (LeftoverPickups.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[CT] 새 라운드 시작: 맵에 남은 꼬리 아이템 %d개 제거"), LeftoverPickups.Num());
		}
	}

	// 1) 유령 부활
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		PlayerState->SetIsGhost(false);
	}

	// 1-2) 꼬리 부족한 플레이어에게 채움
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		if (PlayerState->GetTailCount() < MinTailCountAtRoundStart)
		{
			UE_LOG(LogTemp, Log, TEXT("[CT] 라운드 시작 꼬리 보충: %s  %d → %d"), *PlayerState->GetPlayerName(), PlayerState->GetTailCount(), MinTailCountAtRoundStart);
			PlayerState->SetTailCount(MinTailCountAtRoundStart);
		}
	}

	// 2) 색/목표 재배정
	AssignColorsAndTargets();

	// 3) 라운드 종료 시각. 게임 종료 시각 초과 금지
	const double RoundEnd = FMath::Min(Now + RoundDurationSeconds, CTGameState->GetGameEndServerTime());
	CTGameState->SetRoundInfo(NewRoundNumber, RoundEnd);

	UE_LOG(LogTemp, Log, TEXT("[CT] 라운드 %d 시작"), NewRoundNumber);
	OnRoundStarted(NewRoundNumber);
}

void ACTGameMode::AssignColorsAndTargets()
{
	TArray<ACTPlayerState*> Players = CollectPlayers();

	const int32 PlayerCount = Players.Num();
	const int32 AssignCount = FMath::Min(PlayerCount, CTColorCount);
	if (PlayerCount > CTColorCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 플레이어(%d명)가 색(%d개)보다 많습니다. 초과한 플레이어는 색이 없습니다."), PlayerCount, CTColorCount);
	}

	// 셔플 후 앞에서부터 배정. 직전 색 충돌 적은 배치 선택
	TArray<ECTColor> Pool;
	for (int32 Index = 0; Index < CTColorCount; ++Index)
	{
		Pool.Add(static_cast<ECTColor>(Index));
	}

	TArray<ECTColor> BestPool = Pool;
	int32 BestConflictCount = MAX_int32;

	for (int32 Attempt = 0; Attempt < 32 && BestConflictCount > 0; ++Attempt)
	{
		// Fisher-Yates
		for (int32 Index = Pool.Num() - 1; Index > 0; --Index)
		{
			Pool.Swap(Index, FMath::RandRange(0, Index));
		}

		int32 ConflictCount = 0;
		for (int32 PlayerIndex = 0; PlayerIndex < AssignCount; ++PlayerIndex)
		{
			if (Players[PlayerIndex]->GetAssignedColor() == Pool[PlayerIndex])
			{
				++ConflictCount;
			}
		}

		if (ConflictCount < BestConflictCount)
		{
			BestConflictCount = ConflictCount;
			BestPool = Pool;
		}
	}

	// 사용 중인 색 정렬 (목표 색 계산용)
	TArray<uint8> UsedColorValues;
	for (int32 PlayerIndex = 0; PlayerIndex < AssignCount; ++PlayerIndex)
	{
		UsedColorValues.Add(static_cast<uint8>(BestPool[PlayerIndex]));
	}
	UsedColorValues.Sort();

	for (int32 PlayerIndex = 0; PlayerIndex < PlayerCount; ++PlayerIndex)
	{
		ACTPlayerState* PlayerState = Players[PlayerIndex];

		if (PlayerIndex >= AssignCount)
		{
			PlayerState->SetColors(ECTColor::None, ECTColor::None);
			continue;
		}

		const ECTColor MyColor = BestPool[PlayerIndex];
		ECTColor Target = ECTColor::None;

		if (bTargetFromActiveColorsOnly)
		{
			// 혼자면 목표 없음
			if (AssignCount > 1)
			{
				const int32 MyIndex = UsedColorValues.IndexOfByKey(static_cast<uint8>(MyColor));
				const int32 NextIndex = (MyIndex + 1) % UsedColorValues.Num();
				Target = static_cast<ECTColor>(UsedColorValues[NextIndex]);
			}
		}
		else
		{
			// 색 순서 다음. 분홍 → 빨강
			Target = static_cast<ECTColor>((static_cast<int32>(MyColor) + 1) % CTColorCount);
		}

		PlayerState->SetColors(MyColor, Target);
	}
}

// 꼬리 잡기

bool ACTGameMode::HandleTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }
	if (!Grabber || !Victim || Grabber == Victim) { return false; }

	// 유령은 잡기/잡힘 불가
	if (Grabber->IsGhost() || Victim->IsGhost()) { return false; }

	// 목표 색만 가능
	if (Grabber->GetTargetColor() == ECTColor::None) { return false; }
	if (Victim->GetAssignedColor() != Grabber->GetTargetColor()) { return false; }

	// 꼬리 줄다리기: 둘 다 대결에 들어감. 결과는 UCTGrappleComponent 가 FinalizeTailGrabbed 로 알림
	if (bUseGrappleStruggle)
	{
		ACTCharacter* GrabberCharacter = Cast<ACTCharacter>(Grabber->GetPawn());
		ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn());
		if (GrabberCharacter && VictimCharacter && GrabberCharacter->GetGrapple() && VictimCharacter->GetGrapple())
		{
			return GrabberCharacter->GetGrapple()->BeginGrapple(VictimCharacter);
		}
	}

	return FinalizeTailGrabbed(Grabber, Victim);
}

bool ACTGameMode::FinalizeTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }
	if (!Grabber || !Victim || Grabber == Victim) { return false; }
	if (Grabber->IsGhost() || Victim->IsGhost()) { return false; }
	if (Grabber->GetTargetColor() == ECTColor::None || Victim->GetAssignedColor() != Grabber->GetTargetColor()) { return false; }

	// 잡힌 쪽은 꼬리를 잃고 유령. 잡은 쪽은 빼앗은 만큼 얻음 (최소 1개)
	const int32 Taken = bTakeAllTailsOnGrab ? FMath::Max(Victim->GetTailCount(), 1) : 1;
	Grabber->AddTailCount(Taken);
	Victim->SetTailCount(FMath::Max(Victim->GetTailCount() - Taken, 0));
	Victim->SetIsGhost(true);

	// 잡힘 연출, 다른 경로도 동일 재생
	if (ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn()))
	{
		VictimCharacter->MulticastPlayAction(ECTCharacterAction::Caught, true);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] %s 이(가) %s 의 꼬리를 잡음"), *Grabber->GetPlayerName(), *Victim->GetPlayerName());
	OnTailGrabbed(Grabber, Victim);
	return true;
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
