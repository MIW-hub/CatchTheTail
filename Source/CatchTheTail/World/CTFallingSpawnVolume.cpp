// Copyright Moon Party Games. All Rights Reserved.
#include "CTFallingSpawnVolume.h"
#include "CTFallingProp.h"
#include "CTTailPickup.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/OverlapResult.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACTFallingSpawnVolume::ACTFallingSpawnVolume()
{
	PrimaryActorTick.bCanEverTick = true;

	// 멀리서도 복제 (그림자, 머티리얼)
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10.f);

	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	SetRootComponent(SpawnBox);
	SpawnBox->SetBoxExtent(FVector(1500.f, 1500.f, 1000.f));
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBox->SetHiddenInGame(true);
	SpawnBox->ShapeColor = FColor(120, 190, 255);

	DroppedTailClass = ACTTailPickup::StaticClass();

	// 기본 그림자 머티리얼 (디칼)
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShadowFinder(TEXT("/Game/Material/M_FallingShadowDecal.M_FallingShadowDecal"));
	if (ShadowFinder.Succeeded()) { ShadowMaterial = ShadowFinder.Object; }
}

void ACTFallingSpawnVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTFallingSpawnVolume, Entries);
}

void ACTFallingSpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		bSpawning = bAutoStart;
		TimeToNextSpawn = PickInterval();
	}
}

void ACTFallingSpawnVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TPair<int32, FLocalFall>& Pair : LocalFalls)
	{
		DestroyLocalFall(Pair.Value);
	}
	LocalFalls.Reset();

	for (const TWeakObjectPtr<AActor>& Item : LandedActors)
	{
		if (AActor* Actor = Item.Get()) { Actor->OnDestroyed.RemoveAll(this); }
	}
	LandedActors.Reset();

	Super::EndPlay(EndPlayReason);
}

void ACTFallingSpawnVolume::StartSpawning()
{
	if (!HasAuthority()) { return; }
	bSpawning = true;
}

void ACTFallingSpawnVolume::StopSpawning()
{
	if (!HasAuthority()) { return; }
	bSpawning = false;
}

float ACTFallingSpawnVolume::PickInterval() const
{
	const float MinInterval = FMath::Max(SpawnIntervalMin, 0.05f);
	const float MaxInterval = FMath::Max(SpawnIntervalMax, MinInterval);
	return FMath::FRandRange(MinInterval, MaxInterval);
}

// 스폰 (서버)

float ACTFallingSpawnVolume::FindGroundZ(const FVector& From) const
{
	const FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
	const float BoxBottomZ = SpawnBox->GetComponentLocation().Z - BoxExtent.Z;

	// 맨 위에서 아래로 충분히 멀리 검사
	const float TraceDistance = BoxExtent.Z * 2.f + 20000.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTFallingGround), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, From, From - FVector(0.f, 0.f, TraceDistance), GroundTraceChannel, Params))
	{
		return Hit.ImpactPoint.Z;
	}

	return BoxBottomZ;
}

