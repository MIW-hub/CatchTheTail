// Copyright Moon Party Games. All Rights Reserved.
#include "CTAIController.h"
#include "CTCharacter.h"
#include "CTPlayerState.h"
#include "../Core/CTAudioSettings.h"
#include "../Core/CTGameState.h"
#include "../World/CTBush.h"
#include "../World/CTExplosiveBarrel.h"
#include "../World/CTFallingProp.h"
#include "../World/CTRoadVehicle.h"
#include "../World/CTRollingLog.h"
#include "../World/CTTailPickup.h"
#include "../World/CTEnergyPickup.h"
#include "../World/CTThrowable.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "GameFramework/PlayerStart.h"
#include "Navigation/PathFollowingComponent.h"

ACTAIController::ACTAIController()
{
	// 점수판, 색 배정, 잡기 판정이 모두 PlayerState 기준이라 AI도 PlayerState를 가짐
	bWantsPlayerState = true;

	// 직선 이동 입력을 매 프레임 넣기 위해 매 프레임 틱 (판단은 ThinkInterval 마다)
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
}

void ACTAIController::OnUnPossess()
{
	StopMovement();
	bDirectMove = false;
	Super::OnUnPossess();
}

void ACTAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	if (!World || !HasAuthority()) { return; }

	ACTCharacter* Me = Cast<ACTCharacter>(GetPawn());
	const double TickNow = World->GetTimeSeconds();

	// 꼬리 대결 중이거나 잡힌 동작, 밀려 넘어지고 일어나는 동작이 재생되는 동안은 탐색, 이동을 멈춤. 끝난 뒤에도 잠깐 더 쉼
	if (Me && (Me->IsGrappling() || Me->IsPlayingCaughtMontage() || Me->IsPlayingKnockdownMontage()))
	{
		PausedUntil = TickNow + GrappledPauseSeconds;
	}
	if (Me && TickNow < PausedUntil)
	{
		if (!bPaused)
		{
			bPaused = true;
			bDirectMove = false;
			StopMovement();
			Me->AISetRun(false);
		}
		return;
	}
	bPaused = false;

	// 직선 이동 중이면 목적지 쪽으로 입력
	if (bDirectMove && Me)
	{
		FVector ToTarget = DirectMoveTarget - Me->GetActorLocation();
		ToTarget.Z = 0.f;
		if (ToTarget.SizeSquared() > FMath::Square(60.f))
		{
			Me->AddMovementInput(ToTarget.GetSafeNormal(), 1.f);
		}
		else
		{
			bDirectMove = false;
		}
	}

	const double Now = World->GetTimeSeconds();
	if (Now >= NextThinkTime)
	{
		NextThinkTime = Now + ThinkInterval;
		Think();
	}
}

bool ACTAIController::Senses(const ACTCharacter& Me, const ACTCharacter& Other, const AActor* ExtraIgnoredActor) const
{
	const FVector From = Me.GetActorLocation();
	const FVector To = Other.GetActorLocation();
	const float Distance = FVector::Dist2D(From, To);

	// 발소리: 움직이는 상대가 발소리 범위 안에 있으면 벽 너머여도 들림
	const UCTAudioSettings* Audio = GetDefault<UCTAudioSettings>();
	const float Hearing = HearingRadius > 0.f ? HearingRadius : (Audio->FootstepInnerRadius + Audio->FootstepFalloffDistance);
	if (Distance <= Hearing && Other.GetVelocity().Size2D() >= HearingMinSpeed)
	{
		return true;
	}

	// 시야: 거리, 정면 부채꼴(가까우면 뒤도), 벽에 안 가림
	if (Distance > SightRange) { return false; }

	if (Distance > CloseSenseRadius)
	{
		FVector Forward = Me.GetActorForwardVector();
		Forward.Z = 0.f;
		FVector Direction = (To - From);
		Direction.Z = 0.f;
		if (!Forward.Normalize() || !Direction.Normalize()) { return false; }
		if (FVector::DotProduct(Forward, Direction) < FMath::Cos(FMath::DegreesToRadians(SightHalfAngleDegrees))) { return false; }
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTAISight), false);
	Params.AddIgnoredActor(&Me);
	Params.AddIgnoredActor(&Other);
	if (ExtraIgnoredActor) { Params.AddIgnoredActor(ExtraIgnoredActor); } // 부쉬 자체는 시선을 가리지 않음
	FHitResult Hit;
	const FVector Eye(0.f, 0.f, EyeHeight);
	return !GetWorld()->LineTraceSingleByChannel(Hit, From + Eye, To + Eye, ECC_Visibility, Params);
}

