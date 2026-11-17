// Copyright Moon Party Games. All Rights Reserved.
#include "CTEnergyPickup.h"
#include "../Core/CTGameMode.h"
#include "../Core/CTGameRule.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACTEnergyPickup::ACTEnergyPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f); // 가만히 있는 물체라 갱신이 거의 필요 없음

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(90.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Trigger);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);
	Visual->SetCanEverAffectNavigation(false);

	// 엔진 기본 구를 에너지 알갱이로 씀
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded()) { Visual->SetStaticMesh(SphereFinder.Object); }
}

ACTEnergyPickup* ACTEnergyPickup::Spawn(UWorld* World, const FVector& Location, int32 InAmount, float InFallDelay)
{
	if (!World) { return nullptr; }

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// 복제되기 전에 값을 넣어야 해서 BeginPlay 이전에 설정
	ACTEnergyPickup* Pickup = World->SpawnActorDeferred<ACTEnergyPickup>(ACTEnergyPickup::StaticClass(), FTransform(Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup) { return nullptr; }
	Pickup->Amount = FMath::Max(InAmount, 1);
	Pickup->FallDelay = InFallDelay;
	Pickup->FallSeconds = Pickup->DropSeconds;
	Pickup->LaunchOffset = FVector(0.f, 0.f, Pickup->DropHeight);
	Pickup->FinishSpawning(FTransform(Location));
	return Pickup;
}

ACTEnergyPickup* ACTEnergyPickup::SpawnBurst(UWorld* World, const FVector& From, const FVector& Location, int32 InAmount)
{
	if (!World) { return nullptr; }

	ACTEnergyPickup* Pickup = World->SpawnActorDeferred<ACTEnergyPickup>(ACTEnergyPickup::StaticClass(), FTransform(Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup) { return nullptr; }
	Pickup->Amount = FMath::Max(InAmount, 1);
	Pickup->FallDelay = 0.f;
	Pickup->FallSeconds = FMath::FRandRange(0.55f, 0.8f);
	Pickup->LaunchOffset = From - Location;
	Pickup->ArcHeight = FMath::FRandRange(120.f, 260.f);
	Pickup->FinishSpawning(FTransform(Location));
	return Pickup;
}

void ACTEnergyPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ACTEnergyPickup, FallDelay, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ACTEnergyPickup, LaunchOffset, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ACTEnergyPickup, ArcHeight, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ACTEnergyPickup, FallSeconds, COND_InitialOnly);
}

void ACTEnergyPickup::BeginPlay()
{
	Super::BeginPlay();

	Visual->SetRelativeScale3D(FVector(VisualScale));

	// 색만 바꿔 쓰는 머티리얼 인스턴스 (없으면 메시 기본 머티리얼)
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/M_CT_EnergyOrb.M_CT_EnergyOrb")))
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), EnergyColor);
		Visual->SetMaterial(0, Material);
	}

	SpawnTime = GetWorld()->GetTimeSeconds();
	bLanded = FallDelay < 0.f;

	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &ACTEnergyPickup::HandleBeginOverlap);

		if (bLanded)
		{
			EnableTrigger();
		}
		else
		{
			// 내려앉는 순간까지 먹을 수 없게 닫아 둠
			Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, this, &ACTEnergyPickup::EnableTrigger, FallDelay + FallSeconds, false);
		}
	}

	if (!bLanded) { Visual->SetVisibility(false); }
}

void ACTEnergyPickup::EnableTrigger()
{
	bLanded = true;
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 내려앉는 순간 이미 겹쳐 있는 캐릭터가 있으면 바로 먹게 함
	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping, ACTCharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (ACTCharacter* Character = Cast<ACTCharacter>(Actor)) { HandleBeginOverlap(Trigger, Character, nullptr, 0, false, FHitResult()); }
	}
}

void ACTEnergyPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 보기 좋게 천천히 돎 (화면이 있는 쪽만)
	if (GetNetMode() != NM_DedicatedServer)
	{
		// 하늘에서 떨어지는 동안: 기다리는 중에는 숨기고, 중력처럼 점점 빨라지며 내려옴 (보이는 모습만, 판정 위치는 그대로)
		if (FallDelay >= 0.f)
		{
			const float Elapsed = GetWorld()->GetTimeSeconds() - SpawnTime - FallDelay;
			const float Alpha = FMath::Clamp(Elapsed / FMath::Max(FallSeconds, 0.1f), 0.f, 1.f);
			Visual->SetVisibility(Elapsed >= 0.f);
			FVector Offset;
			if (ArcHeight > 0.f)
			{
				// 터지듯 튀어 나감: 옆으로는 똑바로, 위로는 솟았다가 내려옴
				Offset = FVector(LaunchOffset.X * (1.f - Alpha), LaunchOffset.Y * (1.f - Alpha), LaunchOffset.Z * (1.f - Alpha) + ArcHeight * 4.f * Alpha * (1.f - Alpha));
			}
			else
			{
				Offset = FVector(0.f, 0.f, LaunchOffset.Z * (1.f - Alpha * Alpha));
			}
			Visual->SetRelativeLocation(Offset);
		}
		Visual->AddLocalRotation(FRotator(0.f, SpinDegreesPerSecond * DeltaSeconds, 0.f));
	}
}

void ACTEnergyPickup::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bConsumed) { return; }

	ACTCharacter* Character = Cast<ACTCharacter>(OtherActor);
	UWorld* World = GetWorld();
	ACTGameMode* GameMode = World ? World->GetAuthGameMode<ACTGameMode>() : nullptr;
	const ACTGameState* GameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!Character || !GameMode || !GameState || GameState->GetPhase() != ECTGamePhase::Playing) { return; }

	// 먹을 수 있는지는 규칙이 정함 (규칙이 처리하면 사라짐)
	if (GameMode->HandleEnergyPickup(Character, Amount))
	{
		bConsumed = true;
		Destroy();
	}
}
