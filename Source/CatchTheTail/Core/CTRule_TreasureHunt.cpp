// Copyright Moon Party Games. All Rights Reserved.
#include "CTRule_TreasureHunt.h"
#include "CTGameMode.h"
#include "CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "../Player/CTCharacter.h"
#include "../World/CTEnergyPickup.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

// 경기 시작: 팀 배정과 팀 색

void UCTRule_TreasureHunt::OnMatchStart()
{
	ACTGameMode* Mode = GetGameMode();
	ACTGameState* GameState = Mode ? Mode->GetGameState<ACTGameState>() : nullptr;
	if (!Mode || !GameState) { return; }

	CurrentRound = 0;
	bRoundScored = false;
	RoundWins[0] = RoundWins[1] = 0;
	CollectedTotal.Reset();

	const TArray<ACTPlayerState*> Players = Mode->CollectPlayers();

	// 팀 배정: 로비에서 이미 정해진 팀은 그대로 두고, 정해지지 않은 사람, AI 순으로 인원이 적은 팀(같으면 0번 팀)에 넣음
	int32 TeamCount[2] = { 0, 0 };
	for (const ACTPlayerState* Player : Players)
	{
		if (Player->GetTeamId() == 0 || Player->GetTeamId() == 1) { ++TeamCount[Player->GetTeamId()]; }
	}
	auto AssignUnassigned = [&](bool bBots)
	{
		for (ACTPlayerState* Player : Players)
		{
			if (Player->IsABot() != bBots) { continue; }
			if (Player->GetTeamId() == 0 || Player->GetTeamId() == 1) { continue; }

			const int32 Team = (TeamCount[1] < TeamCount[0]) ? 1 : 0;
			Player->SetTeamId(Team);
			++TeamCount[Team];
		}
	};
	AssignUnassigned(false);
	AssignUnassigned(true);

	// 팀 색: 서로 다른 색 두 개를 무작위로. 같은 팀은 모두 같은 색
	TArray<int32> ColorIndices;
	for (int32 Index = 0; Index < CTColorCount; ++Index) { ColorIndices.Add(Index); }
	for (int32 Index = ColorIndices.Num() - 1; Index > 0; --Index) { ColorIndices.Swap(Index, FMath::RandRange(0, Index)); }
	const TArray<ECTColor> TeamColors = { static_cast<ECTColor>(ColorIndices[0]), static_cast<ECTColor>(ColorIndices[1]) };
	GameState->SetTeamColors(TeamColors);
	GameState->SetTeamRoundWins({ 0, 0 });

	for (ACTPlayerState* Player : Players)
	{
		// 목표 색은 쓰지 않음 (None)
		Player->SetColors(TeamColors[Player->GetTeamId()], ECTColor::None);
		Player->SetEnergy(0);
		Player->SetResultEnergy(0);
		Player->SetTailCount(0);
		Player->SetIsGhost(false);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 시작: 팀0 %d명, 팀1 %d명"), TeamCount[0], TeamCount[1]);
}

// 라운드 시작: 에너지 배치와 플레이어 무작위 스폰

float UCTRule_TreasureHunt::GetGameDurationSeconds(float DefaultSeconds) const
{
	const ACTGameMode* Mode = GetGameMode();
	return Mode ? Mode->RoundDurationSeconds * TotalRounds : DefaultSeconds;
}

void UCTRule_TreasureHunt::OnRoundStart(int32 RoundNumber)
{
	ACTGameMode* Mode = GetGameMode();
	if (!Mode) { return; }

	CurrentRound = RoundNumber;
	bRoundScored = false;

	// 라운드마다 개인 에너지와 꼬리를 0으로 되돌리고 에너지를 새로 깖
	for (ACTPlayerState* Player : Mode->CollectPlayers())
	{
		SetPlayerEnergy(Player, 0);
	}

	ClearEnergies();
	SpawnEnergies();
	PlacePlayersRandomly();
}

void UCTRule_TreasureHunt::ClearEnergies()
{
	UWorld* World = GetGameMode() ? GetGameMode()->GetWorld() : nullptr;
	if (!World) { return; }

	TArray<ACTEnergyPickup*> Existing;
	for (TActorIterator<ACTEnergyPickup> It(World); It; ++It) { Existing.Add(*It); }
	for (ACTEnergyPickup* Pickup : Existing) { Pickup->Destroy(); }
}

bool UCTRule_TreasureHunt::PickClearPoint(const FVector& Origin, float Radius, float ClearRadius, float ClearHalfHeight, FVector& OutPoint) const
{
	UWorld* World = GetGameMode() ? GetGameMode()->GetWorld() : nullptr;
	UNavigationSystemV1* Nav = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!Nav) { return false; }

	FNavLocation NavPoint;
	if (!Nav->GetRandomReachablePointInRadius(Origin, Radius, NavPoint)) { return false; }

	// 벽이나 오브젝트에 겹치는 자리는 제외
	const FVector Center = NavPoint.Location + FVector(0.f, 0.f, ClearHalfHeight + 5.f);
	if (World->OverlapBlockingTestByChannel(Center, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(ClearRadius, ClearHalfHeight)))
	{
		return false;
	}

	OutPoint = NavPoint.Location;
	return true;
}