void ACTAIController::Think()
{
	UWorld* World = GetWorld();
	ACTCharacter* Me = Cast<ACTCharacter>(GetPawn());
	const ACTPlayerState* MyState = GetPlayerState<ACTPlayerState>();
	const ACTGameState* GameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!World || !Me || !MyState || !GameState) { return; }

	// 경기 중이 아니거나 전화부스 사용 중이면 가만히
	if (GameState->GetPhase() != ECTGamePhase::Playing || Me->IsBoothLocked())
	{
		Halt();
		SetAIRun(*Me, false);
		return;
	}

	const double Now = World->GetTimeSeconds();
	RefreshHazardList(Now);

	// 보물찾기: 목표 색 대신 에너지를 모으러 다님
	if (GameState->GetModeType() == ECTGameModeType::TreasureHunt)
	{
		ThinkTreasure(*Me, Now);
		return;
	}

	// 유령: 위험물은 맞지 않으니 꼬리 아이템(먹으면 부활)을 찾아다님. 안 보이면 돌아다님
	if (MyState->IsGhost())
	{
		if (GoToTailPickup(*Me, Now, /*bIgnoreHazards=*/true)) { return; }

		SetAIRun(*Me, false);
		if (GetMoveStatus() == EPathFollowingStatus::Idle && !bDirectMove && Now >= NextWanderTime)
		{
			NextWanderTime = Now + FMath::FRandRange(0.3f, 1.5f);
			MoveToPoint(PickWanderPoint(Me->GetActorLocation()), 120.f);
		}
		return;
	}

	// 위험물 회피가 가장 우선
	if (AvoidHazards(*Me, Now)) { return; }

	const FVector MyLocation = Me->GetActorLocation();
	const ECTColor MyColor = MyState->GetAssignedColor();
	const ECTColor MyTarget = MyState->GetTargetColor();

	// 의심 점수 갱신 간격
	const float SuspicionDelta = static_cast<float>(FMath::Clamp(Now - LastSuspicionTime, 0.02, 0.5));
	LastSuspicionTime = Now;

	// 알아챈 상대 정리
	ACTCharacter* Hunter = nullptr;
	float HunterDistance = TNumericLimits<float>::Max();
	ACTCharacter* Prey = nullptr;
	float PreyDistance = TNumericLimits<float>::Max();
	ACTCharacter* Neutral = nullptr;
	float NeutralDistance = TNumericLimits<float>::Max();

	for (APlayerState* State : GameState->PlayerArray)
	{
		const ACTPlayerState* Other = Cast<ACTPlayerState>(State);
		if (!Other || Other == MyState || Other->IsGhost() || Other->IsForfeited() || Other->IsOnlyASpectator()) { continue; }

		ACTCharacter* OtherCharacter = Cast<ACTCharacter>(Other->GetPawn());
		if (!OtherCharacter) { continue; }

		// 부쉬 안은 안 보임. 단, 한자리에 오래 숨어 부쉬가 흔들리면(들킴) 흔들리는 부쉬를 보고 알아챔
		const ACTBush* OtherBush = OtherCharacter->GetCurrentBush();
		if (OtherBush && !OtherBush->IsAlerted()) { continue; }
		if (!Senses(*Me, *OtherCharacter, OtherBush)) { continue; }

		const float Distance = FVector::Dist2D(MyLocation, OtherCharacter->GetActorLocation());
		// 내 목표 색은 알지만, 나를 노리는 사람이 누구인지는 모름. 움직임으로 의심함
		const bool bIsPrey = MyTarget != ECTColor::None && Other->GetAssignedColor() == MyTarget;

		if (bIsPrey)
		{
			if (Distance < PreyDistance) { Prey = OtherCharacter; PreyDistance = Distance; }
			continue;
		}

		if (UpdateSuspicion(*OtherCharacter, MyLocation, SuspicionDelta, Now))
		{
			if (Distance < HunterDistance) { Hunter = OtherCharacter; HunterDistance = Distance; }
		}
		else if (Distance < NeutralDistance) { Neutral = OtherCharacter; NeutralDistance = Distance; }
	}
	DecayUnseenSuspects(SuspicionDelta, Now);

	// 목표를 놓쳐도 잠시 기억
	if (Prey)
	{
		RememberedPrey = Prey;
		RememberedPreyLocation = Prey->GetActorLocation();
		LastPreySenseTime = Now;
	}
	const bool bPreyRemembered = !Prey && RememberedPrey.IsValid() && (Now - LastPreySenseTime) < ChaseMemorySeconds;

	const float GrabReach = Me->GetGrabDistance() * 0.85f;
	const float PushReach = Me->GetPushDistance() * 0.85f;
	const bool bPushReady = (Now - LastPushTime) > (Me->GetPushCooldownSeconds() + 0.2f);

	// 디버그: 2초마다 상태 요약 (LogTemp Verbose)
	if (Now >= NextDebugLogTime)
	{
		NextDebugLogTime = Now + 2.0;
		UE_LOG(LogTemp, Verbose, TEXT("[CT-AI] %s 위치(%.0f,%.0f) 색=%d 목표=%d 추격자=%s 먹잇감=%s 기억=%d 이동상태=%d 직선=%d"),
			*MyState->GetPlayerName(), MyLocation.X, MyLocation.Y, static_cast<int32>(MyColor), static_cast<int32>(MyTarget),
			Hunter ? TEXT("O") : TEXT("X"), Prey ? TEXT("O") : TEXT("X"), bPreyRemembered ? 1 : 0, static_cast<int32>(GetMoveStatus()), bDirectMove ? 1 : 0);
	}

	// 1) 바로 앞에 목표가 있고 나를 노리는 상대가 멀면 먼저 잡기
	if (Prey && PreyDistance <= GrabReach && (!Hunter || HunterDistance > 350.f))
	{
		Halt();
		FaceLocation(*Me, Prey->GetActorLocation());
		Me->AIGrab();
		return;
	}

	// 1-2) 보이는 꼬리 아이템은 우선 먹으러 감 (나를 노리는 상대가 아주 가까우면 도망이 먼저)
	if (!Hunter || HunterDistance > PickupAbortHunterDistance)
	{
		if (GoToTailPickup(*Me, Now, /*bIgnoreHazards=*/false)) { return; }
	}

	// 2) 나를 노리는 상대가 가까우면 도망. 붙으면 밀어냄
	if (Hunter && HunterDistance <= FleeStartDistance)
	{
		if (HunterDistance <= PushReach && bPushReady)
		{
			Halt();
			FaceLocation(*Me, Hunter->GetActorLocation());
			Me->AIPush();
			LastPushTime = Now;
			return;
		}

		SetAIRun(*Me, HunterDistance <= RunDistance);

		if (Now >= NextMoveCommandTime || GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			NextMoveCommandTime = Now + 0.5;
			MoveToPoint(PickFleePoint(MyLocation, Hunter->GetActorLocation()), 120.f);
		}
		return;
	}

	// 3) 목표 추적 (알아챘거나 기억 중)
	if (Prey || bPreyRemembered)
	{
		const FVector PreyLocation = Prey ? Prey->GetActorLocation() : RememberedPreyLocation;
		const float Distance = FVector::Dist2D(MyLocation, PreyLocation);

		// 기억한 자리에 도착했는데 없으면 포기
		if (!Prey && Distance < 150.f)
		{
			RememberedPrey.Reset();
			Halt();
			return;
		}

		SetAIRun(*Me, Distance <= RunDistance * 2.f && Distance > 350.f);

		if (Now >= NextMoveCommandTime || GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			NextMoveCommandTime = Now + 0.25;
			if (Prey) { MoveToTarget(*Prey, GrabReach * 0.6f); }
			else { MoveToPoint(PreyLocation, 100.f); }
		}
		return;
	}

	// 4) 아무도 없을 때: 가까운 목표 아닌 상대를 가끔 밀고, 아니면 돌아다님
	if (Neutral && NeutralDistance <= PushReach && bPushReady && Me->IsInFrontOf(Neutral, Me->GetPushDistance(), 60.f) && FMath::FRand() < RandomPushChance)
	{
		Me->AIPush();
		LastPushTime = Now;
		return;
	}

	SetAIRun(*Me, false);
	if (GetMoveStatus() == EPathFollowingStatus::Idle && !bDirectMove && Now >= NextWanderTime)
	{
		NextWanderTime = Now + FMath::FRandRange(0.5f, 2.5f);
		MoveToPoint(PickWanderPoint(MyLocation), 120.f);
	}
}