AActor* ACTFallingSpawnVolume::SpawnOne()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World) { return nullptr; }
	if (Entries.Num() >= MaxFallingActors) { return nullptr; }

	// 클래스, 메시 한 묶음에서 무작위 (빈 칸 제외)
	TArray<TSubclassOf<AActor>> ClassCandidates;
	for (const TSubclassOf<AActor>& Candidate : ActorClasses)
	{
		if (Candidate) { ClassCandidates.Add(Candidate); }
	}

	TArray<UStaticMesh*> MeshCandidates;
	for (UStaticMesh* Candidate : StaticMeshes)
	{
		if (Candidate) { MeshCandidates.Add(Candidate); }
	}

	const int32 TotalCandidates = ClassCandidates.Num() + MeshCandidates.Num();
	if (TotalCandidates == 0) { return nullptr; }

	const int32 PickedIndex = FMath::RandRange(0, TotalCandidates - 1);
	const bool bPickedMesh = PickedIndex >= ClassCandidates.Num();
	UStaticMesh* PickedMesh = bPickedMesh ? MeshCandidates[PickedIndex - ClassCandidates.Num()] : nullptr;
	const TSubclassOf<AActor> ChosenClass = bPickedMesh ? TSubclassOf<AActor>(ACTFallingProp::StaticClass()) : ClassCandidates[PickedIndex];

	// 박스 안의 임의의 XY, 박스 맨 위 높이에서 시작
	const FVector BoxExtent = SpawnBox->GetUnscaledBoxExtent();
	const FVector LocalPoint(FMath::FRandRange(-BoxExtent.X, BoxExtent.X), FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y), BoxExtent.Z);
	const FVector SpawnLocation = SpawnBox->GetComponentTransform().TransformPosition(LocalPoint);
	const FRotator SpawnRotation = bRandomYaw ? FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f) : FRotator::ZeroRotator;
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	// 시작부터 복제되게 BeginPlay 전 설정
	AActor* Spawned = World->SpawnActorDeferred<AActor>(ChosenClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Spawned) { return nullptr; }

	Spawned->SetReplicates(true);
	Spawned->SetReplicateMovement(true);
	// 메시, 크기 주입 (FinishSpawning 전)
	if (ACTFallingProp* Prop = Cast<ACTFallingProp>(Spawned))
	{
		const float ScaleLow = FMath::Min(StaticMeshScaleMin, StaticMeshScaleMax);
		const float ScaleHigh = FMath::Max(StaticMeshScaleMin, StaticMeshScaleMax);
		Prop->Setup(PickedMesh, FMath::FRandRange(ScaleLow, ScaleHigh));
	}

	if (USceneComponent* SpawnedRoot = Spawned->GetRootComponent())
	{
		SpawnedRoot->SetMobility(EComponentMobility::Movable);
	}
	Spawned->FinishSpawning(SpawnTransform);

	// 낙하 충돌 (클라는 TickVisuals에서 적용)
	if (bControlCollision)
	{
		ApplyCollision(Spawned, CollisionWhileFalling);
	}

	// 크기, 바닥으로 그림자 결정
	FVector BoundsOrigin, BoundsExtent;
	Spawned->GetActorBounds(/*bOnlyCollidingComponents=*/false, BoundsOrigin, BoundsExtent);

	FCTFallEntry Entry;
	Entry.Actor = Spawned;
	Entry.Id = NextEntryId++;
	Entry.StartZ = SpawnLocation.Z;
	Entry.ShadowRadius = FMath::Max(FMath::Max(BoundsExtent.X, BoundsExtent.Y) * ShadowRadiusMultiplier, 20.f);
	Entry.MaterialIndex = Materials.IsEmpty() ? INDEX_NONE : FMath::RandRange(0, Materials.Num() - 1);

	const float GroundZ = FindGroundZ(SpawnLocation);
	Entry.GroundLocation = FVector(SpawnLocation.X, SpawnLocation.Y, GroundZ);

	// 선형보간 낙하: 착지 시 기준점 높이(EndZ)
	const float PivotToBottom = Spawned->GetActorLocation().Z - (BoundsOrigin.Z - BoundsExtent.Z);

	FServerFall Fall;
	Fall.Speed = FMath::FRandRange(FMath::Min(FallSpeedMin, FallSpeedMax), FMath::Max(FallSpeedMin, FallSpeedMax));
	Fall.StartZ = SpawnLocation.Z;
	Fall.EndZ = GroundZ + PivotToBottom;
	Fall.XY = FVector2D(SpawnLocation.X, SpawnLocation.Y);
	Fall.Duration = FMath::Max((Fall.StartZ - Fall.EndZ) / FMath::Max(Fall.Speed, 1.f), 0.05f);

	// 기울어 시작해 착지 때 반듯해짐
	const float TiltPitch = FMath::FRandRange(-FallTiltDegrees, FallTiltDegrees);
	const float TiltRoll = FMath::FRandRange(-FallTiltDegrees, FallTiltDegrees);
	const float SpinYaw = FMath::FRandRange(-FallSpinDegrees, FallSpinDegrees);
	Fall.StartRotation = FRotator(TiltPitch, SpawnRotation.Yaw, TiltRoll).Quaternion();
	Fall.EndRotation = FRotator(0.f, SpawnRotation.Yaw + SpinYaw, 0.f).Quaternion();
	ServerFalls.Add(Entry.Id, Fall);
	Spawned->SetActorRotation(Fall.StartRotation);

	ApplyMaterial(Spawned, Entry.MaterialIndex);

	UE_LOG(LogTemp, Log, TEXT("[CT] 낙하 스폰: %s  시작 Z=%.0f  바닥 Z=%.0f  속도=%.0f  머티리얼=%d  충돌=%s  크기=%.2f"),
		*Spawned->GetClass()->GetName(), SpawnLocation.Z, GroundZ, ServerFalls.FindRef(Entry.Id).Speed, Entry.MaterialIndex,
		Spawned->GetRootComponent() && Cast<UPrimitiveComponent>(Spawned->GetRootComponent()) ? *Cast<UPrimitiveComponent>(Spawned->GetRootComponent())->GetCollisionProfileName().ToString() : TEXT("-"), Spawned->GetActorScale3D().X);

	Entries.Add(Entry);
	ForceNetUpdate();
	SyncLocalVisuals(); // 서버 화면용

	return Spawned;
}

