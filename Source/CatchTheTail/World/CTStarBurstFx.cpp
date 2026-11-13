// Copyright Moon Party Games. All Rights Reserved.
#include "CTStarBurstFx.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACTStarBurstFx::ACTStarBurstFx()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);
	InitialLifeSpan = 2.f;

	// 별 한 개를 판 하나로 그림 (엔진 기본 Plane)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));

	GoldStars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GoldStars"));
	SetRootComponent(GoldStars);
	PinkStars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PinkStars"));
	PinkStars->SetupAttachment(GoldStars);

	for (UInstancedStaticMeshComponent* Component : { GoldStars.Get(), PinkStars.Get() })
	{
		if (PlaneFinder.Succeeded()) { Component->SetStaticMesh(PlaneFinder.Object); }
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		Component->SetCanEverAffectNavigation(false);
	}
}

ACTStarBurstFx* ACTStarBurstFx::Spawn(UWorld* World, const FVector& Location, float Scale)
{
	if (!World) { return nullptr; }

	// 크기 배율을 BeginPlay 전에 넣기 위해 지연 스폰
	ACTStarBurstFx* Effect = World->SpawnActorDeferred<ACTStarBurstFx>(
		ACTStarBurstFx::StaticClass(), FTransform(Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Effect) { return nullptr; }

	Effect->EffectScale = FMath::Max(Scale, 0.1f);
	Effect->FinishSpawning(FTransform(Location));
	return Effect;
}

void ACTStarBurstFx::BeginPlay()
{
	Super::BeginPlay();

	// 별 그림 머티리얼에 색만 바꿔 쓰는 인스턴스 (색 파라미터 이름: Color)
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/M_CT_StarParticle.M_CT_StarParticle")))
	{
		UMaterialInstanceDynamic* GoldMaterial = UMaterialInstanceDynamic::Create(Base, this);
		GoldMaterial->SetVectorParameterValue(TEXT("Color"), GoldColor);
		GoldStars->SetMaterial(0, GoldMaterial);

		UMaterialInstanceDynamic* PinkMaterial = UMaterialInstanceDynamic::Create(Base, this);
		PinkMaterial->SetVectorParameterValue(TEXT("Color"), PinkColor);
		PinkStars->SetMaterial(0, PinkMaterial);
	}

	BuildStars();
}

void ACTStarBurstFx::BuildStars()
{
	Stars.Reset();
	GoldStars->ClearInstances();
	PinkStars->ClearInstances();

	const FVector Origin = GetActorLocation();
	for (int32 Index = 0; Index < StarCount; ++Index)
	{
		FStar Star;
		Star.bPink = (Index % 3 == 2);
		Star.Position = Origin;

		// 위쪽으로 넓게 퍼지는 방향 (수평으로 둥글게, 위로 치우침)
		const float Yaw = FMath::FRandRange(0.f, 2.f * PI);
		const float Pitch = FMath::FRandRange(0.25f, 1.25f);
		const FVector Direction(FMath::Cos(Yaw) * FMath::Cos(Pitch), FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch));
		Star.Velocity = Direction * FMath::FRandRange(SpeedRange.X, SpeedRange.Y) * EffectScale;
		Star.Size = FMath::FRandRange(SizeRange.X, SizeRange.Y) * EffectScale;
		Star.Life = FMath::FRandRange(LifeRange.X, LifeRange.Y);
		Star.Roll = FMath::FRandRange(0.f, 360.f);
		Star.Spin = FMath::FRandRange(-360.f, 360.f);

		UInstancedStaticMeshComponent* Target = Star.bPink ? PinkStars.Get() : GoldStars.Get();
		Star.InstanceIndex = Target->AddInstance(FTransform(FQuat::Identity, Origin, FVector(0.f)), /*bWorldSpace=*/true);
		Stars.Add(Star);
	}

	// 가장 오래 사는 별이 끝난 뒤 스스로 사라짐
	SetLifeSpan(LifeRange.Y + 0.2f);
}

void ACTStarBurstFx::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateStars(DeltaSeconds);
}

void ACTStarBurstFx::UpdateStars(float DeltaSeconds)
{
	// 별이 항상 화면을 보도록 카메라 방향에 맞춤
	FVector CameraForward = FVector::ForwardVector;
	if (const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (Controller->PlayerCameraManager) { CameraForward = Controller->PlayerCameraManager->GetCameraRotation().Vector(); }
	}
	const FQuat FaceCamera = FRotationMatrix::MakeFromZ(-CameraForward).ToQuat();

	for (FStar& Star : Stars)
	{
		Star.Age += DeltaSeconds;
		Star.Velocity.Z -= Gravity * DeltaSeconds;
		Star.Velocity *= FMath::Max(0.f, 1.f - 1.5f * DeltaSeconds); // 공기 저항
		Star.Position += Star.Velocity * DeltaSeconds;

		// 크기: 빠르게 커졌다가 서서히 작아지며 사라짐
		const float T = FMath::Clamp(Star.Age / Star.Life, 0.f, 1.f);
		const float Pop = FMath::Clamp(T / 0.12f, 0.f, 1.f);
		const float Fade = 1.f - FMath::Square(FMath::Clamp((T - 0.35f) / 0.65f, 0.f, 1.f));
		const float Size = Star.Size * Pop * Fade;

		const FQuat Spin(FVector::UpVector, FMath::DegreesToRadians(Star.Roll + Star.Spin * Star.Age));
		const FTransform Transform(FaceCamera * Spin, Star.Position, FVector(Size / 100.f)); // Plane 한 변 = 100cm

		UInstancedStaticMeshComponent* Target = Star.bPink ? PinkStars.Get() : GoldStars.Get();
		Target->UpdateInstanceTransform(Star.InstanceIndex, Transform, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/true, /*bTeleport=*/true);
	}
}