void ACTAIController::MoveToPoint(const FVector& Destination, float AcceptRadius)
{
	bDirectMove = false;
	const EPathFollowingRequestResult::Type Result = MoveToLocation(Destination, AcceptRadius, /*bStopOnOverlap=*/true,
		/*bUsePathfinding=*/true, /*bProjectDestinationToNavigation=*/true, /*bCanStrafe=*/false);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		// 길찾기 실패(NavMesh 없음 등): 직선으로
		bDirectMove = true;
		DirectMoveTarget = Destination;
	}
}

void ACTAIController::MoveToTarget(const ACTCharacter& Target, float AcceptRadius)
{
	bDirectMove = false;
	const EPathFollowingRequestResult::Type Result = MoveToActor(const_cast<ACTCharacter*>(&Target), AcceptRadius, /*bStopOnOverlap=*/true,
		/*bUsePathfinding=*/true, /*bCanStrafe=*/false);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		bDirectMove = true;
		DirectMoveTarget = Target.GetActorLocation();
	}
}

void ACTAIController::SetAIRun(ACTCharacter& Me, bool bRun)
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	if (bRun)
	{
		if (Me.GetRunState() == ECTRunState::Idle) { RunStartedAt = Now; }
		Me.AISetRun(true);
		return;
	}

	// 달리는 중이고 막 시작했으면 잠깐 더 유지
	if (Me.GetRunState() == ECTRunState::Running && Now - RunStartedAt < RunMinHoldSeconds) { return; }
	Me.AISetRun(false);
}

void ACTAIController::Halt()
{
	bDirectMove = false;
	if (GetMoveStatus() != EPathFollowingStatus::Idle) { StopMovement(); }
}

void ACTAIController::FaceLocation(ACTCharacter& Me, const FVector& Location) const
{
	FVector Direction = Location - Me.GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero()) { return; }
	Me.SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
}