void UCTRule_TreasureHunt::SpawnEnergies()
{
	UWorld* World = GetGameMode() ? GetGameMode()->GetWorld() : nullptr;
	if (!World) { return; }

	// 길찾기가 닿는 범위의 기준이 될 시작점들
	TArray<FVector> Origins;
	for (TActorIterator<APlayerStart> It(World); It; ++It) { Origins.Add(It->GetActorLocation()); }
	if (Origins.IsEmpty()) { return; }

	TArray<FVector> Placed;
	for (int32 Attempt = 0; Attempt < EnergyPerRound * 30 && Placed.Num() < EnergyPerRound; ++Attempt)
	{
		FVector Point;
		if (!PickClearPoint(Origins[FMath::RandRange(0, Origins.Num() - 1)], 30000.f, 40.f, 40.f, Point)) { continue; }

		// 서로 너무 가깝게 겹치지 않게
		bool bTooClose = false;
		for (const FVector& Other : Placed)
		{
			if (FVector::DistSquared2D(Point, Other) < FMath::Square(150.f)) { bTooClose = true; break; }
		}
		if (bTooClose) { continue; }

		Placed.Add(Point);
		ACTEnergyPickup::Spawn(World, Point + FVector(0.f, 0.f, 60.f), 1);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 라운드 %d 준비: 에너지 %d개 배치"), CurrentRound, Placed.Num());
}

void UCTRule_TreasureHunt::PlacePlayersRandomly()
{
	ACTGameMode* Mode = GetGameMode();
	UWorld* World = Mode ? Mode->GetWorld() : nullptr;
	if (!World) { return; }

	TArray<FVector> Origins;
	for (TActorIterator<APlayerStart> It(World); It; ++It) { Origins.Add(It->GetActorLocation()); }
	if (Origins.IsEmpty()) { return; }

	TArray<FVector> Taken;
	int32 Moved = 0;
	for (ACTPlayerState* Player : Mode->CollectPlayers())
	{
		ACTCharacter* Character = Cast<ACTCharacter>(Player->GetPawn());
		if (!Character) { continue; }

		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 34.f;
		const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.f;

		// 서로 8m 이상 떨어진 자리를 찾음. 좁은 맵에서 못 찾으면 거리를 조금씩 줄여 다시 시도
		float MinDistance = SpawnMinDistance;
		bool bFound = false;
		FVector Point = FVector::ZeroVector;
		for (int32 Attempt = 0; Attempt < 400 && !bFound; ++Attempt)
		{
			if (Attempt > 0 && Attempt % 80 == 0) { MinDistance *= 0.75f; }

			if (!PickClearPoint(Origins[FMath::RandRange(0, Origins.Num() - 1)], 30000.f, Radius + 6.f, HalfHeight, Point)) { continue; }

			bool bFarEnough = true;
			for (const FVector& Other : Taken)
			{
				if (FVector::DistSquared2D(Point, Other) < FMath::Square(MinDistance)) { bFarEnough = false; break; }
			}
			bFound = bFarEnough;
		}
		if (!bFound) { continue; } // 못 찾으면 그 자리에 둠

		Taken.Add(Point);
		Character->GetCharacterMovement()->StopMovementImmediately();
		Character->TeleportTo(Point + FVector(0.f, 0.f, HalfHeight + 5.f), FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f));
		++Moved;
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 라운드 %d 준비: 플레이어 %d명 무작위 스폰"), CurrentRound, Moved);
}

// 에너지와 꼬리

void UCTRule_TreasureHunt::SetPlayerEnergy(ACTPlayerState* Player, int32 NewEnergy) const
{
	if (!Player) { return; }

	Player->SetEnergy(NewEnergy);
	Player->SetTailCount(Player->GetEnergy() / EnergyPerTail); // 꼬리 1개 = 에너지 10개
}

bool UCTRule_TreasureHunt::OnEnergyPicked(ACTCharacter* Picker, int32 Amount)
{
	ACTPlayerState* Player = Picker ? Picker->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!Player || Player->IsForfeited() || Player->IsOnlyASpectator()) { return false; }

	SetPlayerEnergy(Player, Player->GetEnergy() + Amount);
	CollectedTotal.FindOrAdd(Player) += Amount;
	return true;
}

