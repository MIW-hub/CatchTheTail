// Copyright Moon Party Games. All Rights Reserved.
#include "CTRule_TreasureHunt.h"
#include "CTGameMode.h"
#include "CTGameInstance.h"
#include "CTMapSettings.h"
#include "CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTGrappleComponent.h"
#include "../World/CTEnergyPickup.h"
#include "../World/CTThrowable.h"
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
	// (로비에서 미리 정해 둔 색이 있으면 그대로 씀)
	TArray<ECTColor> TeamColors;
	if (const UCTGameInstance* GameInstance = Mode->GetGameInstance<UCTGameInstance>())
	{
		const TArray<ECTColor>& Stored = GameInstance->GetStoredTeamColors();
		if (Stored.Num() == 2 && Stored[0] != Stored[1]) { TeamColors = Stored; }
	}
	if (TeamColors.Num() != 2)
	{
		TArray<int32> ColorIndices;
		for (int32 Index = 0; Index < CTColorCount; ++Index) { ColorIndices.Add(Index); }
		for (int32 Index = ColorIndices.Num() - 1; Index > 0; --Index) { ColorIndices.Swap(Index, FMath::RandRange(0, Index)); }
		TeamColors = { static_cast<ECTColor>(ColorIndices[0]), static_cast<ECTColor>(ColorIndices[1]) };
	}
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

	// 쉬는 시간에 이미 준비했으면 다시 하지 않음 (첫 라운드와 쉬는 시간이 없을 때만 여기서 준비)
	if (!bNextRoundPrepared) { PrepareRound(RoundNumber, 0.f); }
	else if (RoundNumber == 1) { PlacePlayersRandomly(true); } // 카운트다운 때 준비했고, 그 뒤에 들어온 AI 만 놓음
	bNextRoundPrepared = false;

	// 라운드가 시작될 때는 에너지가 모두 내려앉아 있어야 함 (아니면 준비 시간 계산 오류)
	if (UWorld* CheckWorld = Mode->GetWorld())
	{
		int32 StillFalling = 0;
		for (TActorIterator<ACTEnergyPickup> It(CheckWorld); It; ++It) { if (!It->IsLanded()) { ++StillFalling; } }
		if (StillFalling > 0) { UE_LOG(LogTemp, Log, TEXT("[CT] 오류: 라운드 %d 시작 때 아직 떨어지는 에너지 %d개"), RoundNumber, StillFalling); }
	}
	PrePlaced.Reset();
}

int32 UCTRule_TreasureHunt::PrepareRound(int32 RoundNumber, float DropWindowSeconds)
{
	ACTGameMode* Mode = GetGameMode();
	if (!Mode) { return 0; }

	CurrentRound = RoundNumber;

	// 라운드마다 개인 에너지와 꼬리를 0으로 되돌리고 에너지를 새로 깖
	for (ACTPlayerState* Player : Mode->CollectPlayers())
	{
		SetPlayerEnergy(Player, 0);
	}

	ClearEnergies();
	ClearThrowables();
	// 떨어지는 시간(약 0.9초)과 여유를 빼고 남는 만큼 안에서 시작 시각을 흩음 (최대 2초). 준비 시간이 0이면 바로 그 자리에 놓음
	const int32 Placed = SpawnEnergies(DropWindowSeconds > 0.f ? FMath::Clamp(DropWindowSeconds - 1.4f, 0.f, 2.f) : -1.f);
	SpawnThrowables();
	// 첫 라운드는 카운트다운 때 이미 놓인 플레이어를 다시 옮기지 않음
	PlacePlayersRandomly(RoundNumber == 1);
	return Placed;
}

void UCTRule_TreasureHunt::PrepareNextRound(int32 NextRoundNumber)
{
	// 쉬는 시간이 시작되고 1초 뒤에 준비하므로, 남은 시간 안에 모두 내려앉아야 함
	PrepareRound(NextRoundNumber, RoundBreakSeconds - 1.f);
	bNextRoundPrepared = true;
}

