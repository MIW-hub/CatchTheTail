// Copyright Moon Party Games. All Rights Reserved.
#include "CTGameMode.h"
#include "CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "../Player/CTPlayerController.h"
#include "CatchTheTail/Player/CTCharacter.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"


ACTGameMode::ACTGameMode()
{
	// 기본 클래스 지정. (블루프린트 GameMode에서 BP_CTCharacter 등으로 바꿔도 됩니다)
	DefaultPawnClass = ACTCharacter::StaticClass();
	PlayerControllerClass = ACTPlayerController::StaticClass();
	GameStateClass = ACTGameState::StaticClass();
	PlayerStateClass = ACTPlayerState::StaticClass();
}

void ACTGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 테스트용 자동 시작: 새로 접속할 때마다 타이머를 다시 맞춥니다.
	if (bAutoStartForTesting)
	{
		const ACTGameState* CTGameState = GetGameState<ACTGameState>();
		if (CTGameState && CTGameState->GetPhase() == ECTGamePhase::WaitingForPlayers)
		{
			GetWorldTimerManager().SetTimer(
				AutoStartTimerHandle, this, &ACTGameMode::StartGame,
				FMath::Max(AutoStartDelaySeconds, 0.1f), false);
		}
	}
}

// ============================================================
// 게임 시작 / 종료
// ============================================================

void ACTGameMode::StartGame()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() == ECTGamePhase::Playing)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AutoStartTimerHandle);

	// 모든 플레이어 초기화
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		PlayerState->SetColors(ECTColor::None, ECTColor::None); // 첫 라운드는 "직전 색" 없음
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

	CTGameState->SetPhase(ECTGamePhase::Finished);
	RefreshAllChaseBuffs(); // 게임이 끝났으니 추격 버프 해제

	// 꼬리를 가장 많이 가진 사람(들) 찾기
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

	OnGameFinished(Winners, bIsTie);
}

// ============================================================
// 라운드 진행
// ============================================================

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

	// 1) 유령이었던 사람은 부활 (그 자리 그대로)
	for (ACTPlayerState* PlayerState : CollectPlayers())
	{
		PlayerState->SetIsGhost(false);
	}

	// 2) 색과 목표 색 다시 배정
	AssignColorsAndTargets();

	// 3) 라운드 종료 시각 (마지막 라운드는 게임 종료 시각을 넘지 않게)
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

	// 색 8개를 섞어서 앞에서부터 한 명씩 나눠 줍니다. (중복 없음)
	// "직전 라운드와 같은 색"이 적은 배치를 고르기 위해 여러 번 시도합니다.
	TArray<ECTColor> Pool;
	for (int32 Index = 0; Index < CTColorCount; ++Index)
	{
		Pool.Add(static_cast<ECTColor>(Index));
	}

	TArray<ECTColor> BestPool = Pool;
	int32 BestConflictCount = MAX_int32;

	for (int32 Attempt = 0; Attempt < 32 && BestConflictCount > 0; ++Attempt)
	{
		// 섞기 (Fisher-Yates)
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

	// 지금 게임에서 쓰는 색들을 색 순서대로 정렬 (목표 색 계산용)
	TArray<uint8> UsedColorValues;
	for (int32 PlayerIndex = 0; PlayerIndex < AssignCount; ++PlayerIndex)
	{
		UsedColorValues.Add(static_cast<uint8>(BestPool[PlayerIndex]));
	}
	UsedColorValues.Sort();

	// 배정
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
			// 혼자뿐이면 목표가 없습니다.
			if (AssignCount > 1)
			{
				const int32 MyIndex = UsedColorValues.IndexOfByKey(static_cast<uint8>(MyColor));
				const int32 NextIndex = (MyIndex + 1) % UsedColorValues.Num();
				Target = static_cast<ECTColor>(UsedColorValues[NextIndex]);
			}
		}
		else
		{
			// 색 순서의 바로 다음 색 (분홍 다음은 빨강)
			Target = static_cast<ECTColor>((static_cast<int32>(MyColor) + 1) % CTColorCount);
		}

		PlayerState->SetColors(MyColor, Target);
	}
}

// ============================================================
// 꼬리 잡기
// ============================================================

bool ACTGameMode::HandleTailGrabbed(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }
	if (!Grabber || !Victim || Grabber == Victim) { return false; }

	// 유령은 잡을 수도, 잡힐 수도 없습니다.
	if (Grabber->IsGhost() || Victim->IsGhost()) { return false; }

	// 정해진 목표 색만 잡을 수 있습니다.
	if (Grabber->GetTargetColor() == ECTColor::None) { return false; }
	if (Victim->GetAssignedColor() != Grabber->GetTargetColor()) { return false; }

	// 잡은 사람 +1, 잡힌 사람 -1 (0 아래로는 안 내려감), 잡힌 사람은 유령이 됨
	Grabber->AddTailCount(1);
	Victim->AddTailCount(-1);
	Victim->SetIsGhost(true);

	// 잡힌 사람의 반응 연출 (AI 등 다른 경로로 잡아도 똑같이 재생되도록 여기서 처리)
	if (ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn()))
	{
		VictimCharacter->MulticastPlayAction(ECTCharacterAction::Caught, true);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] %s 이(가) %s 의 꼬리를 잡음"), *Grabber->GetPlayerName(), *Victim->GetPlayerName());
	OnTailGrabbed(Grabber, Victim);
	return true;
}

// ============================================================
// 도우미
// ============================================================

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
