// Copyright Moon Party Games. All Rights Reserved.
#include "CTTailPickup.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACTTailPickup::ACTTailPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicatingMovement(true);

	// 닿음 판정. 캐릭터는 겹침, 바닥과 벽은 막혀서 튕겨 나온 꼬리가 땅에 떨어짐
	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->InitSphereRadius(70.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->SetMobility(EComponentMobility::Movable);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(Trigger);

	// 꼬리 한 마디: 받침 + 끝 캡. 캐릭터 꼬리와 같은 메시
	BaseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(VisualRoot);
	BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BaseMesh->SetGenerateOverlapEvents(false);

	// 몸통 한 마디 (캐릭터 꼬리 1마디와 같은 구성: 받침 + 몸통 + 끝 캡)
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(VisualRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);

	CapMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CapMesh"));
	CapMesh->SetupAttachment(VisualRoot);
	CapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapMesh->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BaseFinder(TEXT("/Game/Character/Socket/tail_03.tail_03"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyFinder(TEXT("/Game/Character/Socket/tail_02.tail_02"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CapFinder(TEXT("/Game/Character/Socket/tail_01.tail_01"));
	if (BodyFinder.Succeeded()) { BodyMesh->SetSkeletalMeshAsset(BodyFinder.Object); }
	if (BaseFinder.Succeeded()) { BaseMesh->SetSkeletalMeshAsset(BaseFinder.Object); }
	if (CapFinder.Succeeded()) { CapMesh->SetSkeletalMeshAsset(CapFinder.Object); }

	// 평소에는 가만히 놓여 있고, LaunchAway 를 부를 때만 날아감
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Trigger;
	Movement->bAutoActivate = false;
	Movement->bShouldBounce = true;
	Movement->Bounciness = 0.35f;
	Movement->Friction = 0.6f;
	Movement->ProjectileGravityScale = 1.5f;
	Movement->InitialSpeed = 0.f;
}

void ACTTailPickup::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACTTailPickup::HandleBeginOverlap);

	if (ItemMaterial)
	{
		// 받침(tail_03)은 원래 머티리얼 그대로, 몸통과 캡만 색이 바뀜
		for (USkeletalMeshComponent* Part : { BodyMesh.Get(), CapMesh.Get() })
		{
			for (int32 SlotIndex = 0; Part && SlotIndex < Part->GetNumMaterials(); ++SlotIndex)
			{
				Part->SetMaterial(SlotIndex, ItemMaterial);
			}
		}
	}

	// LaunchAway 로 이미 정해졌으면 그 값을 씀
	if (UWorld* World = GetWorld(); World && !bLaunchConfigured)
	{
		PickupReadyTime = World->GetTimeSeconds() + PickupDelaySeconds;
	}
	if (HasAuthority() && LifeSeconds > 0.f && !bLaunchConfigured)
	{
		SetLifeSpan(LifeSeconds);
	}

	// 서로 다른 아이템이 같은 모양으로 흔들리지 않게 시작 위상을 흩음
	VisualTime = FMath::FRandRange(0.f, 10.f);
}

int32 ACTTailPickup::DropTailsFromCharacter(ACTCharacter* Character, int32 TailLoss, const FVector& Direction, TSubclassOf<ACTTailPickup> PickupClass,
	float LaunchSpeed, float UpSpeed, float PickupDelay, float LifeSeconds, UObject* OwnerObject)
{
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!World || !CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing || !World->GetAuthGameMode()) { return 0; }

	ACTPlayerState* PlayerState = Character->GetPlayerState<ACTPlayerState>();
	if (!PlayerState || PlayerState->IsForfeited() || PlayerState->GetTailCount() <= 0) { return 0; }

	// 실제로 잃은 만큼만 아이템으로 (꼬리가 모자라면 있는 만큼)
	const int32 Lost = FMath::Min(TailLoss, PlayerState->GetTailCount());
	PlayerState->AddTailCount(-Lost);

	// 꼬리가 하나도 남지 않으면 잡혔을 때처럼 유령이 됨
	if (PlayerState->GetTailCount() <= 0)
	{
		PlayerState->SetIsGhost(true);
		UE_LOG(LogTemp, Log, TEXT("[CT] %s 꼬리를 모두 잃어 유령이 됨"), *PlayerState->GetPlayerName());
	}

	if (!PickupClass) { return Lost; }

	// 머리 위쪽에서 시작, 방향 주변으로 흩어짐
	for (int32 Index = 0; Index < Lost; ++Index)
	{
		const FTransform SpawnTransform(FRotator::ZeroRotator, Character->GetActorLocation() + FVector(0.f, 0.f, 90.f));

		// 만들어지는 순간 바로 먹히지 않게, 대기 시간을 정한 뒤 월드에 내보냄
		AActor* SpawnOwner = Cast<AActor>(OwnerObject);
		ACTTailPickup* Dropped = World->SpawnActorDeferred<ACTTailPickup>(PickupClass, SpawnTransform, SpawnOwner, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Dropped) { continue; }

		const FVector Scatter = Direction.GetSafeNormal2D().RotateAngleAxis(FMath::FRandRange(-70.f, 70.f), FVector::UpVector);
		Dropped->LaunchAway(Scatter * LaunchSpeed + FVector(0.f, 0.f, UpSpeed), PickupDelay, LifeSeconds);
		Dropped->FinishSpawning(SpawnTransform);
	}

	return Lost;
}

void ACTTailPickup::LaunchAway(const FVector& Velocity, float NewPickupDelaySeconds, float NewLifeSeconds)
{
	if (!HasAuthority()) { return; }

	// SpawnActorDeferred 직후, BeginPlay 전에 불러야 만들어지는 순간 겹친 캐릭터가 바로 먹지 못함
	bLaunchConfigured = true;
	if (UWorld* World = GetWorld())
	{
		PickupReadyTime = World->GetTimeSeconds() + NewPickupDelaySeconds;
	}
	if (NewLifeSeconds > 0.f)
	{
		SetLifeSpan(NewLifeSeconds);
	}

	Movement->Velocity = Velocity;
	Movement->bAutoActivate = true;
}

void ACTTailPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 모든 화면: 돌고 위아래로 흔들림 (움직임 복제와 따로, 보이는 부분만)
	VisualTime += DeltaSeconds;
	VisualRoot->SetRelativeRotation(FRotator(0.f, VisualTime * SpinDegreesPerSecond, 0.f));
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(VisualTime * BobSpeed) * BobHeight));

	if (!HasAuthority() || bConsumed) { return; }

	// 대기가 끝난 뒤에도 이미 겹쳐 있는 캐릭터가 먹을 수 있게 확인
	if (GetWorld()->GetTimeSeconds() >= PickupReadyTime)
	{
		TArray<AActor*> Overlapping;
		Trigger->GetOverlappingActors(Overlapping, ACTCharacter::StaticClass());
		for (AActor* Actor : Overlapping)
		{
			TryPickup(Cast<ACTCharacter>(Actor));
			if (bConsumed) { break; }
		}
	}
}