int32 UCTRule_TreasureHunt::GetTeamEnergy(int32 TeamId) const
{
	ACTGameMode* Mode = GetGameMode();
	if (!Mode) { return 0; }

	int32 Total = 0;
	for (const ACTPlayerState* Player : Mode->CollectPlayers())
	{
		if (Player->GetTeamId() == TeamId) { Total += Player->GetEnergy(); }
	}
	return Total;
}

// 라운드 정산과 결과

void UCTRule_TreasureHunt::ScoreRound(int32 RoundNumber)
{
	ACTGameMode* Mode = GetGameMode();
	ACTGameState* GameState = Mode ? Mode->GetGameState<ACTGameState>() : nullptr;
	if (!GameState || bRoundScored || RoundNumber <= 0) { return; }
	bRoundScored = true;

	const int32 Energy0 = GetTeamEnergy(0);
	const int32 Energy1 = GetTeamEnergy(1);
	if (Energy0 > Energy1) { ++RoundWins[0]; }
	else if (Energy1 > Energy0) { ++RoundWins[1]; }
	else { ++RoundWins[0]; ++RoundWins[1]; } // 동점이면 양 팀 모두 승점 +1

	GameState->SetTeamRoundWins({ RoundWins[0], RoundWins[1] });
	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 라운드 %d 정산: 팀0 %d / 팀1 %d, 승점 %d:%d"), RoundNumber, Energy0, Energy1, RoundWins[0], RoundWins[1]);
}

bool UCTRule_TreasureHunt::OnRoundTimeUp(int32 RoundNumber)
{
	ScoreRound(RoundNumber);
	return RoundNumber >= TotalRounds;
}

void UCTRule_TreasureHunt::OnGameEnding()
{
	// 마지막 라운드가 시간 종료와 겹쳐 정산되지 않았다면 여기서 정산
	ScoreRound(CurrentRound);
	ClearEnergies();

	// 경기가 끝난 뒤 모두에게 개인 에너지 점수 공개
	if (ACTGameMode* Mode = GetGameMode())
	{
		for (ACTPlayerState* Player : Mode->CollectPlayers())
		{
			const int32* Collected = CollectedTotal.Find(Player);
			Player->SetResultEnergy(Collected ? *Collected : 0);
		}
	}
}

void UCTRule_TreasureHunt::ComputeResult(TArray<ACTPlayerState*>& OutWinners, bool& bOutTie)
{
	ACTGameMode* Mode = GetGameMode();
	if (!Mode) { return; }

	OutWinners.Reset();
	bOutTie = (RoundWins[0] == RoundWins[1]);

	// 승점이 같으면 무승부(모두 공동), 아니면 이긴 팀 전원이 승자
	const int32 WinningTeam = RoundWins[0] > RoundWins[1] ? 0 : 1;
	for (ACTPlayerState* Player : Mode->CollectPlayers())
	{
		if (bOutTie || Player->GetTeamId() == WinningTeam) { OutWinners.Add(Player); }
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 종료: 승점 %d:%d, %s"), RoundWins[0], RoundWins[1], bOutTie ? TEXT("무승부") : (WinningTeam == 0 ? TEXT("팀0 승리") : TEXT("팀1 승리")));
}

// 잡기: 빼앗지 않고 상대를 잠깐 멈추게만 함

bool UCTRule_TreasureHunt::HandleGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	ACTGameMode* Mode = GetGameMode();
	const ACTGameState* GameState = Mode ? Mode->GetGameState<ACTGameState>() : nullptr;
	if (!GameState || GameState->GetPhase() != ECTGamePhase::Playing) { return false; }
	if (!Grabber || !Victim || Grabber == Victim) { return false; }
	if (Grabber->GetTeamId() == Victim->GetTeamId()) { return false; } // 아군은 잡을 수 없음

	ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn());
	if (!VictimCharacter || VictimCharacter->IsBoothLocked()) { return false; } // 이미 멈춰 있으면 중복으로 잡지 않음

	// 약 2초 멈춤. 멈춘 동안에도 무적이 아님
	VictimCharacter->SetBoothLocked(true);
	VictimCharacter->MulticastPlayAction(ECTCharacterAction::Caught, true);

	FTimerHandle Handle;
	Mode->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(VictimCharacter, [VictimCharacter]()
	{
		VictimCharacter->SetBoothLocked(false);
	}), GrabFreezeSeconds, false);
	return true;
}