int32 UCTRule_TreasureHunt::OnHazardHit(ACTCharacter* Victim, const FVector& Direction)
{
	ACTPlayerState* VictimState = Victim ? Victim->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!VictimState || VictimState->IsForfeited() || VictimState->IsOnlyASpectator()) { return 0; }

	// 던진 오브젝트에 맞았을 때와 같음: 에너지의 절반(내림, 1개 이상이면 최소 1개)을 떨어뜨림
	const int32 Energy = VictimState->GetEnergy();
	const int32 Loss = (Energy <= 0) ? 0 : FMath::Max(Energy / 2, 1);
	if (Loss <= 0) { return 0; }

	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 충돌체에 맞음: %s, 에너지 %d -> %d (-%d)"), *VictimState->GetPlayerName(), Energy, Energy - Loss, Loss);
	SetPlayerEnergy(VictimState, Energy - Loss);
	DropEnergies(Victim, Loss);
	return Loss;
}

void UCTRule_TreasureHunt::FillRoundBreakInfo(FCTRoundBreakInfo& OutInfo, int32 RoundNumber)
{
	ACTGameMode* Mode = GetGameMode();
	OutInfo.RoundNumber = RoundNumber;
	OutInfo.TeamEnergy = { GetTeamEnergy(0), GetTeamEnergy(1) };
	OutInfo.WinsAfter = { RoundWins[0], RoundWins[1] };
	// 이번 라운드로 오른 승점을 빼서 팝업이 "이전 승점 -> +1" 로 올려 보여줌
	const int32 E0 = OutInfo.TeamEnergy[0], E1 = OutInfo.TeamEnergy[1];
	OutInfo.WinsBefore = { RoundWins[0] - (E0 >= E1 ? 1 : 0), RoundWins[1] - (E1 >= E0 ? 1 : 0) };

	OutInfo.Entries.Reset();
	if (!Mode) { return; }
	for (ACTPlayerState* Player : Mode->CollectPlayers())
	{
		FCTRoundBreakEntry Entry;
		Entry.Name = Player->GetPlayerName();
		Entry.TeamId = Player->GetTeamId();
		Entry.Score = Player->GetEnergy();
		OutInfo.Entries.Add(Entry);
	}
	// 점수 높은 순
	OutInfo.Entries.Sort([](const FCTRoundBreakEntry& A, const FCTRoundBreakEntry& B) { return A.Score > B.Score; });
}

void UCTRule_TreasureHunt::OnCountdownStart()
{
	// 첫 라운드는 시작 카운트다운 동안 미리 준비: 에너지를 깔아 모두 내려앉게 하고 플레이어를 제자리에 놓음 (카운트다운이 다시 시작돼도 한 번만 깖)
	ACTGameMode* Mode = GetGameMode();
	if (Mode && !bNextRoundPrepared)
	{
		bNextRoundPrepared = true;
		// 이동 지도(NavMesh)가 아직 준비되지 않은 맵은 에너지가 거의 안 깔림: 그러면 라운드 시작 때 바로 그 자리에 깖 (떨어뜨리지 않음)
		if (PrepareRound(1, Mode->GameStartCountdownSeconds) < EnergyPerRound / 2)
		{
			UE_LOG(LogTemp, Log, TEXT("[CT] 카운트다운 중에는 에너지를 충분히 깔지 못해(이동 지도 준비 중) 라운드 시작 때 다시 깖"));
			bNextRoundPrepared = false;
		}
	}
	else
	{
		PlacePlayersRandomly(true);
	}
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
	if (!World) { return false; }

	FVector GroundPoint = FVector::ZeroVector;
	FNavLocation NavPoint;
	if (Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) && Nav->GetRandomReachablePointInRadius(Origin, Radius, NavPoint))
	{
		GroundPoint = NavPoint.Location;
	}
	else if (!Nav || !Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
	{
		// 이동 지도(NavMesh)가 없는 맵: 시작 지점 주변의 임의 위치에서 아래로 쏴서 평평한 바닥을 찾음 (갈 수 있는지는 알 수 없어 범위를 좁게 잡음)
		const float FallbackRadius = FMath::Min(Radius, 3000.f);
		const FVector2D Offset = FMath::RandPointInCircle(FallbackRadius);
		const FVector Top = Origin + FVector(Offset.X, Offset.Y, 1500.f);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CTTreasureGround), false);
		if (!World->LineTraceSingleByChannel(Hit, Top, Top - FVector(0.f, 0.f, 4000.f), ECC_Visibility, Params)) { return false; }
		if (Hit.ImpactNormal.Z < 0.85f || Hit.ImpactPoint.Z > Origin.Z + 300.f) { return false; }
		GroundPoint = Hit.ImpactPoint;
	}
	else
	{
		return false;
	}

	// 벽이나 오브젝트에 겹치는 자리는 제외
	const FVector Center = GroundPoint + FVector(0.f, 0.f, ClearHalfHeight + 5.f);
	if (World->OverlapBlockingTestByChannel(Center, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(ClearRadius, ClearHalfHeight)))
	{
		return false;
	}

	OutPoint = GroundPoint;
	return true;
}

