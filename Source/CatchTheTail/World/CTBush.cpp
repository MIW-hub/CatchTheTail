// Copyright Moon Party Games. All Rights Reserved.
#include "CTBush.h"
#include "../Core/CTGameInstance.h"
#include "../Player/CTCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 사람이 안에 있는지 다시 확인하는 간격(초)
	constexpr float OccupantCheckInterval = 0.1f;
}

ACTBush::ACTBush()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	Trigger->SetGenerateOverlapEvents(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(3.f, 3.f, 2.f));

	// 임시 모양 (블루프린트에서 풀 메시로 교체)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		Mesh->SetStaticMesh(SphereFinder.Object);
	}
}

void ACTBush::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACTBush, bAlerted);
}

void ACTBush::BeginPlay()
{
	Super::BeginPlay();

	MeshBaseRotation = Mesh->GetRelativeRotation();
	ShakePhase = FMath::FRandRange(0.f, 6.28f); // 부쉬마다 다른 박자

	// 임시 모양일 때만 초록색 (메시를 바꿨으면 그 머티리얼 그대로)
	if (Mesh->GetNumMaterials() > 0 && Mesh->GetMaterial(0) && Mesh->GetMaterial(0)->GetName().Contains(TEXT("BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 0.45f, 0.12f, 1.f));
		}
	}
}

void ACTBush::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		OccupantCheckAccumulator += DeltaSeconds;
		if (OccupantCheckAccumulator >= OccupantCheckInterval)
		{
			OccupantCheckAccumulator = 0.f;
			ServerRefreshOccupants();
		}
	}

	// 흔들림 (모든 화면). 들키면 빠르고 크게
	if (GetNetMode() != NM_DedicatedServer)
	{
		const float Speed = bAlerted ? AlertShakeSpeed : IdleShakeSpeed;
		const float Angle = bAlerted ? AlertShakeAngle : IdleShakeAngle;
		ShakePhase += DeltaSeconds * Speed * 2.f * PI;
		const float Wave = FMath::Sin(ShakePhase);
		Mesh->SetRelativeRotation(MeshBaseRotation + FRotator(Wave * Angle, 0.f, FMath::Cos(ShakePhase * 0.8f) * Angle));
	}
}

void ACTBush::ServerRefreshOccupants()
{
	const double Now = GetWorld()->GetTimeSeconds();

	// 지금 안에 있는 (유령 아닌) 캐릭터
	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping, ACTCharacter::StaticClass());

	TSet<ACTCharacter*> Inside;
	for (AActor* Actor : Overlapping)
	{
		ACTCharacter* Character = Cast<ACTCharacter>(Actor);
		if (Character && !Character->IsGhost())
		{
			Inside.Add(Character);
		}
	}

	// 방장이 부쉬를 끈 판에서는 아무도 숨을 수 없음
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	if (GameInstance && GameInstance->HasStoredMatchSettings() && !GameInstance->GetStoredMatchSettings().IsBushEnabled())
	{
		Inside.Reset();
	}

	// 나간 사람
	for (auto It = Occupants.CreateIterator(); It; ++It)
	{
		ACTCharacter* Character = It.Key().Get();
		if (!Character || !Inside.Contains(Character))
		{
			if (Character && Character->GetCurrentBush() == this) { Character->SetCurrentBush(nullptr); }
			It.RemoveCurrent();
		}
	}

	// 들어온 사람 (다른 부쉬에 이미 있으면 그쪽 유지)
	for (ACTCharacter* Character : Inside)
	{
		if (Occupants.Contains(Character)) { continue; }
		if (Character->GetCurrentBush() && Character->GetCurrentBush() != this) { continue; }

		Occupants.Add(Character, Now);
		Character->SetCurrentBush(this);
	}

	// 오래 숨은 사람이 있으면 들킴
	bool bNewAlerted = false;
	for (const TPair<TWeakObjectPtr<ACTCharacter>, double>& Pair : Occupants)
	{
		if (Now - Pair.Value >= AlertDelaySeconds) { bNewAlerted = true; break; }
	}

	if (bNewAlerted != bAlerted)
	{
		bAlerted = bNewAlerted;
		OnRep_Alerted();
	}
}

void ACTBush::OnRep_Alerted()
{
	BP_OnAlertChanged(bAlerted);
}