// 매 프레임

void ACTFallingSpawnVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		TickServer(DeltaSeconds);
	}

	TickVisuals();
}

void ACTFallingSpawnVolume::TickServer(float DeltaSeconds)
{
	bool bChanged = false;

	// 낙하
	for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
	{
		const FCTFallEntry Entry = Entries[Index];
		AActor* Falling = Entry.Actor;

		// 사라진 액터 제거
		FServerFall* Fall = ServerFalls.Find(Entry.Id);
		if (!IsValid(Falling) || !Fall)
		{
			ServerFalls.Remove(Entry.Id);
			Entries.RemoveAt(Index);
			bChanged = true;
			continue;
		}

		// 시작~바닥 선형보간, 기울기 동시
		Fall->Elapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(Fall->Elapsed / Fall->Duration, 0.f, 1.f);
		const float NewZ = FMath::Lerp(Fall->StartZ, Fall->EndZ, Alpha);
		const FQuat NewRotation = FQuat::Slerp(Fall->StartRotation, Fall->EndRotation, Alpha);
		Falling->SetActorLocationAndRotation(FVector(Fall->XY.X, Fall->XY.Y, NewZ), NewRotation, /*bSweep=*/false);

		// 액터 점유 영역
		FVector BoundsOrigin, BoundsExtent;
		Falling->GetActorBounds(/*bOnlyCollidingComponents=*/false, BoundsOrigin, BoundsExtent);

		// 적중 캐릭터 튕김
		if (bKnockbackOnHit)
		{
			ApplyHitKnockback(Entry, Falling, BoundsOrigin, BoundsExtent);
		}

		if (Alpha >= 1.f)
		{
			const bool bWasSettle = Fall->bSettle;

			// 착지 충돌 (클라는 목록 제거 시 적용)
			if (bControlCollision)
			{
				ApplyCollision(Falling, CollisionAfterLanding);
			}

			// 수명은 처음 착지할 때만 정함 (다시 내려와도 늘리지 않음)
			if (!bWasSettle && LandedLifeSeconds > 0.f)
			{
				Falling->SetLifeSpan(LandedLifeSeconds);
			}

			if (bSettleWhenUnsupported)
			{
				if (!bWasSettle)
				{
					LandedActors.AddUnique(Falling);
					Falling->OnDestroyed.AddUniqueDynamic(this, &ACTFallingSpawnVolume::HandleLandedActorDestroyed);
				}
				// 내려앉은 자리에 받침이 있는지 한 번 더 확인
				PendingCheckActors.Add(Falling);
			}

			ServerFalls.Remove(Entry.Id);
			Entries.RemoveAt(Index);
			bChanged = true;

			UE_LOG(LogTemp, Log, TEXT("[CT] 낙하 착지: %s  Z=%.0f  충돌=%s  카메라=%s"), *Falling->GetClass()->GetName(), Falling->GetActorLocation().Z,
			Falling->GetRootComponent() && Cast<UPrimitiveComponent>(Falling->GetRootComponent()) ? *Cast<UPrimitiveComponent>(Falling->GetRootComponent())->GetCollisionProfileName().ToString() : TEXT("-"),
				Falling->GetRootComponent() && Cast<UPrimitiveComponent>(Falling->GetRootComponent()) ? (Cast<UPrimitiveComponent>(Falling->GetRootComponent())->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore ? TEXT("무시") : TEXT("막음")) : TEXT("-"));
			OnActorLanded.Broadcast(Falling, Falling->GetActorLocation());
		}
	}

	// 받침을 잃은 낙하물 내려오기
	if (ProcessSupportChecks())
	{
		bChanged = true;
	}

	if (bChanged)
	{
		ForceNetUpdate();
		SyncLocalVisuals();
	}

	// 새로 낙하
	if (!bSpawning) { return; }

	if (bOnlySpawnWhilePlaying)
	{
		const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
		if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }
	}

	TimeToNextSpawn -= DeltaSeconds;
	if (TimeToNextSpawn <= 0.f)
	{
		// 가득 차면 다음 프레임 재시도
		if (SpawnOne())
		{
			TimeToNextSpawn = PickInterval();
		}
		else
		{
			TimeToNextSpawn = 0.f;
		}
	}
}