void ACTTailPickup::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) { return; }
	TryPickup(Cast<ACTCharacter>(OtherActor));
}

void ACTTailPickup::TryPickup(ACTCharacter* Character)
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || bConsumed || !Character || !World) { return; }
	if (World->GetTimeSeconds() < PickupReadyTime) { return; }

	// 경기 중, 유령과 기권자는 먹을 수 없음
	const ACTGameState* CTGameState = World->GetGameState<ACTGameState>();
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return; }
	const bool bGhost = Character->IsGhost();
	if (bGhost && !bAllowGhostPickup) { return; }

	ACTPlayerState* PlayerState = Character->GetPlayerState<ACTPlayerState>();
	if (!PlayerState || PlayerState->IsForfeited()) { return; }

	bConsumed = true;
	if (bGhost)
	{
		// 유령이 먹으면 그 자리에서 부활, 꼬리는 최소 TailAmount 개
		PlayerState->SetIsGhost(false);
		if (PlayerState->GetTailCount() < TailAmount) { PlayerState->SetTailCount(TailAmount); }
		Character->RefreshChaseBuff();
		UE_LOG(LogTemp, Log, TEXT("[CT] 꼬리 아이템: %s 유령 부활 (꼬리 %d)"), *PlayerState->GetPlayerName(), PlayerState->GetTailCount());
	}
	else
	{
		PlayerState->AddTailCount(TailAmount);
		UE_LOG(LogTemp, Log, TEXT("[CT] 꼬리 아이템: %s +%d (꼬리 %d)"), *PlayerState->GetPlayerName(), TailAmount, PlayerState->GetTailCount());
	}

	BP_OnPickedUp(Character);
	Destroy();
}
