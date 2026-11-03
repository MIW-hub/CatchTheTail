// Copyright Moon Party Games. All Rights Reserved.
#include "CTExplosiveBarrel.h"
#include "CTTailPickup.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ACTExplosiveBarrel::ACTExplosiveBarrel()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetNetUpdateFrequency(10.f);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	TouchTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("TouchTrigger"));
	TouchTrigger->SetupAttachment(Mesh);
	TouchTrigger->SetSphereRadius(90.f);
	TouchTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TouchTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	TouchTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	TouchTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TouchTrigger->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BarrelFinder(TEXT("/Game/ThirdParty/Low_Poly_Town/Models/Props/SM_Barrel_2A.SM_Barrel_2A"));
	if (BarrelFinder.Succeeded()) { Mesh->SetStaticMesh(BarrelFinder.Object); }

	DroppedTailClass = ACTTailPickup::StaticClass();
}

void ACTExplosiveBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACTExplosiveBarrel, State);
	DOREPLIFETIME(ACTExplosiveBarrel, ExplodeServerTime);
}

void ACTExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();

	BaseScale = Mesh->GetComponentScale();
	TouchTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACTExplosiveBarrel::HandleTouchBeginOverlap);
}

bool ACTExplosiveBarrel::IsPlaying() const
{
	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	return CTGameState && CTGameState->GetPhase() == ECTGamePhase::Playing;
}

double ACTExplosiveBarrel::GetServerNow() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
}

void ACTExplosiveBarrel::HandleTouchBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || State != ECTBarrelState::Idle || !IsPlaying()) { return; }

	const ACTCharacter* Character = Cast<ACTCharacter>(OtherActor);
	if (!Character || Character->IsGhost()) { return; }

	StartFuse();
}

void ACTExplosiveBarrel::StartFuse()
{
	const ECTBarrelState Old = State;
	State = ECTBarrelState::Burning;
	ExplodeServerTime = GetServerNow() + FuseSeconds;
	GetWorldTimerManager().SetTimer(FuseTimer, this, &ACTExplosiveBarrel::Explode, FuseSeconds, false);

	UE_LOG(LogTemp, Log, TEXT("[CT] 드럼통 도화선 시작: %s (%.1f초 뒤 폭발)"), *GetName(), FuseSeconds);
	ApplyStateVisuals(Old); // 서버(리슨 호스트) 화면
}

void ACTExplosiveBarrel::Explode()
{
	UWorld* World = GetWorld();
	if (!World || State != ECTBarrelState::Burning) { return; }

	const FVector Center = GetActorLocation() + FVector(0.f, 0.f, 50.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTBarrelExplosion), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ExplosionRadius), Params);

	TSet<ACTCharacter*> Done;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACTCharacter* Character = Cast<ACTCharacter>(Overlap.GetActor());
		if (!Character || Character->IsGhost() || Done.Contains(Character)) { continue; }
		Done.Add(Character);

		FVector Away = Character->GetActorLocation() - GetActorLocation();
		Away.Z = 0.f;
		if (Away.IsNearlyZero()) { Away = FMath::VRand(); Away.Z = 0.f; }
		const FVector Push = Away.GetSafeNormal();

		// 가운데에 가까울수록 세게 날아감
		const float Dist = FVector::Dist2D(Character->GetActorLocation(), GetActorLocation());
		const float Ratio = FMath::Lerp(1.f, MinSpeedRatio, FMath::Clamp(Dist / ExplosionRadius, 0.f, 1.f));
		Character->LaunchCharacter(Push * KnockbackSpeed * Ratio + FVector(0.f, 0.f, KnockbackUpSpeed), /*bXYOverride=*/true, /*bZOverride=*/true);

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

		UE_LOG(LogTemp, Log, TEXT("[CT] 드럼통 폭발: %s 가 %s 폭발에 휘말림 (꼬리 -%d)"), *Character->GetName(), *GetName(), Lost);
		BP_OnHitCharacter(Character);
	}

	const ECTBarrelState Old = State;
	State = ECTBarrelState::Exploded;
	ApplyStateVisuals(Old); // 서버(리슨 호스트) 화면

	if (RespawnSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ACTExplosiveBarrel::Respawn, RespawnSeconds, false);
	}
}

void ACTExplosiveBarrel::Respawn()
{
	const ECTBarrelState Old = State;
	State = ECTBarrelState::Idle;
	ApplyStateVisuals(Old);

	// 다시 나타난 자리에 이미 서 있는 사람이 있으면 바로 도화선이 탐
	if (IsPlaying())
	{
		TArray<AActor*> Overlapping;
		TouchTrigger->GetOverlappingActors(Overlapping, ACTCharacter::StaticClass());
		for (AActor* Actor : Overlapping)
		{
			const ACTCharacter* Character = Cast<ACTCharacter>(Actor);
			if (Character && !Character->IsGhost())
			{
				StartFuse();
				break;
			}
		}
	}
}

void ACTExplosiveBarrel::OnRep_State()
{
	ApplyStateVisuals(AppliedState);
}

void ACTExplosiveBarrel::ApplyStateVisuals(ECTBarrelState OldState)
{
	if (AppliedState == State && OldState == State) { return; }
	AppliedState = State;

	const bool bVisible = State != ECTBarrelState::Exploded;
	SetActorHiddenInGame(!bVisible);
	Mesh->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	TouchTrigger->SetCollisionEnabled(bVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	Mesh->SetWorldScale3D(BaseScale);
	Mesh->SetOverlayMaterial(State == ECTBarrelState::Burning ? BurningOverlayMaterial.Get() : nullptr);

	if (State == ECTBarrelState::Burning)
	{
		if (FuseSound) { UGameplayStatics::PlaySoundAtLocation(this, FuseSound, GetActorLocation()); }
		BP_OnFuseStarted();
	}
	else if (State == ECTBarrelState::Exploded && OldState != ECTBarrelState::Exploded)
	{
		if (ExplosionSound) { UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation()); }
		BP_OnExploded();
	}
}

void ACTExplosiveBarrel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State != AppliedState) { ApplyStateVisuals(AppliedState); }
	if (State != ECTBarrelState::Burning) { return; }

	// 폭발이 가까울수록 빠르게 깜빡이며 커졌다 작아짐 (서버 시간 기준이라 모든 화면이 비슷하게 보임)
	const double Remaining = FMath::Max(ExplodeServerTime - GetServerNow(), 0.0);
	const float Alpha = 1.f - FMath::Clamp(static_cast<float>(Remaining) / FuseSeconds, 0.f, 1.f);
	const float Rate = FMath::Lerp(BlinkStartRate, BlinkEndRate, Alpha);
	const float Wave = 0.5f + 0.5f * FMath::Sin(static_cast<float>(ExplodeServerTime - Remaining) * Rate * UE_TWO_PI);
	Mesh->SetWorldScale3D(BaseScale * (1.f + BlinkScaleAmount * Wave * (0.4f + 0.6f * Alpha)));
}