// 받침을 잃은 낙하물 (서버)

void ACTFallingSpawnVolume::HandleLandedActorDestroyed(AActor* DestroyedActor)
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !DestroyedActor || !World || World->bIsTearingDown) { return; }

	LandedActors.Remove(DestroyedActor);

	// 사라진 자리 위에 있던 낙하물을 확인할 구역
	FVector Origin, Extent;
	DestroyedActor->GetActorBounds(false, Origin, Extent);

	FSupportRegion Region;
	Region.Footprint = FBox2D(FVector2D(Origin.X - Extent.X, Origin.Y - Extent.Y), FVector2D(Origin.X + Extent.X, Origin.Y + Extent.Y));
	Region.BottomZ = Origin.Z - Extent.Z;
	PendingRegions.Add(Region);
}

bool ACTFallingSpawnVolume::ProcessSupportChecks()
{
	if (PendingRegions.IsEmpty() && PendingCheckActors.IsEmpty()) { return false; }

	LandedActors.RemoveAll([](const TWeakObjectPtr<AActor>& Item) { return !Item.IsValid(); });

	// 확인할 낙하물: 방금 착지한 것 + 사라진 자리 주변에서 같거나 더 높은 곳에 있는 것
	TArray<AActor*> Candidates;
	for (const TWeakObjectPtr<AActor>& Item : PendingCheckActors)
	{
		if (AActor* Actor = Item.Get()) { Candidates.AddUnique(Actor); }
	}
	PendingCheckActors.Reset();

	for (const FSupportRegion& Region : PendingRegions)
	{
		const FBox2D Expanded = Region.Footprint.ExpandBy(SettleSearchMargin);
		for (const TWeakObjectPtr<AActor>& Item : LandedActors)
		{
			AActor* Actor = Item.Get();
			if (!Actor) { continue; }

			FVector Origin, Extent;
			Actor->GetActorBounds(false, Origin, Extent);
			if (Origin.Z - Extent.Z < Region.BottomZ - 5.f) { continue; }

			const FBox2D Footprint(FVector2D(Origin.X - Extent.X, Origin.Y - Extent.Y), FVector2D(Origin.X + Extent.X, Origin.Y + Extent.Y));
			if (Expanded.Intersect(Footprint)) { Candidates.AddUnique(Actor); }
		}
	}
	PendingRegions.Reset();

	// 아래 것부터 처리해야 위 것이 아래 것의 도착 높이를 참고할 수 있음
	auto BottomOf = [](const AActor* Actor)
	{
		FVector Origin, Extent;
		Actor->GetActorBounds(false, Origin, Extent);
		return Origin.Z - Extent.Z;
	};
	Candidates.Sort([&BottomOf](const AActor& A, const AActor& B) { return BottomOf(&A) < BottomOf(&B); });

	bool bStarted = false;
	for (AActor* Actor : Candidates)
	{
		if (StartSettleIfUnsupported(Actor)) { bStarted = true; }
	}
	return bStarted;
}

