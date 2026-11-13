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

ACTEnergyPickup* ACTEnergyPickup::Spawn(UWorld* World, const FVector& Location, int32 InAmount)
{
	if (!World) { return nullptr; }

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACTEnergyPickup* Pickup = World->SpawnActor<ACTEnergyPickup>(ACTEnergyPickup::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Pickup) { Pickup->Amount = FMath::Max(InAmount, 1); }
	return Pickup;
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

	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &ACTEnergyPickup::HandleBeginOverlap);

		// 놓이는 순간 이미 겹쳐 있는 캐릭터가 있으면 바로 먹게 함
		TArray<AActor*> Overlapping;
		Trigger->GetOverlappingActors(Overlapping, ACTCharacter::StaticClass());
		for (AActor* Actor : Overlapping)
		{
			if (ACTCharacter* Character = Cast<ACTCharacter>(Actor)) { HandleBeginOverlap(Trigger, Character, nullptr, 0, false, FHitResult()); }
		}
	}
}

void ACTEnergyPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 보기 좋게 천천히 돎 (화면이 있는 쪽만)
	if (GetNetMode() != NM_DedicatedServer)
	{
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