int32 UCTRule_TreasureHunt::SpawnEnergies(float MaxDropDelay)
{
	UWorld* World = GetGameMode() ? GetGameMode()->GetWorld() : nullptr;
	if (!World) { return 0; }

	// 길찾기가 닿는 범위의 기준이 될 시작점들
	TArray<FVector> Origins;
	for (TActorIterator<APlayerStart> It(World); It; ++It) { Origins.Add(It->GetActorLocation()); }
	if (Origins.IsEmpty()) { return 0; }

	TArray<FVector> Placed;
	int32 Dropped = 0;
	for (int32 Attempt = 0; Attempt < EnergyPerRound * 30 && Placed.Num() < EnergyPerRound; ++Attempt)
	{
		FVector Point;
		// 벽에 바짝 붙어 가려 보이는 자리를 피하려고 주변 80cm 가 비어 있는 곳만 고름
		if (!PickClearPoint(Origins[FMath::RandRange(0, Origins.Num() - 1)], 30000.f, 80.f, 80.f, Point)) { continue; }

		// 서로 너무 가깝게 겹치지 않게
		bool bTooClose = false;
		for (const FVector& Other : Placed)
		{
			if (FVector::DistSquared2D(Point, Other) < FMath::Square(150.f)) { bTooClose = true; break; }
		}
		if (bTooClose) { continue; }

		// 머리 위가 하늘로 트여 있으면 하늘에서 떨어뜨림 (천장, 지붕, 나뭇가지 아래는 바로 그 자리에 놓음)
		const FVector Spot = Point + FVector(0.f, 0.f, 60.f);
		FHitResult SkyHit;
		FCollisionQueryParams SkyParams(SCENE_QUERY_STAT(CTEnergySky), false);
		const bool bOpenSky = !World->SweepSingleByChannel(SkyHit, Spot + FVector(0.f, 0.f, 20.f), Spot + FVector(0.f, 0.f, 1600.f), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(40.f), SkyParams);
		const float FallDelay = (bOpenSky && MaxDropDelay >= 0.f) ? FMath::FRandRange(0.f, MaxDropDelay) : -1.f;

		Placed.Add(Point);
		if (bOpenSky) { ++Dropped; }
		ACTEnergyPickup::Spawn(World, Spot, 1, FallDelay);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 라운드 %d 준비: 에너지 %d개 배치 (하늘에서 %d개)"), CurrentRound, Placed.Num(), Dropped);
	return Placed.Num();
}

void UCTRule_TreasureHunt::ClearThrowables()
{
	UWorld* World = GetGameMode() ? GetGameMode()->GetWorld() : nullptr;
	if (!World) { return; }

	// 들고 있는 것도 함께 정리됨 (들고 있던 캐릭터 손에서 빠짐)
	TArray<ACTThrowable*> Existing;
	for (TActorIterator<ACTThrowable> It(World); It; ++It) { Existing.Add(*It); }
	for (ACTThrowable* Throwable : Existing) { Throwable->Destroy(); }
}

void UCTRule_TreasureHunt::SpawnThrowables()
{
	ACTGameMode* Mode = GetGameMode();
	UWorld* World = Mode ? Mode->GetWorld() : nullptr;
	if (!World) { return; }

	TArray<FVector> Origins;
	for (TActorIterator<APlayerStart> It(World); It; ++It) { Origins.Add(It->GetActorLocation()); }
	if (Origins.IsEmpty()) { return; }

	// 이 맵에 맞는 오브젝트 모양: 프로젝트 설정(CT Maps)의 맵 항목 > 게임 모드 기본값 > 기본 공
	TSubclassOf<ACTThrowable> ThrowableClass = Mode->TreasureThrowableClass;
	{
		const FString LevelName = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
		for (const FCTMapInfo& Info : GetDefault<UCTMapSettings>()->Maps)
		{
			if (Info.Level.GetLongPackageName() == LevelName && !Info.TreasureThrowable.IsNull())
			{
				if (UClass* Loaded = Info.TreasureThrowable.LoadSynchronous()) { ThrowableClass = Loaded; }
				break;
			}
		}
	}

	TArray<FVector> Placed;
	const int32 Wanted = Mode->TreasureThrowableCount;
	for (int32 Attempt = 0; Attempt < Wanted * 60 && Placed.Num() < Wanted; ++Attempt)
	{
		FVector Point;
		if (!PickClearPoint(Origins[FMath::RandRange(0, Origins.Num() - 1)], 30000.f, 60.f, 60.f, Point)) { continue; }

		// 오브젝트끼리 멀리 흩어지게
		bool bTooClose = false;
		for (const FVector& Other : Placed)
		{
			if (FVector::DistSquared2D(Point, Other) < FMath::Square(600.f)) { bTooClose = true; break; }
		}
		if (bTooClose) { continue; }

		Placed.Add(Point);
		ACTThrowable::Spawn(World, ThrowableClass, Point + FVector(0.f, 0.f, 70.f));
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 라운드 %d 준비: 던지기 오브젝트 %d개 배치"), CurrentRound, Placed.Num());
	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 던지기 오브젝트 모양: %s"), ThrowableClass ? *ThrowableClass->GetName() : TEXT("기본 공"));
}

void UCTRule_TreasureHunt::PlacePlayersRandomly(bool bOnlyNew)
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
		if (bOnlyNew && PrePlaced.Contains(Player))
		{
			Taken.Add(Character->GetActorLocation()); // 이미 놓인 사람 자리는 피함
			continue;
		}

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
		PrePlaced.Add(Player);
		Character->GetCharacterMovement()->StopMovementImmediately();
		Character->TeleportTo(Point + FVector(0.f, 0.f, HalfHeight + 5.f), FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f));
		++Moved;
	}

	if (bOnlyNew && Moved > 0 && !Taken.IsEmpty() && Moved < Taken.Num()) { UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 라운드 %d: 새로 들어온 플레이어 %d명 추가 배치"), CurrentRound, Moved); }
	else if (!(bOnlyNew && Moved == 0)) { UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 라운드 %d 준비: 플레이어 %d명 무작위 스폰"), CurrentRound, Moved); }
}

// 에너지와 꼬리

void UCTRule_TreasureHunt::SetPlayerEnergy(ACTPlayerState* Player, int32 NewEnergy) const
{
	if (!Player) { return; }

	Player->SetEnergy(NewEnergy);
	Player->SetTailCount(Player->GetEnergy() / EnergyPerTail); // 꼬리 1개 = 에너지 10개

	// 같은 팀원 모두에게 우리 팀 합계를 알려 줌 (상대 팀에게는 복제되지 않음)
	if (const ACTGameMode* Mode = GetGameMode())
	{
		const int32 Team = Player->GetTeamId();
		const int32 Sum = GetTeamEnergy(Team);
		for (ACTPlayerState* Other : Mode->CollectPlayers())
		{
			if (Other->GetTeamId() == Team) { Other->SetTeamEnergy(Sum); }
		}
	}
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

// 던진 오브젝트에 맞음: 에너지의 절반(내림, 1개 이상이면 최소 1개)을 떨어뜨림. 아군이 맞아도 똑같음

void UCTRule_TreasureHunt::OnThrowableHit(ACTCharacter* Thrower, ACTCharacter* Victim, const FVector& Velocity)
{
	ACTPlayerState* VictimState = Victim ? Victim->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!VictimState || VictimState->IsForfeited() || VictimState->IsOnlyASpectator()) { return; }

	const int32 Energy = VictimState->GetEnergy();
	const int32 Loss = (Energy <= 0) ? 0 : FMath::Max(Energy / 2, 1);

	const ACTPlayerState* ThrowerState = Thrower ? Thrower->GetPlayerState<ACTPlayerState>() : nullptr;
	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 맞음: %s -> %s, 에너지 %d -> %d (-%d)%s"),
		ThrowerState ? *ThrowerState->GetPlayerName() : TEXT("?"), *VictimState->GetPlayerName(), Energy, Energy - Loss, Loss,
		(ThrowerState && ThrowerState->GetTeamId() == VictimState->GetTeamId()) ? TEXT(" [아군]") : TEXT(""));

	// 맞은 연출은 에너지가 없어도 보여줌
	Victim->PlayKnockedDown(Velocity);
	if (Loss <= 0) { return; }

	SetPlayerEnergy(VictimState, Energy - Loss);
	DropEnergies(Victim, Loss);
}