bool ACTFallingSpawnVolume::StartSettleIfUnsupported(AActor* Actor)
{
	UWorld* World = GetWorld();
	if (!IsValid(Actor) || !World) { return false; }

	// 이미 내려오는 중이면 건너뜀
	if (Entries.ContainsByPredicate([Actor](const FCTFallEntry& Entry) { return Entry.Actor == Actor; })) { return false; }

	FVector Origin, Extent;
	Actor->GetActorBounds(false, Origin, Extent);
	const float BottomZ = Origin.Z - Extent.Z;

	// 바닥 면 모양: 메시가 있으면 회전을 반영한 실제 크기, 없으면 경계 상자
	FVector BoxHalf = Extent;
	FVector BoxCenter = Origin;
	FQuat BoxRotation = FQuat::Identity;
	if (const UStaticMeshComponent* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>())
	{
		if (const UStaticMesh* StaticMesh = Mesh->GetStaticMesh())
		{
			const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
			BoxHalf = MeshBounds.BoxExtent * Mesh->GetComponentScale().GetAbs();
			BoxCenter = Mesh->GetComponentTransform().TransformPosition(MeshBounds.Origin);
			BoxRotation = Mesh->GetComponentQuat();
		}
	}

	// 바닥 면 바로 위에서 아래로 훑어 받침 높이를 찾음 (가장자리만 걸쳐도 받쳐진 것으로 봄)
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTFallingSettle), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Actor);

	const FVector SweepStart(BoxCenter.X, BoxCenter.Y, BottomZ + 3.f);
	const FVector SweepEnd = SweepStart - FVector(0.f, 0.f, 30000.f);
	const FCollisionShape Shape = FCollisionShape::MakeBox(FVector(BoxHalf.X * 0.9f, BoxHalf.Y * 0.9f, 1.f));

	float SurfaceZ = SpawnBox->GetComponentLocation().Z - SpawnBox->GetScaledBoxExtent().Z;
	FHitResult Hit;
	if (World->SweepSingleByChannel(Hit, SweepStart, SweepEnd, BoxRotation, GroundTraceChannel, Shape, Params))
	{
		if (Hit.bStartPenetrating) { return false; }
		SurfaceZ = Hit.Location.Z - 1.f;
	}

	// 먼저 내려오기 시작한 낙하물이 도착할 자리 위에는 쌓임
	const FBox2D MyFootprint(FVector2D(Origin.X - Extent.X * 0.9f, Origin.Y - Extent.Y * 0.9f), FVector2D(Origin.X + Extent.X * 0.9f, Origin.Y + Extent.Y * 0.9f));
	for (const TPair<int32, FServerFall>& Pair : ServerFalls)
	{
		const FServerFall& Other = Pair.Value;
		if (Other.bSettle && Other.DestFootprint.Intersect(MyFootprint))
		{
			SurfaceZ = FMath::Max(SurfaceZ, Other.DestTopZ);
		}
	}

	const float Gap = BottomZ - SurfaceZ;
	if (Gap <= SettleMinGap) { return false; }

	// 같은 낙하 처리에 넣음: 충돌 끄고 내려오며, 적중하면 넉백과 꼬리 손실도 같음
	const float PivotToBottom = Actor->GetActorLocation().Z - BottomZ;

	FCTFallEntry Entry;
	Entry.Actor = Actor;
	Entry.Id = NextEntryId++;
	Entry.StartZ = Actor->GetActorLocation().Z;
	Entry.ShadowRadius = FMath::Max(FMath::Max(Extent.X, Extent.Y) * ShadowRadiusMultiplier, 20.f);
	Entry.MaterialIndex = INDEX_NONE; // 머티리얼은 그대로
	Entry.GroundLocation = FVector(Actor->GetActorLocation().X, Actor->GetActorLocation().Y, SurfaceZ);

	FServerFall Fall;
	Fall.bSettle = true;
	Fall.Speed = SettleFallSpeed;
	Fall.StartZ = Actor->GetActorLocation().Z;
	Fall.EndZ = SurfaceZ + PivotToBottom;
	Fall.XY = FVector2D(Actor->GetActorLocation().X, Actor->GetActorLocation().Y);
	Fall.Duration = FMath::Max((Fall.StartZ - Fall.EndZ) / FMath::Max(SettleFallSpeed, 1.f), 0.05f);
	Fall.StartRotation = Actor->GetActorQuat();
	Fall.EndRotation = Actor->GetActorQuat();
	Fall.DestFootprint = FBox2D(FVector2D(Origin.X - Extent.X, Origin.Y - Extent.Y), FVector2D(Origin.X + Extent.X, Origin.Y + Extent.Y));
	Fall.DestTopZ = SurfaceZ + Extent.Z * 2.f;
	ServerFalls.Add(Entry.Id, Fall);
	Entries.Add(Entry);

	if (bControlCollision)
	{
		ApplyCollision(Actor, CollisionWhileFalling);
	}

	// 이 낙하물이 떠난 자리 위의 낙하물도 확인
	FSupportRegion Region;
	Region.Footprint = Fall.DestFootprint;
	Region.BottomZ = BottomZ;
	PendingRegions.Add(Region);

	UE_LOG(LogTemp, Log, TEXT("[CT] 낙하물 재낙하: %s  %.0fcm 아래로 (받침 없음)"), *Actor->GetName(), Gap);
	return true;
}