FVector ACTAIController::PickFleePoint(const FVector& MyLocation, const FVector& HunterLocation) const
{
	FVector Away = MyLocation - HunterLocation;
	Away.Z = 0.f;
	Away = Away.GetSafeNormal();
	if (Away.IsNearlyZero()) { Away = FVector::ForwardVector; }

	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (Nav)
	{
		// 갈 수 있는 곳을 여러 개 뽑아 추격자에게서 가장 먼 곳 (반대 방향을 선호)
		float BestScore = -TNumericLimits<float>::Max();
		FVector Best = FVector::ZeroVector;
		bool bFound = false;
		const FVector Center = GetPlayAreaCenter();
		for (int32 Sample = 0; Sample < 10; ++Sample)
		{
			FNavLocation Point;
			if (!Nav->GetRandomReachablePointInRadius(MyLocation, FleeSampleRadius, Point)) { continue; }
			if (IsNearHazardPath(Point.Location)) { continue; } // 도로, 통나무 길, 드럼통 근처는 피함

			FVector ToPoint = Point.Location - MyLocation;
			ToPoint.Z = 0.f;
			float Score = FVector::Dist2D(Point.Location, HunterLocation) + 400.f * FVector::DotProduct(ToPoint.GetSafeNormal(), Away);
			// 가운데에서 멀수록 점수를 깎아 가장자리로만 도망치지 않게 함
			if (bPreferMapCenter) { Score -= FleeCenterWeight * FVector::Dist2D(Point.Location, Center); }
			if (Score > BestScore)
			{
				BestScore = Score;
				Best = Point.Location;
				bFound = true;
			}
		}
		if (bFound) { return Best; }
	}

	// NavMesh가 없으면 반대 방향 직선
	return MyLocation + Away * FleeSampleRadius * 0.6f;
}

FVector ACTAIController::GetPlayAreaCenter() const
{
	if (bUseCenterOverride) { return CenterOverride; }
	if (bCenterCached) { return CachedCenter; }

	// NavMesh 범위(볼륨)들을 합친 상자의 가운데
	FBox Bounds(ForceInit);
	for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
	{
		Bounds += It->GetComponentsBoundingBox();
	}

	// 볼륨이 없으면 플레이어 시작점들의 평균
	if (!Bounds.IsValid)
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			Bounds += It->GetActorLocation();
		}
	}

	CachedCenter = Bounds.IsValid ? Bounds.GetCenter() : FVector::ZeroVector;
	bCenterCached = true;
	return CachedCenter;
}

FVector ACTAIController::PickWanderPoint(const FVector& MyLocation) const
{
	if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		const FVector Center = GetPlayAreaCenter();
		const bool bCenterMode = bPreferMapCenter;

		// 가운데에서 너무 멀어졌으면 가운데 구역에서 목적지를 고름
		if (bCenterMode && FVector::Dist2D(MyLocation, Center) > CenterPullDistance)
		{
			FNavLocation Pulled;
			for (int32 Attempt = 0; Attempt < 8; ++Attempt)
			{
				if (!Nav->GetRandomReachablePointInRadius(Center, CenterRoamRadius, Pulled)) { continue; }
				if (!IsNearHazardPath(Pulled.Location)) { return Pulled.Location; }
			}
		}

		// 여러 곳을 뽑아 가운데에 가까운 곳을 선호 (WanderCenterBias 만큼). 위험물 근처는 제외
		FNavLocation Point;
		FVector Fallback = MyLocation;
		float BestScore = -TNumericLimits<float>::Max();
		FVector Best = MyLocation;
		bool bFound = false;
		for (int32 Attempt = 0; Attempt < 8; ++Attempt)
		{
			if (!Nav->GetRandomReachablePointInRadius(MyLocation, WanderRadius, Point)) { continue; }
			Fallback = Point.Location;
			if (IsNearHazardPath(Point.Location)) { continue; } // 도로, 통나무 길, 드럼통 근처는 피함

			// 가까울수록 높은 점수 + 무작위. 편향이 0이면 완전 무작위
			const float DistToCenter = FVector::Dist2D(Point.Location, Center);
			const float CenterScore = bCenterMode ? -DistToCenter / FMath::Max(CenterRoamRadius, 1.f) : 0.f;
			const float Score = CenterScore * WanderCenterBias + FMath::FRand() * (1.f - WanderCenterBias);
			if (Score > BestScore)
			{
				BestScore = Score;
				Best = Point.Location;
				bFound = true;
			}
		}
		if (bFound) { return Best; }
		if (!Fallback.Equals(MyLocation)) { return Fallback; }
	}

	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	return MyLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * FMath::FRandRange(300.f, WanderRadius * 0.5f);
}

// 위험물 회피

void ACTAIController::RefreshHazardList(double Now)
{
	if (Now < NextHazardScanTime) { return; }
	NextHazardScanTime = Now + 0.4;

	UWorld* World = GetWorld();
	Hazards.Reset();
	for (TActorIterator<ACTRoadVehicle> It(World); It; ++It) { Hazards.Add(*It); }
	for (TActorIterator<ACTRollingLog> It(World); It; ++It) { Hazards.Add(*It); }
	for (TActorIterator<ACTExplosiveBarrel> It(World); It; ++It) { Hazards.Add(*It); }
	for (TActorIterator<ACTFallingProp> It(World); It; ++It) { Hazards.Add(*It); }

	Pickups.Reset();
	for (TActorIterator<ACTTailPickup> It(World); It; ++It) { Pickups.Add(*It); }
}

