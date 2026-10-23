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

	// 모든 클라이언트가 항상 받아야 합니다. (멀리 있어도 그림자와 머티리얼이 맞게)
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10.f);

	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	SetRootComponent(SpawnBox);
	SpawnBox->SetBoxExtent(FVector(1500.f, 1500.f, 1000.f));
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBox->SetHiddenInGame(true);
	SpawnBox->ShapeColor = FColor(120, 190, 255);

	// 기본 그림자 메시와 머티리얼 (블루프린트에서 바꿀 수 있음)
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

// ============================================================
// 스폰 (서버)
// ============================================================

float ACTFallingSpawnVolume::FindGroundZ(const FVector& From) const
{
	const FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
	const float BoxBottomZ = SpawnBox->GetComponentLocation().Z - BoxExtent.Z;

	// 박스 맨 위에서 아래로 충분히 멀리 검사합니다. (박스 높이의 몇 배 + 여유)
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

	// 액터 클래스와 스태틱 메시를 한 묶음으로 보고 무작위로 하나를 고릅니다. (비어 있는 칸은 건너뜀)
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

	// 시작부터 복제되도록 BeginPlay 전에 설정합니다. (어떤 액터든 모든 화면에 보이게)
	AActor* Spawned = World->SpawnActorDeferred<AActor>(ChosenClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Spawned) { return nullptr; }

	Spawned->SetReplicates(true);
	Spawned->SetReplicateMovement(true);
	// 스태틱 메시를 골랐다면 낙하용 액터에 메시와 크기를 넣습니다. (FinishSpawning 전이라 모든 화면에 함께 전달됨)
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

	// 떨어지는 동안의 충돌 설정 (클라이언트는 액터가 도착하면 TickVisuals에서 적용)
	if (bControlCollision)
	{
		ApplyCollision(Spawned, CollisionWhileFalling);
	}

	// 크기와 바닥 위치로 그림자를 정합니다.
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

	// ----- 선형보간(Lerp) 낙하 정보 -----
	// 액터의 기준점에서 바닥 면까지의 거리를 구해서, 바닥에 닿았을 때 기준점이 있어야 할 높이(EndZ)를 정합니다.
	const float PivotToBottom = Spawned->GetActorLocation().Z - (BoundsOrigin.Z - BoundsExtent.Z);

	FServerFall Fall;
	Fall.Speed = FMath::FRandRange(FMath::Min(FallSpeedMin, FallSpeedMax), FMath::Max(FallSpeedMin, FallSpeedMax));
	Fall.StartZ = SpawnLocation.Z;
	Fall.EndZ = GroundZ + PivotToBottom;
	Fall.XY = FVector2D(SpawnLocation.X, SpawnLocation.Y);
	Fall.Duration = FMath::Max((Fall.StartZ - Fall.EndZ) / FMath::Max(Fall.Speed, 1.f), 0.05f);

	// 기울어진 채 시작해서 바닥에 닿을 때 반듯해지는 회전 (물리처럼 보이게 하는 연출)
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
	SyncLocalVisuals(); // 방장(서버) 화면에도 그림자를 만듭니다.

	return Spawned;
}