void ACTFallingSpawnVolume::TickVisuals()
{
	if (GetNetMode() == NM_DedicatedServer) { return; }

	for (const FCTFallEntry& Entry : Entries)
	{
		FLocalFall* Fall = LocalFalls.Find(Entry.Id);
		if (!Fall) { continue; }

		const AActor* Falling = Entry.Actor;

		// 클라는 액터가 늦게 오면 그때 적용
		if (!Fall->bMaterialApplied && Falling)
		{
			ApplyMaterial(Entry.Actor, Entry.MaterialIndex);
			if (bControlCollision && !HasAuthority())
			{
				ApplyCollision(Entry.Actor, CollisionWhileFalling);
			}
			Fall->bMaterialApplied = true;
		}
		Fall->Actor = Entry.Actor;

		if (!Fall->ShadowComponent || !Fall->ShadowMaterialInstance) { continue; }

		// 가까울수록 진하고 큼
		float Progress = 0.f;
		const float Drop = Entry.StartZ - Entry.GroundLocation.Z;
		if (Falling && Drop > KINDA_SMALL_NUMBER)
		{
			Progress = FMath::Clamp((Entry.StartZ - Falling->GetActorLocation().Z) / Drop, 0.f, 1.f);
		}

		const float Opacity = FMath::Lerp(ShadowMinOpacity, ShadowMaxOpacity, Progress);
		Fall->ShadowMaterialInstance->SetScalarParameterValue(ShadowOpacityParameterName, Opacity);

		// 디칼 크기는 반 길이(투영 깊이, 가로, 세로)
		const float Radius = Entry.ShadowRadius * FMath::Lerp(ShadowStartScale, 1.f, Progress);
		Fall->ShadowComponent->DecalSize = FVector(ShadowProjectionHalfDepth, Radius, Radius);
		Fall->ShadowComponent->MarkRenderStateDirty();
	}
}