bool ACTAIController::AvoidHazards(ACTCharacter& Me, double Now)
{
	RefreshHazardList(Now);

	const float DeltaTime = static_cast<float>(FMath::Max(Now - LastHazardThinkTime, 0.02));
	LastHazardThinkTime = Now;

	const FVector MyLocation = Me.GetActorLocation();
	const float Radius = HazardAvoidRadius;

	// 가장 급한 위험물 하나만 피함
	float BestUrgency = -1.f;
	FVector BestEscape = FVector::ZeroVector;
	float BestRadius = Radius;

	for (const TWeakObjectPtr<AActor>& Weak : Hazards)
	{
		AActor* Hazard = Weak.Get();
		if (!Hazard) { continue; }

		// 위치와 종류별 위험 판단
		FVector HazardLocation = Hazard->GetActorLocation();
		bool bFalling = false;
		if (const ACTRoadVehicle* Vehicle = Cast<ACTRoadVehicle>(Hazard))
		{
			HazardLocation = Vehicle->GetCarLocation();
		}
		else if (const ACTRollingLog* Log = Cast<ACTRollingLog>(Hazard))
		{
			if (!Log->IsRolling()) { continue; } // 쉬는 동안은 안 보임, 위험 없음
			HazardLocation = Log->GetLogLocation();
		}
		else if (const ACTExplosiveBarrel* Barrel = Cast<ACTExplosiveBarrel>(Hazard))
		{
			FVector Away = MyLocation - HazardLocation;
			Away.Z = 0.f;
			const float Distance = Away.Size();
			// 터지기 직전이면 폭발 범위 밖으로, 아직이면 닿는 범위(도화선이 켜짐) 밖으로
			float Danger = 0.f;
			if (Barrel->GetBarrelState() == ECTBarrelState::Burning) { Danger = Barrel->GetExplosionRadius() + 150.f; }
			else if (Barrel->GetBarrelState() == ECTBarrelState::Idle) { Danger = Barrel->GetTouchRadius() + 80.f; }
			if (Danger > 0.f && Distance < Danger)
			{
				const float Urgency = 1.f + (Danger - Distance) / Danger;
				if (Urgency > BestUrgency)
				{
					BestUrgency = Urgency;
					BestEscape = Distance > 1.f ? Away / Distance : FVector::ForwardVector;
					BestRadius = Danger;
				}
			}
			continue;
		}
		else if (Cast<ACTFallingProp>(Hazard))
		{
			bFalling = true;
		}

		// 이전 위치와 비교해 속도 계산
		const FVector* Prev = HazardPrevLocations.Find(Weak);
		const FVector Velocity = Prev ? (HazardLocation - *Prev) / DeltaTime : FVector::ZeroVector;
		const bool bHadPrev = Prev != nullptr;
		HazardPrevLocations.Add(Weak, HazardLocation);
		if (!bHadPrev) { continue; }

		FVector Rel = MyLocation - HazardLocation;
		Rel.Z = 0.f;

		if (bFalling)
		{
			// 위에서 내려오는 물건: 바로 아래에서 옆으로 비킴
			if (Velocity.Z < -150.f && HazardLocation.Z > MyLocation.Z - 50.f && HazardLocation.Z < MyLocation.Z + 3000.f && Rel.Size() < 380.f)
			{
				const float Urgency = 1.5f;
				if (Urgency > BestUrgency)
				{
					BestUrgency = Urgency;
					BestEscape = Rel.IsNearlyZero() ? FVector::RightVector : Rel.GetSafeNormal();
					BestRadius = 380.f;
				}
			}
			continue;
		}

		const FVector Velocity2D(Velocity.X, Velocity.Y, 0.f);
		const float Speed = Velocity2D.Size();

		if (Speed < 50.f)
		{
			// 멈춰 있는 위험물 바로 옆: 그냥 떨어짐
			if (Rel.Size() < Radius * 0.7f)
			{
				const float Urgency = 1.f;
				if (Urgency > BestUrgency)
				{
					BestUrgency = Urgency;
					BestEscape = Rel.GetSafeNormal();
					BestRadius = Radius;
				}
			}
			continue;
		}

		// 진행 방향 기준: Along > 0 이면 내가 위험물보다 앞쪽(다가오는 중), Lateral 은 옆으로 떨어진 거리
		const FVector Direction = Velocity2D / Speed;
		const float Along = FVector::DotProduct(Rel, Direction);
		const FVector Lateral = Rel - Direction * Along;
		const float LateralDistance = Lateral.Size();
		const float Reach = Speed * HazardLookAheadSeconds + Radius;

		if (Along < -Radius * 0.6f || Along > Reach) { continue; } // 이미 지나갔거나 아직 멀다
		if (LateralDistance >= Radius) { continue; }               // 옆으로 충분히 비껴감

		// 가까울수록, 옆이 좁을수록 급함
		const float Urgency = 1.f + (1.f - FMath::Clamp(Along / Reach, 0.f, 1.f)) + (1.f - LateralDistance / Radius);
		if (Urgency > BestUrgency)
		{
			BestUrgency = Urgency;
			// 이미 있는 쪽 옆으로 비킴 (정면이면 진행 방향의 오른쪽)
			BestEscape = LateralDistance > 20.f ? Lateral / LateralDistance : FVector(-Direction.Y, Direction.X, 0.f);
			BestRadius = Radius;
		}
	}

	// 정리: 사라진 위험물 기록 제거
	for (auto It = HazardPrevLocations.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); }
	}

	if (BestUrgency < 0.f) { return false; }

	// 피할 곳: 위험 반경 밖. 달려서 빠르게
	Halt();
	SetAIRun(Me, true);
	const FVector Destination = MyLocation + BestEscape * (BestRadius + 300.f);
	FVector Target = Destination;
	if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation Projected;
		if (Nav->ProjectPointToNavigation(Destination, Projected, FVector(300.f, 300.f, 400.f))) { Target = Projected.Location; }
	}
	MoveToPoint(Target, 40.f);
	NextMoveCommandTime = Now + 0.3;
	return true;
}