// ============================================================
// 매 프레임
// ============================================================

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

	// ----- 떨어뜨리기 -----
	for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
	{
		const FCTFallEntry Entry = Entries[Index];
		AActor* Falling = Entry.Actor;

		// 도중에 사라진 액터는 목록에서 뺍니다.
		FServerFall* Fall = ServerFalls.Find(Entry.Id);
		if (!IsValid(Falling) || !Fall)
		{
			ServerFalls.Remove(Entry.Id);
			Entries.RemoveAt(Index);
			bChanged = true;
			continue;
		}

		// 시작 높이에서 바닥까지 시간에 따라 선형보간(Lerp)합니다. 기울기도 같은 비율로 반듯해집니다.
		Fall->Elapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(Fall->Elapsed / Fall->Duration, 0.f, 1.f);
		const float NewZ = FMath::Lerp(Fall->StartZ, Fall->EndZ, Alpha);
		const FQuat NewRotation = FQuat::Slerp(Fall->StartRotation, Fall->EndRotation, Alpha);
		Falling->SetActorLocationAndRotation(FVector(Fall->XY.X, Fall->XY.Y, NewZ), NewRotation, /*bSweep=*/false);

		// 지금 액터가 차지한 영역
		FVector BoundsOrigin, BoundsExtent;
		Falling->GetActorBounds(/*bOnlyCollidingComponents=*/false, BoundsOrigin, BoundsExtent);

		// 떨어지는 액터에 맞은 캐릭터는 튕겨냅니다.
		if (bKnockbackOnHit)
		{
			ApplyHitKnockback(Entry, Falling, BoundsOrigin, BoundsExtent);
		}

		if (Alpha >= 1.f)
		{
			// 착지한 뒤의 충돌 설정 (클라이언트는 목록에서 사라질 때 같은 설정을 적용)
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

	// ----- 새로 떨어뜨리기 -----
	if (!bSpawning) { return; }

	if (bOnlySpawnWhilePlaying)
	{
		const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
		if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }
	}

	TimeToNextSpawn -= DeltaSeconds;
	if (TimeToNextSpawn <= 0.f)
	{
		// 가득 차 있으면 SpawnOne이 실패하고, 다음 프레임에 다시 시도합니다.
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

		// 클라이언트에서는 액터가 늦게 도착할 수 있어서, 도착하면 머티리얼과 충돌 설정을 적용합니다.
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

		// 가까워질수록 그림자가 진해지고 커집니다.
		float Progress = 0.f;
		const float Drop = Entry.StartZ - Entry.GroundLocation.Z;
		if (Falling && Drop > KINDA_SMALL_NUMBER)
		{
			Progress = FMath::Clamp((Entry.StartZ - Falling->GetActorLocation().Z) / Drop, 0.f, 1.f);
		}

		const float Opacity = FMath::Lerp(ShadowMinOpacity, ShadowMaxOpacity, Progress);
		Fall->ShadowMaterialInstance->SetScalarParameterValue(ShadowOpacityParameterName, Opacity);

		// Plane 메시는 한 변이 100cm라서 (반지름 × 2) ÷ 100 이 크기 배율입니다.
		const float Scale = Entry.ShadowRadius * 2.f / 100.f * FMath::Lerp(ShadowStartScale, 1.f, Progress);
		Fall->ShadowComponent->SetWorldScale3D(FVector(Scale, Scale, 1.f));
	}
}

// ============================================================
// 그림자 / 머티리얼 (모든 화면)
// ============================================================

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

	// 끝난 것의 그림자를 지웁니다.
	for (auto It = LocalFalls.CreateIterator(); It; ++It)
	{
		const int32 Id = It.Key();
		if (!Entries.ContainsByPredicate([Id](const FCTFallEntry& Entry) { return Entry.Id == Id; }))
		{
			// 클라이언트: 목록에서 빠졌다는 것은 바닥에 닿았다는 뜻이라, 착지 충돌을 적용합니다.
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

	// 새로 떨어지는 것의 그림자를 만듭니다.
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

			// 월드 좌표에 그대로 놓습니다. (볼륨이 움직여도 그림자는 바닥에 고정)
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

	// 액터가 차지한 상자 안에 있는 폰(캐릭터)을 찾습니다.
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
		if (!Character || Character->IsGhost()) { continue; } // 유령은 맞지 않습니다.

		// 바로 전에 맞은 캐릭터는 건너뜁니다.
		const double* LastHit = LastHitTimes.Find(Character);
		if (LastHit && Now - *LastHit < HitCooldownSeconds) { continue; }
		LastHitTimes.Add(Character, Now);

		// 박스 중심에서 캐릭터 쪽 가로 방향으로 밀어냅니다. (정확히 위에 있으면 무작위 방향)
		FVector Direction = Character->GetActorLocation() - BoundsOrigin;
		Direction.Z = 0.f;
		if (Direction.IsNearlyZero())
		{
			Direction = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f);
		}
		Direction = Direction.GetSafeNormal();

		// 정한 거리만큼 날아가도록 속도를 계산합니다.
		// 위로 UpSpeed로 떠서 다시 내려올 때까지의 시간(공중 시간) = 2 × UpSpeed ÷ 중력, 가로 속도 = 거리 ÷ 공중 시간
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

	// 오래된 기록 정리 (사라진 캐릭터)
	for (auto It = LastHitTimes.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); }
	}
}
