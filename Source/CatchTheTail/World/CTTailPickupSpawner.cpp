// Copyright Moon Party Games. All Rights Reserved.
#include "CTTailPickupSpawner.h"
#include "CTTailPickup.h"
#include "../Core/CTGameState.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

ACTTailPickupSpawner::ACTTailPickupSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	// 서버에서만 스폰, 만들어진 아이템이 복제됨
	bReplicates = false;

	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	SetRootComponent(SpawnBox);
	SpawnBox->SetBoxExtent(FVector(1500.f, 1500.f, 500.f));
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBox->SetHiddenInGame(true);
	SpawnBox->ShapeColor = FColor(255, 200, 60);

	PickupClass = ACTTailPickup::StaticClass();
}

void ACTTailPickupSpawner::BeginPlay()
{
	Super::BeginPlay();
	TimeToNextSpawn = PickInterval();
}

float ACTTailPickupSpawner::PickInterval() const
{
	const float MinInterval = FMath::Max(SpawnIntervalMin, 0.5f);
	const float MaxInterval = FMath::Max(SpawnIntervalMax, MinInterval);
	return FMath::FRandRange(MinInterval, MaxInterval);
}

void ACTTailPickupSpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) { return; }

	if (bOnlySpawnWhilePlaying)
	{
		const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
		if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }
	}

	TimeToNextSpawn -= DeltaSeconds;
	if (TimeToNextSpawn > 0.f) { return; }

	// 먹히거나 사라진 아이템 정리 후, 가득 차 있으면 다음 기회에
	Spawned.RemoveAll([](const TWeakObjectPtr<ACTTailPickup>& Item) { return !Item.IsValid(); });
	if (Spawned.Num() >= MaxPickups)
	{
		TimeToNextSpawn = 1.f;
		return;
	}

	TimeToNextSpawn = TrySpawnOne() ? PickInterval() : 1.f;
}

bool ACTTailPickupSpawner::TrySpawnOne()
{
	UWorld* World = GetWorld();
	if (!World || !PickupClass) { return false; }

	// 영역 안의 임의의 XY, 맨 위에서 아래로 바닥을 찾음
	const FVector Extent = SpawnBox->GetUnscaledBoxExtent();
	const FVector LocalPoint(FMath::FRandRange(-Extent.X, Extent.X), FMath::FRandRange(-Extent.Y, Extent.Y), Extent.Z);
	const FVector Top = SpawnBox->GetComponentTransform().TransformPosition(LocalPoint);
	const float TraceLength = Extent.Z * 2.f * SpawnBox->GetComponentScale().Z + 20000.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CTTailPickupGround), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Top, Top - FVector(0.f, 0.f, TraceLength), GroundTraceChannel, Params))
	{
		return false;
	}

	// 가파른 면(벽)은 피하고 평평한 바닥만
	if (Hit.ImpactNormal.Z < 0.8f) { return false; }

	// 박스의 높이 범위 밖(높은 건물 지붕, 깊은 구덩이)은 제외. 박스를 바닥 높이에 맞춰 두면 지상에만 생김
	const float BoxCenterZ = SpawnBox->GetComponentLocation().Z;
	const float BoxHalfZ = Extent.Z * SpawnBox->GetComponentScale().Z;
	if (Hit.ImpactPoint.Z > BoxCenterZ + BoxHalfZ || Hit.ImpactPoint.Z < BoxCenterZ - BoxHalfZ) { return false; }

	ACTTailPickup* Item = nullptr;
	FVector Location;
	if (bPopOutOnSpawn)
	{
		// 바닥 위 높은 곳에서 옆으로 살짝 튕기며 떨어져 땅에서 몇 번 튀고 멈춤
		Location = Hit.ImpactPoint + FVector(0.f, 0.f, PopStartHeight);
		const FTransform SpawnTransform(FRotator::ZeroRotator, Location);
		Item = World->SpawnActorDeferred<ACTTailPickup>(PickupClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Item) { return false; }

		const FVector Sideways = FVector::ForwardVector.RotateAngleAxis(FMath::FRandRange(0.f, 360.f), FVector::UpVector) * PopSideSpeed;
		Item->LaunchAway(Sideways + FVector(0.f, 0.f, PopUpSpeed), PopPickupDelay, PickupLifeSeconds);
		Item->FinishSpawning(SpawnTransform);
	}
	else
	{
		Location = Hit.ImpactPoint + FVector(0.f, 0.f, SpawnHeightAboveGround);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		Item = World->SpawnActor<ACTTailPickup>(PickupClass, Location, FRotator::ZeroRotator, SpawnParams);
		if (!Item) { return false; }

		if (PickupLifeSeconds > 0.f) { Item->SetLifeSpan(PickupLifeSeconds); }
	}
	Spawned.Add(Item);

	UE_LOG(LogTemp, Log, TEXT("[CT] 꼬리 아이템 생성: (%.0f, %.0f, %.0f)"), Location.X, Location.Y, Location.Z);
	return true;
}