bool ACTAIController::IsNearHazardPath(const FVector& Point) const
{
	const float Clearance = HazardPathClearance;
	const FVector2D P(Point.X, Point.Y);

	for (const TWeakObjectPtr<AActor>& Weak : Hazards)
	{
		const AActor* Hazard = Weak.Get();
		if (!Hazard) { continue; }

		if (const ACTRoadVehicle* Vehicle = Cast<ACTRoadVehicle>(Hazard))
		{
			if (const USplineComponent* Spline = Vehicle->GetPathSpline())
			{
				const FVector Closest = Spline->FindLocationClosestToWorldLocation(Point, ESplineCoordinateSpace::World);
				if (FVector2D::Distance(P, FVector2D(Closest.X, Closest.Y)) < Clearance) { return true; }
			}
		}
		else if (const ACTRollingLog* Log = Cast<ACTRollingLog>(Hazard))
		{
			const FVector A = Log->GetPathStartWorld();
			const FVector B = Log->GetPathEndWorld();
			const FVector2D Start(A.X, A.Y);
			const FVector2D Segment = FVector2D(B.X, B.Y) - Start;
			const float LengthSquared = Segment.SizeSquared();
			const float T = LengthSquared > 1.f ? FMath::Clamp(FVector2D::DotProduct(P - Start, Segment) / LengthSquared, 0.f, 1.f) : 0.f;
			if (FVector2D::Distance(P, Start + Segment * T) < Clearance) { return true; }
		}
		else if (const ACTExplosiveBarrel* Barrel = Cast<ACTExplosiveBarrel>(Hazard))
		{
			const FVector BarrelLocation = Barrel->GetActorLocation();
			if (FVector2D::Distance(P, FVector2D(BarrelLocation.X, BarrelLocation.Y)) < Barrel->GetTouchRadius() + 150.f) { return true; }
		}
	}
	return false;
}

// 꼬리 아이템

bool ACTAIController::SeesLocation(const ACTCharacter& Me, const FVector& Location, const AActor* IgnoredActor) const
{
	const FVector From = Me.GetActorLocation();
	const float Distance = FVector::Dist2D(From, Location);
	if (Distance > PickupSightRange) { return false; }

	// 가까우면 뒤쪽이어도 보임
	if (Distance > CloseSenseRadius)
	{
		FVector Forward = Me.GetActorForwardVector();
		Forward.Z = 0.f;
		FVector Direction = Location - From;
		Direction.Z = 0.f;
		if (!Forward.Normalize() || !Direction.Normalize()) { return false; }
		if (FVector::DotProduct(Forward, Direction) < FMath::Cos(FMath::DegreesToRadians(SightHalfAngleDegrees))) { return false; }
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTAIPickupSight), false);
	Params.AddIgnoredActor(&Me);
	if (IgnoredActor) { Params.AddIgnoredActor(IgnoredActor); }
	FHitResult Hit;
	const FVector Eye(0.f, 0.f, EyeHeight);
	return !GetWorld()->LineTraceSingleByChannel(Hit, From + Eye, Location + FVector(0.f, 0.f, 40.f), ECC_Visibility, Params);
}