// 그림자 / 머티리얼 (모든 화면)

void ACTFallingSpawnVolume::OnRep_Entries()
{
	SyncLocalVisuals();
}

void ACTFallingSpawnVolume::DestroyLocalFall(FLocalFall& Fall)
{
	if (Fall.ShadowComponent)
	{
		Fall.ShadowComponent->DestroyComponent();
		Fall.ShadowComponent = nullptr;
	}
	Fall.ShadowMaterialInstance = nullptr;
}

void ACTFallingSpawnVolume::SyncLocalVisuals()
{
	if (GetNetMode() == NM_DedicatedServer) { return; }

	// 끝난 그림자 제거
	for (auto It = LocalFalls.CreateIterator(); It; ++It)
	{
		const int32 Id = It.Key();
		if (!Entries.ContainsByPredicate([Id](const FCTFallEntry& Entry) { return Entry.Id == Id; }))
		{
			// 목록에서 빠짐 = 착지, 충돌 적용
			if (bControlCollision && !HasAuthority())
			{
				if (AActor* LandedActor = It.Value().Actor.Get())
				{
					ApplyCollision(LandedActor, CollisionAfterLanding);
				}
			}
			DestroyLocalFall(It.Value());
			It.RemoveCurrent();
		}
	}

	// 새 그림자 생성
	for (const FCTFallEntry& Entry : Entries)
	{
		if (LocalFalls.Contains(Entry.Id)) { continue; }

		FLocalFall NewFall;

		if (bShowShadow && ShadowMaterial)
		{
			// 디칼: 지형 모양을 따라 바닥에 그려짐. X축이 투영 방향이라 아래(-Z)를 보게 눕힘
			UDecalComponent* Shadow = NewObject<UDecalComponent>(this);
			Shadow->SetMobility(EComponentMobility::Movable);
			Shadow->RegisterComponent();

			const FVector Location(Entry.GroundLocation.X, Entry.GroundLocation.Y, Entry.GroundLocation.Z + ShadowProjectionLift);
			Shadow->SetWorldLocationAndRotation(Location, FRotator(-90.f, 0.f, 0.f));
			Shadow->DecalSize = FVector(ShadowProjectionHalfDepth, Entry.ShadowRadius * ShadowStartScale, Entry.ShadowRadius * ShadowStartScale);

			NewFall.ShadowComponent = Shadow;
			NewFall.ShadowMaterialInstance = UMaterialInstanceDynamic::Create(ShadowMaterial, this);
			if (NewFall.ShadowMaterialInstance)
			{
				Shadow->SetDecalMaterial(NewFall.ShadowMaterialInstance);
				NewFall.ShadowMaterialInstance->SetVectorParameterValue(ShadowColorParameterName, ShadowColor);
				NewFall.ShadowMaterialInstance->SetScalarParameterValue(ShadowOpacityParameterName, ShadowMinOpacity);
			}
		}

		LocalFalls.Add(Entry.Id, NewFall);
	}
}

