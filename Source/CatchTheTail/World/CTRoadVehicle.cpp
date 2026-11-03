// Copyright Moon Party Games. All Rights Reserved.
#include "CTRoadVehicle.h"
#include "CTTailPickup.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

ACTRoadVehicle::ACTRoadVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	// 위치는 서버 시간으로 각 화면이 계산하므로 복제하지 않고 레벨에 놓인 채로 양쪽에 존재
	bReplicates = false;
	SetReplicatingMovement(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Path = CreateDefaultSubobject<USplineComponent>(TEXT("Path"));
	Path->SetupAttachment(Root);
	Path->SetClosedLoop(true);

	CarMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CarMesh"));
	CarMesh->SetupAttachment(Root);
	CarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 판정은 코드가 직접 함
	CarMesh->SetGenerateOverlapEvents(false);
	CarMesh->SetUsingAbsoluteLocation(true);
	CarMesh->SetUsingAbsoluteRotation(true);

	DroppedTailClass = ACTTailPickup::StaticClass();
}

void ACTRoadVehicle::BeginPlay()
{
	Super::BeginPlay();
}

void ACTRoadVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState) { return; }

	const bool bPlaying = CTGameState->GetPhase() == ECTGamePhase::Playing;
	if (!bPlaying && !bDriveOutsideMatch) { return; }

	const float Length = Path->GetSplineLength();
	if (Length < 1.f) { return; }

	// 서버 시간으로 경로 위의 거리를 정함. 모든 화면이 같은 자리에 차를 그림
	const double Time = CTGameState->GetServerWorldTimeSeconds() + PhaseOffsetSeconds;
	double Distance = FMath::Fmod(Time * DriveSpeed, static_cast<double>(Length));
	if (Distance < 0.0) { Distance += Length; }
	if (bReverse) { Distance = Length - Distance; }

	FVector Location = Path->GetLocationAtDistanceAlongSpline(static_cast<float>(Distance), ESplineCoordinateSpace::World);
	FVector Tangent = Path->GetDirectionAtDistanceAlongSpline(static_cast<float>(Distance), ESplineCoordinateSpace::World);
	if (bReverse) { Tangent = -Tangent; }
	Tangent.Z = 0.f;
	const FQuat Rotation = Tangent.GetSafeNormal().Rotation().Quaternion();
	Location.Z += HeightOffset;

	CarMesh->SetWorldLocationAndRotation(Location, Rotation);

	if (HasAuthority() && bPlaying)
	{
		ApplyHits(Location, Rotation);
	}
}

void ACTRoadVehicle::ApplyHits(const FVector& Location, const FQuat& Rotation)
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTRoadVehicleHit), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);

	const FVector Center = Location + FVector(0.f, 0.f, HitCenterHeight);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, Rotation, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeBox(HitHalfExtent), Params);

	const FVector Forward = Rotation.GetForwardVector();
	const FVector Right = Rotation.GetRightVector();
	const double Now = World->GetTimeSeconds();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACTCharacter* Character = Cast<ACTCharacter>(Overlap.GetActor());
		if (!Character || Character->IsGhost()) { continue; }

		const double* LastHit = LastHitTimes.Find(Character);
		if (LastHit && Now - *LastHit < HitCooldownSeconds) { continue; }
		LastHitTimes.Add(Character, Now);

		// 차 진행 방향으로 날리되, 차가 있는 선에서 비켜나도록 캐릭터가 있는 옆으로도 밀어냄
		const float Side = FVector::DotProduct(Character->GetActorLocation() - Location, Right) >= 0.f ? 1.f : -1.f;
		const FVector Launch = Forward * KnockbackForwardSpeed + Right * Side * KnockbackSideSpeed;
		const FVector Push = Launch.GetSafeNormal2D();

		Character->LaunchCharacter(FVector(Launch.X, Launch.Y, KnockbackUpSpeed), /*bXYOverride=*/true, /*bZOverride=*/true);

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

		UE_LOG(LogTemp, Log, TEXT("[CT] 자동차 충돌: %s 가 %s 에 치여 튕겨남 (꼬리 -%d)"), *Character->GetName(), *GetName(), Lost);
		BP_OnHitCharacter(Character);
	}

	for (auto It = LastHitTimes.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); }
	}
}