void ACTAIController::ThinkTreasure(ACTCharacter& Me, double Now)
{
	// 위험물 회피가 가장 우선
	if (AvoidHazards(Me, Now)) { return; }

	// 에너지와 던지기 오브젝트 목록은 0.5초마다 다시 모음
	if (Now >= NextEnergyRefreshTime)
	{
		NextEnergyRefreshTime = Now + 0.5;
		EnergyList.Reset();
		for (TActorIterator<ACTEnergyPickup> It(GetWorld()); It; ++It) { EnergyList.Add(*It); }
		ThrowableList.Reset();
		for (TActorIterator<ACTThrowable> It(GetWorld()); It; ++It) { ThrowableList.Add(*It); }
	}

	const ACTPlayerState* MyState = GetPlayerState<ACTPlayerState>();
	const ACTGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;

	// 오브젝트를 들고 있으면: 가까운 상대 팀에게 던짐 (상대가 없으면 계속 들고 다님)
	if (Me.IsHoldingThrowable() && MyState && GameState)
	{
		if (HoldStartTime <= 0.0) { HoldStartTime = Now; }

		ACTCharacter* Target = nullptr;
		float BestScore = TNumericLimits<float>::Max();

		// 같은 팀이 던지는 길목에 있으면 던지지 않음 (아군 피해 방지)
		auto FriendlyInLine = [&](const FVector& To)
		{
			for (APlayerState* State : GameState->PlayerArray)
			{
				const ACTPlayerState* Mate = Cast<ACTPlayerState>(State);
				if (!Mate || Mate == MyState || Mate->GetTeamId() != MyState->GetTeamId()) { continue; }
				const AActor* MatePawn = Mate->GetPawn();
				if (!MatePawn) { continue; }
				const FVector Closest = FMath::ClosestPointOnSegment(MatePawn->GetActorLocation(), Me.GetActorLocation(), To);
				if (FVector::Dist2D(Closest, MatePawn->GetActorLocation()) < 150.f) { return true; }
			}
			return false;
		};
		for (APlayerState* State : GameState->PlayerArray)
		{
			const ACTPlayerState* Other = Cast<ACTPlayerState>(State);
			if (!Other || Other == MyState || Other->GetTeamId() == MyState->GetTeamId() || Other->IsForfeited() || Other->IsOnlyASpectator()) { continue; }

			ACTCharacter* OtherCharacter = Cast<ACTCharacter>(Other->GetPawn());
			if (!OtherCharacter) { continue; }
			const float Distance = FVector::Dist2D(Me.GetActorLocation(), OtherCharacter->GetActorLocation());
			if (Distance < 2500.f && (Now - HoldStartTime > 3.0 || SeesLocation(Me, OtherCharacter->GetActorLocation(), OtherCharacter)) && !FriendlyInLine(OtherCharacter->GetActorLocation()))
			{
				// 꼬리가 긴 상대를 먼저 노림 (상대 에너지는 몰라서 꼬리 길이로 짐작. 사람과 같은 정보)
				const float Score = Distance - 120.f * Other->GetTailCount();
				if (Score < BestScore) { Target = OtherCharacter; BestScore = Score; }
			}
		}

		if (Target && Now - HoldStartTime > 0.7)
		{
			Halt();
			SetAIRun(Me, false);
			FaceLocation(Me, Target->GetActorLocation());
			if (Now >= NextThrowTime)
			{
				NextThrowTime = Now + 1.0;
				Me.AIThrow();
				HoldStartTime = 0.0;
			}
			return;
		}
	}
	else
	{
		HoldStartTime = 0.0;

		// 손이 닿는 거리에 상대 팀이 있으면 잡아서 2초 멈추게 함 (쿨타임은 서버가 검사)
		if (MyState && GameState && Now >= NextThrowTime && !Me.IsBoothLocked())
		{
			for (APlayerState* State : GameState->PlayerArray)
			{
				const ACTPlayerState* Other = Cast<ACTPlayerState>(State);
				if (!Other || Other == MyState || Other->GetTeamId() == MyState->GetTeamId() || Other->IsForfeited() || Other->IsOnlyASpectator()) { continue; }
				ACTCharacter* OtherCharacter = Cast<ACTCharacter>(Other->GetPawn());
				if (!OtherCharacter || OtherCharacter->IsBoothLocked()) { continue; }
				if (FVector::Dist2D(Me.GetActorLocation(), OtherCharacter->GetActorLocation()) > Me.GetGrabDistance() * 1.05f) { continue; }

				NextThrowTime = Now + 1.5;
				Halt();
				SetAIRun(Me, false);
				FaceLocation(Me, OtherCharacter->GetActorLocation());
				Me.AIGrab();
				return;
			}
		}
	}

	const FVector MyLocation = Me.GetActorLocation();
	ACTEnergyPickup* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<ACTEnergyPickup>& Weak : EnergyList)
	{
		ACTEnergyPickup* Energy = Weak.Get();
		if (!Energy || Energy->IsActorBeingDestroyed()) { continue; }

		const float Distance = FVector::Dist2D(MyLocation, Energy->GetActorLocation());
		if (Distance < BestDistance) { Best = Energy; BestDistance = Distance; }
	}

	// 오브젝트를 안 들고 있고 가까이에 있으면 집으러 감 (에너지보다 가까울 때)
	if (!Me.IsHoldingThrowable())
	{
		ACTThrowable* NearThrowable = nullptr;
		float NearThrowableDistance = TNumericLimits<float>::Max();
		for (const TWeakObjectPtr<ACTThrowable>& Weak : ThrowableList)
		{
			ACTThrowable* Throwable = Weak.Get();
			if (!Throwable || !Throwable->CanBePickedUp()) { continue; }
			const float Distance = FVector::Dist2D(MyLocation, Throwable->GetActorLocation());
			if (Distance < NearThrowableDistance) { NearThrowable = Throwable; NearThrowableDistance = Distance; }
		}

		if (NearThrowable && NearThrowableDistance < 900.f && NearThrowableDistance < BestDistance)
		{
			if (NearThrowableDistance < 450.f)
			{
				// 바라보고 집기 (쿨타임은 서버가 검사)
				Halt();
				SetAIRun(Me, false);
				FaceLocation(Me, NearThrowable->GetActorLocation());
				Me.AIGrab();
			}
			else
			{
				SetAIRun(Me, false);
				if (Now >= NextMoveCommandTime || GetMoveStatus() == EPathFollowingStatus::Idle)
				{
					NextMoveCommandTime = Now + 0.2;
					MoveToPoint(NearThrowable->GetActorLocation(), 250.f);
				}
			}
			return;
		}
	}

	if (Best)
	{
		// 멀면 달려서, 가까우면 걸어서 정확히 닿음
		SetAIRun(Me, BestDistance > 350.f);
		if (Now >= NextMoveCommandTime || GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			NextMoveCommandTime = Now + 0.2;
			MoveToPoint(Best->GetActorLocation(), 10.f);
		}
		return;
	}

	// 에너지가 안 보이면 돌아다님
	SetAIRun(Me, false);
	if (GetMoveStatus() == EPathFollowingStatus::Idle && !bDirectMove && Now >= NextWanderTime)
	{
		NextWanderTime = Now + FMath::FRandRange(0.3f, 1.5f);
		MoveToPoint(PickWanderPoint(MyLocation), 120.f);
	}
}

