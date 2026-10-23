// Copyright Moon Party Games. All Rights Reserved.
#include "CTGameMode.h"
#include "CTGameInstance.h"
#include "CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "../Player/CTPlayerController.h"
#include "CatchTheTail/Player/CTCharacter.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
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

void ACTGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty()) { return; }

	// 게임 중 / 결과 화면 중에는 입장 불가. 로비를 거쳐 들어온 사람만 참여
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

		// listen 서버의 로컬 플레이어가 방장 (결과 화면의 로비 복귀 버튼에 사용)
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
	// Super 이후에는 PlayerState가 정리될 수 있어 주소만 비교용으로 보관
	const ACTPlayerState* LeavingState = Exiting ? Exiting->GetPlayerState<ACTPlayerState>() : nullptr;

	Super::Logout(Exiting);

	HandlePlayerLeftDuringGame(LeavingState);
}

void ACTGameMode::HandlePlayerLeftDuringGame(const ACTPlayerState* LeavingPlayer)
{
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

	// 나간 사람의 색이 목표였던 사람이 잡을 대상을 잃지 않게, 남은 색끼리 목표를 다시 이음
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

	const float Delay = FMath::Max(AutoStartDelaySeconds, 0.1f);

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

	// 플레이어 초기화
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		PlayerState->SetColors(ECTColor::None, ECTColor::None); // 직전 색 없음
		PlayerState->SetTailCount(InitialTailCount);
		PlayerState->SetIsGhost(false);
	}

	const double Now = CTGameState->GetServerWorldTimeSeconds();
	CTGameState->SetGameInfo(Now + GameDurationSeconds, RoundDurationSeconds, CountdownSeconds);
	CTGameState->SetRoundInfo(0, Now);
	CTGameState->SetPhase(ECTGamePhase::Playing);

	OnGameStarted();
	StartNewRound();
	RefreshAllChaseBuffs();

	GetWorldTimerManager().SetTimer(FlowTimerHandle, this, &ACTGameMode::TickGameFlow, 0.2f, true);

	UE_LOG(LogTemp, Log, TEXT("[CT] 게임 시작: 전체 %.0f초, 라운드 %.0f초"), GameDurationSeconds, RoundDurationSeconds);
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

	UE_LOG(LogTemp, Log, TEXT("[CT] 대기 로비로 복귀: %s"), *LobbyMap.ToString());
	World->ServerTravel(LobbyMap.ToString());
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

	// 1) 유령 부활
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		PlayerState->SetIsGhost(false);
	}

	// 1-2) 꼬리가 부족한 플레이어에게 꼬리를 채워 줍니다. (꼬리를 모두 잃었어도 새 라운드는 꼬리를 갖고 시작)
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

	// 잡은 쪽 +1, 잡힌 쪽 -1 후 유령
	Grabber->AddTailCount(1);
	Victim->AddTailCount(-1);
	Victim->SetIsGhost(true);

	// 잡힘 연출. AI 등 다른 경로도 동일 재생
	if (ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn()))
	{
		VictimCharacter->MulticastPlayAction(ECTCharacterAction::Caught, true);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] %s 이(가) %s 의 꼬리를 잡음"), *Grabber->GetPlayerName(), *Victim->GetPlayerName());
	OnTailGrabbed(Grabber, Victim);
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
			if (CTPlayerState && !CTPlayerState->IsOnlyASpectator())
			{
				Result.Add(CTPlayerState);
			}
		}
	}

	return Result;
}