void UCTRule_TreasureHunt::DropEnergies(ACTCharacter* Victim, int32 Count)
{
	UWorld* World = Victim ? Victim->GetWorld() : nullptr;
	if (!World) { return; }

	const FVector Origin = Victim->GetActorLocation();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// 몸에서 조금 떨어진 바닥에 흩뿌림 (바로 다시 먹히지 않을 거리)
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(170.f, 320.f);
		FVector Spot = Origin + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);

		// 바닥 높이를 찾음. 못 찾으면 몸 아래 높이로
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CTEnergyDrop), false, Victim);
		if (World->LineTraceSingleByChannel(Hit, Spot + FVector(0.f, 0.f, 200.f), Spot - FVector(0.f, 0.f, 600.f), ECC_Visibility, Params))
		{
			Spot.Z = Hit.ImpactPoint.Z;
		}
		else
		{
			Spot.Z = Origin.Z - Victim->GetSimpleCollisionHalfHeight();
		}
		// 맞은 사람의 가슴 높이에서 터지듯 튀어 나가 바닥에 내려앉음
		ACTEnergyPickup::SpawnBurst(World, Origin + FVector(0.f, 0.f, 40.f), Spot + FVector(0.f, 0.f, 60.f), 1);
	}
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

	ACTCharacter* GrabberCharacter = Cast<ACTCharacter>(Grabber->GetPawn());
	ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn());
	if (!VictimCharacter || VictimCharacter->IsBoothLocked()) { return false; } // 이미 멈춰 있으면 중복으로 잡지 않음

	// 꼬리잡기와 같은 연타 대결: 잡은 쪽이 이기면 FinalizeGrab(약 2초 멈춤), 잡힌 쪽이 뿌리치면 잡은 쪽이 튕겨 나감
	if (Mode->bUseGrappleStruggle && GrabberCharacter && GrabberCharacter->GetGrapple() && VictimCharacter->GetGrapple())
	{
		return GrabberCharacter->GetGrapple()->BeginGrapple(VictimCharacter);
	}

	return FinalizeGrab(Grabber, Victim);
}

bool UCTRule_TreasureHunt::FinalizeGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	ACTGameMode* Mode = GetGameMode();
	const ACTGameState* GameState = Mode ? Mode->GetGameState<ACTGameState>() : nullptr;
	if (!GameState || GameState->GetPhase() != ECTGamePhase::Playing) { return false; }
	if (!Grabber || !Victim || Grabber == Victim) { return false; }
	if (Grabber->GetTeamId() == Victim->GetTeamId()) { return false; }

	ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn());
	if (!VictimCharacter || VictimCharacter->IsBoothLocked()) { return false; }
	// 약 2초 멈춤. 멈춘 동안에도 무적이 아님
	VictimCharacter->SetBoothLocked(true);
	VictimCharacter->MulticastPlayAction(ECTCharacterAction::Caught, true);
	UE_LOG(LogTemp, Log, TEXT("[CT] 보물찾기 잡기: %s -> %s (2초 멈춤)"), *Grabber->GetPlayerName(), *Victim->GetPlayerName());

	FTimerHandle Handle;
	Mode->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(VictimCharacter, [VictimCharacter]()
	{
		VictimCharacter->SetBoothLocked(false);
	}), GrabFreezeSeconds, false);
	return true;
}