bool ACTAIController::GoToTailPickup(ACTCharacter& Me, double Now, bool bIgnoreHazards)
{
	const FVector MyLocation = Me.GetActorLocation();

	// 보이는 것 중 가장 가까운 아이템
	ACTTailPickup* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<ACTTailPickup>& Weak : Pickups)
	{
		ACTTailPickup* Pickup = Weak.Get();
		if (!Pickup || Pickup->IsActorBeingDestroyed()) { continue; }

		const FVector Location = Pickup->GetActorLocation();
		const float Distance = FVector::Dist2D(MyLocation, Location);
		if (Distance >= BestDistance) { continue; }
		if (!bIgnoreHazards && IsNearHazardPath(Location)) { continue; } // 도로 한가운데 같은 곳은 살아 있을 땐 포기
		if (!SeesLocation(Me, Location, Pickup)) { continue; }

		Best = Pickup;
		BestDistance = Distance;
	}

	FVector Goal = FVector::ZeroVector;
	if (Best)
	{
		RememberedPickup = Best;
		LastPickupSeenTime = Now;
		Goal = Best->GetActorLocation();
	}
	else if (RememberedPickup.IsValid() && (Now - LastPickupSeenTime) < PickupMemorySeconds)
	{
		// 시야에서 놓쳐도(방향을 틀었거나 가려짐) 잠깐은 그 자리로 계속
		Goal = RememberedPickup->GetActorLocation();
	}
	else
	{
		RememberedPickup.Reset();
		return false;
	}

	// 달려서 가고, 가까우면 걸어서 정확히 닿음
	const float Distance = FVector::Dist2D(MyLocation, Goal);
	SetAIRun(Me, Distance > 350.f);
	if (Now >= NextMoveCommandTime || GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		NextMoveCommandTime = Now + 0.2;
		MoveToPoint(Goal, 10.f);
	}
	return true;
}

// 추격자 의심: 나를 노리는 사람이 누군지는 모르므로, 나에게 계속 다가오는 사람을 의심함

bool ACTAIController::UpdateSuspicion(ACTCharacter& Other, const FVector& MyLocation, float DeltaTime, double Now)
{
	const TWeakObjectPtr<ACTCharacter> Key(&Other);
	FSuspect& Suspect = Suspects.FindOrAdd(Key);

	const FVector OtherLocation = Other.GetActorLocation();
	const bool bHadPrev = Suspect.LastSeenTime > 0.0 && (Now - Suspect.LastSeenTime) < 1.0;
	const FVector Velocity = bHadPrev ? (OtherLocation - Suspect.LastLocation) / DeltaTime : FVector::ZeroVector;
	Suspect.LastLocation = OtherLocation;
	Suspect.LastSeenTime = Now;
	Suspect.bTouched = true;

	FVector ToMe = MyLocation - OtherLocation;
	ToMe.Z = 0.f;
	const float Distance = ToMe.Size();
	FVector Velocity2D(Velocity.X, Velocity.Y, 0.f);
	const float Speed = Velocity2D.Size();

	bool bApproaching = false;
	if (Distance > 1.f && Distance <= SuspectRange && Speed > 80.f)
	{
		const FVector ToMeDir = ToMe / Distance;
		const float Closing = FVector::DotProduct(Velocity2D, ToMeDir);
		const float Alignment = Closing / Speed; // 1이면 똑바로 나를 향해 옴
		bApproaching = Closing >= SuspectClosingSpeed && Alignment >= 0.8f;
	}

	if (bApproaching)
	{
		// 가까울수록 더 빨리 확신
		const float Boost = Distance < 400.f ? 2.f : 1.f;
		Suspect.Score += DeltaTime * Boost;
	}
	else
	{
		Suspect.Score = FMath::Max(Suspect.Score - DeltaTime * 0.6f, 0.f);
	}

	return Suspect.Score >= SuspectSeconds;
}

void ACTAIController::DecayUnseenSuspects(float DeltaTime, double Now)
{
	for (auto It = Suspects.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || (Now - It.Value().LastSeenTime) > 5.0)
		{
			It.RemoveCurrent();
			continue;
		}
		if (It.Value().bTouched)
		{
			It.Value().bTouched = false;
		}
		else
		{
			// 이번에 안 보인 사람은 의심이 천천히 식음
			It.Value().Score = FMath::Max(It.Value().Score - DeltaTime * 0.6f, 0.f);
		}
	}
}
