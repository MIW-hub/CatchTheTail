// Copyright Moon Party Games. All Rights Reserved.
#include "CTRollingLog.h"
#include "CTTailPickup.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACTRollingLog::ACTRollingLog()
{
	PrimaryActorTick.bCanEverTick = true;

	// 위치는 서버 시간으로 각 화면이 계산하므로 복제하지 않고 레벨에 놓인 채로 양쪽에 존재
	bReplicates = false;
	SetReplicatingMovement(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 판정은 코드가 직접 함
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded()) { Mesh->SetStaticMesh(CylinderFinder.Object); }

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LogMaterialFinder(TEXT("/Game/Material/tile/MI_Tile_Orange.MI_Tile_Orange"));
	if (LogMaterialFinder.Succeeded()) { LogMaterial = LogMaterialFinder.Object; }

	DroppedTailClass = ACTTailPickup::StaticClass();
}

void ACTRollingLog::BeginPlay()
{
	Super::BeginPlay();

	// 경로: 시작 XY → 끝 XY (높이는 지형을 따라가므로 수평 방향만 씀). 시작 위치는 움직이기 전에 저장
	PathStart = GetActorLocation();
	const FVector Start = GetPathStart();
	const FVector End = GetPathEndWorld();
	const FVector Flat = FVector(End.X - Start.X, End.Y - Start.Y, 0.f);
	PathLength = FMath::Max(Flat.Size(), 1.f);
	PathDirection = Flat.GetSafeNormal();
	if (PathDirection.IsNearlyZero()) { PathDirection = FVector::ForwardVector; }

	// 지름 100, 높이 100인 기본 원기둥을 원하는 크기로. Z축이 길이 방향
	Mesh->SetWorldScale3D(FVector(LogRadius * 2.f / 100.f, LogRadius * 2.f / 100.f, LogLength / 100.f));
	if (LogMaterial) { Mesh->SetMaterial(0, LogMaterial); }

	SetActorHiddenInGame(true);
}

bool ACTRollingLog::GetRollState(double ServerTime, float& OutDistance) const
{
	const float TravelSeconds = PathLength / FMath::Max(RollSpeed, 1.f);
	const float CycleSeconds = TravelSeconds + FMath::Max(WaitSeconds, 0.f);

	const double Phase = FMath::Fmod(ServerTime + PhaseOffsetSeconds, static_cast<double>(CycleSeconds));
	const double Wrapped = Phase < 0.0 ? Phase + CycleSeconds : Phase;
	if (Wrapped >= TravelSeconds) { return false; }

	OutDistance = static_cast<float>(Wrapped) * RollSpeed;
	return true;
}

float ACTRollingLog::FindGroundZ(const FVector& XY, float FallbackZ) const
{
	const UWorld* World = GetWorld();
	if (!World) { return FallbackZ; }

	// 정적인 지형과 구조물만 바닥으로 봄 (캐릭터, 떨어진 물체는 무시)
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTRollingLogGround), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);

	const float TraceTop = FMath::Max(FallbackZ, GetActorLocation().Z) + 3000.f;
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, FVector(XY.X, XY.Y, TraceTop), FVector(XY.X, XY.Y, TraceTop - 12000.f), ObjectParams, Params))
	{
		return Hit.ImpactPoint.Z;
	}
	return FallbackZ;
}

void ACTRollingLog::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState) { return; }

	// 경기 중에만 굴러감
	float Distance = 0.f;
	const bool bRolling = CTGameState->GetPhase() == ECTGamePhase::Playing && GetRollState(CTGameState->GetServerWorldTimeSeconds(), Distance);

	if (bRolling != bWasRolling)
	{
		bWasRolling = bRolling;
		SetActorHiddenInGame(!bRolling);
		bZInitialized = false; // 다시 시작할 때 바닥 높이부터 맞춤
		LastHitTimes.Reset();
	}
	if (!bRolling) { return; }

	// 경로 위의 현재 XY, 지형을 따라 높이 결정
	const FVector Start = GetPathStart();
	const FVector XY = Start + PathDirection * Distance;
	const float GroundZ = FindGroundZ(XY, bZInitialized ? CurrentZ : Start.Z);
	const float TargetZ = GroundZ + LogRadius;
	CurrentZ = bZInitialized ? FMath::FInterpTo(CurrentZ, TargetZ, DeltaSeconds, GroundFollowSpeed) : TargetZ;
	bZInitialized = true;

	// 진행 방향이 앞(X), 옆(Y)이 통나무 길이 방향. 원기둥 Z축을 옆으로 눕히고 이동 거리만큼 굴림
	const FRotator Heading = PathDirection.Rotation();
	const FVector Lateral = FRotationMatrix(Heading).GetUnitAxis(EAxis::Y);
	const float RollRadians = Distance / LogRadius;
	const FQuat RollQuat(Lateral, RollRadians);
	const FQuat LieDown = FQuat(FRotator(0.f, 0.f, 90.f));
	SetActorLocationAndRotation(FVector(XY.X, XY.Y, CurrentZ), RollQuat * Heading.Quaternion() * LieDown);

	if (HasAuthority())
	{
		ApplyHits(GetActorLocation(), PathDirection, Lateral);
	}
}

void ACTRollingLog::ApplyHits(const FVector& Location, const FVector& Direction, const FVector& Lateral)
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	// 옆으로 누운 통나무 모양의 캡슐
	const float HalfLength = LogLength * 0.5f * HitRangeScale;
	const float Radius = LogRadius * HitRangeScale;
	const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(Lateral).ToQuat();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTRollingLogHit), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Location, CapsuleRotation, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeCapsule(Radius, FMath::Max(HalfLength, Radius)), Params);

	const double Now = World->GetTimeSeconds();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACTCharacter* Character = Cast<ACTCharacter>(Overlap.GetActor());
		if (!Character || Character->IsGhost()) { continue; }

		const double* LastHit = LastHitTimes.Find(Character);
		if (LastHit && Now - *LastHit < HitCooldownSeconds) { continue; }
		LastHitTimes.Add(Character, Now);

		// 통나무 진행 방향의 옆으로 튕김. 어느 쪽 옆인지는 캐릭터가 있는 쪽
		const FVector ToCharacter = Character->GetActorLocation() - Location;
		const float Side = FVector::DotProduct(ToCharacter, Lateral) >= 0.f ? 1.f : -1.f;
		const FVector Push = (Lateral * Side * 0.85f + Direction * 0.15f).GetSafeNormal();

		Character->LaunchCharacter(Push * KnockbackSideSpeed + FVector(0.f, 0.f, KnockbackUpSpeed), /*bXYOverride=*/true, /*bZOverride=*/true);

		if (bPlayPushedReaction)
		{
			Character->PlayKnockedDown(Push);
		}

		int32 Lost = 0;
		if (bHitDropsTail)
		{
			Lost = ACTTailPickup::DropTailsFromCharacter(Character, HitTailLoss, Push, DroppedTailClass,
				DroppedTailLaunchSpeed, DroppedTailUpSpeed, DroppedTailPickupDelay, DroppedTailLifeSeconds, this);
		}

		UE_LOG(LogTemp, Log, TEXT("[CT] 통나무 충돌: %s 가 %s 에 맞아 튕겨남 (꼬리 -%d)"), *Character->GetName(), *GetName(), Lost);
		BP_OnHitCharacter(Character);
	}

	// 사라진 캐릭터 기록 정리
	for (auto It = LastHitTimes.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); }
	}
}
