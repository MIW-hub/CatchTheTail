// Copyright Moon Party Games. All Rights Reserved.
#include "CTFallingSpawnVolume.h"
#include "CTFallingProp.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "Components/BoxComponent.h"
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

	// 기본 그림자 메시, 머티리얼
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShadowFinder(TEXT("/Game/Material/M_FallingShadow.M_FallingShadow"));
	if (PlaneFinder.Succeeded()) { ShadowMesh = PlaneFinder.Object; }
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
			// 착지 충돌 (클라는 목록 제거 시 적용)
			if (bControlCollision)
			{
				ApplyCollision(Falling, CollisionAfterLanding);
			}

			if (LandedLifeSeconds > 0.f)
			{
				Falling->SetLifeSpan(LandedLifeSeconds);
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

		// Plane은 100cm, 배율 = 지름 ÷ 100
		const float Scale = Entry.ShadowRadius * 2.f / 100.f * FMath::Lerp(ShadowStartScale, 1.f, Progress);
		Fall->ShadowComponent->SetWorldScale3D(FVector(Scale, Scale, 1.f));
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

		if (bShowShadow && ShadowMesh && ShadowMaterial)
		{
			UStaticMeshComponent* Shadow = NewObject<UStaticMeshComponent>(this);
			Shadow->SetStaticMesh(ShadowMesh);
			Shadow->SetMobility(EComponentMobility::Movable);
			Shadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Shadow->SetGenerateOverlapEvents(false);
			Shadow->SetCastShadow(false);
			Shadow->RegisterComponent();

			// 월드 좌표 고정
			const FVector Location(Entry.GroundLocation.X, Entry.GroundLocation.Y, Entry.GroundLocation.Z + ShadowHeightOffset);
			Shadow->SetWorldLocationAndRotation(Location, FRotator::ZeroRotator);
			Shadow->SetWorldScale3D(FVector(Entry.ShadowRadius * 2.f / 100.f * ShadowStartScale, Entry.ShadowRadius * 2.f / 100.f * ShadowStartScale, 1.f));

			NewFall.ShadowComponent = Shadow;
			NewFall.ShadowMaterialInstance = Shadow->CreateAndSetMaterialInstanceDynamicFromMaterial(0, ShadowMaterial);
			if (NewFall.ShadowMaterialInstance)
			{
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
void ACTFallingSpawnVolume::ApplyHitKnockback(const FCTFallEntry& Entry, AActor* Falling, const FVector& BoundsOrigin, const FVector& BoundsExtent)
{
	UWorld* World = GetWorld();
	if (!World || !Falling) { return; }

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
			Character->MulticastPlayAction(ECTCharacterAction::Pushed, true);
		}

		UE_LOG(LogTemp, Log, TEXT("[CT] 낙하 충돌: %s 가 %s 에 맞아 튕겨남 (목표 거리 %.0f, 가로 속도 %.0f, 공중 %.2f초)"),
			*Character->GetName(), *Falling->GetName(), HitKnockbackDistance, HorizontalSpeed, AirTime);

		OnCharacterHit.Broadcast(Character, Falling);
	}

	// 사라진 캐릭터 기록 정리
	for (auto It = LastHitTimes.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); }
	}
}