void ACTFallingSpawnVolume::ApplyMaterial(AActor* Actor, int32 MaterialIndex) const
{
	if (!Actor || !Materials.IsValidIndex(MaterialIndex)) { return; }

	UMaterialInterface* Material = Materials[MaterialIndex];
	if (!Material) { return; }

	TInlineComponentArray<UMeshComponent*> MeshComponents;
	Actor->GetComponents(MeshComponents);

	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (MaterialSlotIndex >= 0)
		{
			if (MaterialSlotIndex < Mesh->GetNumMaterials())
			{
				Mesh->SetMaterial(MaterialSlotIndex, Material);
			}
		}
		else
		{
			for (int32 SlotIndex = 0; SlotIndex < Mesh->GetNumMaterials(); ++SlotIndex)
			{
				Mesh->SetMaterial(SlotIndex, Material);
			}
		}
	}
}
void ACTFallingSpawnVolume::ApplyCollision(AActor* Actor, const FCollisionProfileName& Profile) const
{
	if (!Actor || Profile.Name.IsNone()) { return; }

	TInlineComponentArray<UPrimitiveComponent*> Primitives;
	Actor->GetComponents(Primitives);

	for (UPrimitiveComponent* Primitive : Primitives)
	{
		Primitive->SetCollisionProfileName(Profile.Name);

		if (bIgnoreCameraChannel)
		{
			Primitive->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		}
	}
}
void ACTFallingSpawnVolume::DropTailFromHit(ACTCharacter* Character, const FVector& KnockbackDirection)
{
	const int32 Lost = ACTTailPickup::DropTailsFromCharacter(Character, HitTailLoss, KnockbackDirection, DroppedTailClass,
		DroppedTailLaunchSpeed, DroppedTailUpSpeed, DroppedTailPickupDelay, DroppedTailLifeSeconds, this);

	if (Lost > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[CT] 낙하 충돌: %s 꼬리 -%d"), *Character->GetName(), Lost);
	}
}

void ACTFallingSpawnVolume::ApplyHitKnockback(const FCTFallEntry& Entry, AActor* Falling, const FVector& BoundsOrigin, const FVector& BoundsExtent)
{
	UWorld* World = GetWorld();
	if (!World || !Falling) { return; }

	// 경기 중에만 맞음 (카운트다운, 결과 화면에서는 떨어지는 모습만 보임)
	const ACTGameState* CTGameState = World->GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }

	// 영역 안의 폰 검색
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTFallingHit), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Falling);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps, BoundsOrigin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeBox(BoundsExtent * HitRangeScale),
		Params);

	const double Now = World->GetTimeSeconds();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACTCharacter* Character = Cast<ACTCharacter>(Overlap.GetActor());
		if (!Character || Character->IsGhost()) { continue; } // 유령 제외

		// 직전 적중자 제외
		const double* LastHit = LastHitTimes.Find(Character);
		if (LastHit && Now - *LastHit < HitCooldownSeconds) { continue; }
		LastHitTimes.Add(Character, Now);

		// 중심에서 캐릭터 쪽으로 (바로 위면 무작위)
		FVector Direction = Character->GetActorLocation() - BoundsOrigin;
		Direction.Z = 0.f;
		if (Direction.IsNearlyZero())
		{
			Direction = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f);
		}
		Direction = Direction.GetSafeNormal();

		// 거리 맞춰 속도 계산
		// 가로 속도 = 거리 ÷ 공중 시간 (2 × 상승속도 ÷ 중력)
		const float Gravity = FMath::Max(FMath::Abs(Character->GetCharacterMovement()->GetGravityZ()), 1.f);
		const float UpSpeed = FMath::Max(HitKnockbackUpSpeed, 1.f);
		const float AirTime = FMath::Max(2.f * UpSpeed / Gravity, 0.05f);
		const float HorizontalSpeed = FMath::Min(HitKnockbackDistance / AirTime, HitKnockbackMaxSpeed);

		const FVector LaunchVelocity = Direction * HorizontalSpeed + FVector(0.f, 0.f, UpSpeed);
		Character->LaunchCharacter(LaunchVelocity, /*bXYOverride=*/true, /*bZOverride=*/true);

		if (bPlayPushedReaction)
		{
			Character->PlayKnockedDown(Direction);
		}

		UE_LOG(LogTemp, Log, TEXT("[CT] 낙하 충돌: %s 가 %s 에 맞아 튕겨남 (목표 거리 %.0f, 가로 속도 %.0f, 공중 %.2f초)"),
			*Character->GetName(), *Falling->GetName(), HitKnockbackDistance, HorizontalSpeed, AirTime);

		if (bHitDropsTail)
		{
			DropTailFromHit(Character, Direction);
		}

		OnCharacterHit.Broadcast(Character, Falling);
	}

	// 사라진 캐릭터 기록 정리
	for (auto It = LastHitTimes.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); }
	}
}
