// Copyright Moon Party Games. All Rights Reserved.
#include "CTExplosionFx.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void SetupFxComponent(UPrimitiveComponent* C)
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetCanEverAffectNavigation(false);
	}
}

ACTExplosionFx::ACTExplosionFx()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);
	InitialLifeSpan = 3.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Fireball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fireball"));
	SetRootComponent(Fireball);
	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	Ring->SetupAttachment(Fireball);
	BlobMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Blobs"));
	BlobMeshes->SetupAttachment(Fireball);
	SparkMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Sparks"));
	SparkMeshes->SetupAttachment(Fireball);
	SmokeMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Smoke"));
	SmokeMeshes->SetupAttachment(Fireball);

	for (UStaticMeshComponent* C : { Fireball.Get(), Ring.Get(), static_cast<UStaticMeshComponent*>(BlobMeshes.Get()), static_cast<UStaticMeshComponent*>(SparkMeshes.Get()), static_cast<UStaticMeshComponent*>(SmokeMeshes.Get()) })
	{
		if (SphereFinder.Succeeded()) { C->SetStaticMesh(SphereFinder.Object); }
		SetupFxComponent(C);
	}
}

ACTExplosionFx* ACTExplosionFx::Spawn(UWorld* World, const FVector& Location, float InRadius)
{
	if (!World) { return nullptr; }
	ACTExplosionFx* Fx = World->SpawnActorDeferred<ACTExplosionFx>(ACTExplosionFx::StaticClass(), FTransform(Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Fx) { return nullptr; }
	Fx->Radius = FMath::Max(InRadius, 100.f);
	Fx->FinishSpawning(FTransform(Location));
	return Fx;
}

void ACTExplosionFx::SetFx(UMaterialInstanceDynamic* Material, const FLinearColor& Color, float Brightness, float Opacity)
{
	if (!Material) { return; }
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetScalarParameterValue(TEXT("Brightness"), Brightness);
	Material->SetScalarParameterValue(TEXT("Opacity"), Opacity);
}

void ACTExplosionFx::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/M_CT_ExplosionFx.M_CT_ExplosionFx")))
	{
		FireMat = UMaterialInstanceDynamic::Create(Base, this);
		RingMat = UMaterialInstanceDynamic::Create(Base, this);
		SparkMat = UMaterialInstanceDynamic::Create(Base, this);
		SmokeMat = UMaterialInstanceDynamic::Create(Base, this);
		Ring->SetMaterial(0, RingMat);
		BlobMeshes->SetMaterial(0, FireMat);
		SparkMeshes->SetMaterial(0, SparkMat);
		SmokeMeshes->SetMaterial(0, SmokeMat);
	}
	Build();
	SetLifeSpan(TotalSeconds + 0.2f);
}

void ACTExplosionFx::Build()
{
	const FVector Origin = GetActorLocation();
	Fireball->SetVisibility(false, false);
	for (int32 i = 0; i < 7; ++i)
	{
		FPart P;
		const float Yaw = FMath::FRandRange(0.f, 2.f * PI);
		const float Dist = FMath::FRandRange(0.f, 0.4f) * Radius;
		P.Position = Origin + FVector(FMath::Cos(Yaw) * Dist, FMath::Sin(Yaw) * Dist, FMath::FRandRange(20.f, 0.45f * Radius));
		P.Size = FMath::FRandRange(0.35f, 0.6f) * Radius;
		P.Life = FMath::FRandRange(0.4f, 0.6f);
		P.Velocity.X = FMath::FRandRange(0.f, 0.1f); // 시작 지연(초)
		P.InstanceIndex = BlobMeshes->AddInstance(FTransform(FQuat::Identity, P.Position, FVector(0.f)), true);
		Blobs.Add(P);
	}
	Ring->SetWorldScale3D(FVector(0.f));

	for (int32 i = 0; i < SparkCount; ++i)
	{
		FPart P;
		P.Position = Origin + FVector(0.f, 0.f, 40.f);
		const float Yaw = FMath::FRandRange(0.f, 2.f * PI);
		const float Pitch = FMath::FRandRange(0.15f, 1.3f);
		const FVector Dir(FMath::Cos(Yaw) * FMath::Cos(Pitch), FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch));
		P.Velocity = Dir * FMath::FRandRange(0.9f, 2.6f) * Radius;
		P.Size = FMath::FRandRange(14.f, 30.f) * Radius / 450.f;
		P.Life = FMath::FRandRange(0.6f, 1.2f);
		P.InstanceIndex = SparkMeshes->AddInstance(FTransform(FQuat::Identity, P.Position, FVector(0.f)), true);
		Sparks.Add(P);
	}
	for (int32 i = 0; i < SmokeCount; ++i)
	{
		FPart P;
		const float Yaw = FMath::FRandRange(0.f, 2.f * PI);
		const float Dist = FMath::FRandRange(0.f, 0.45f) * Radius;
		P.Position = Origin + FVector(FMath::Cos(Yaw) * Dist, FMath::Sin(Yaw) * Dist, FMath::FRandRange(20.f, 120.f));
		P.Velocity = FVector(FMath::Cos(Yaw) * 80.f, FMath::Sin(Yaw) * 80.f, FMath::FRandRange(120.f, 260.f));
		P.Size = FMath::FRandRange(0.25f, 0.45f) * Radius;
		P.Life = FMath::FRandRange(1.0f, TotalSeconds);
		P.InstanceIndex = SmokeMeshes->AddInstance(FTransform(FQuat::Identity, P.Position, FVector(0.f)), true);
		Smokes.Add(P);
	}
}

void ACTExplosionFx::Tick(float Dt)
{
	Super::Tick(Dt);
	Age += Dt;
	const FVector Origin = GetActorLocation();

	// 불덩이: 여러 덩어리가 시차를 두고 부풀며 흰노랑 → 주황 → 붉게 식으며 사라짐
	for (FPart& P : Blobs)
	{
		const float Local = FMath::Max(Age - P.Velocity.X, 0.f);
		const float T = FMath::Clamp(Local / P.Life, 0.f, 1.f);
		const float Grow = 1.f - FMath::Square(1.f - FMath::Clamp(Local / 0.25f, 0.f, 1.f));
		const float S = P.Size * Grow * (1.f - 0.5f * T * T) / 100.f;
		BlobMeshes->UpdateInstanceTransform(P.InstanceIndex, FTransform(FQuat::Identity, P.Position + FVector(0.f, 0.f, 120.f * T), FVector(T < 1.f ? S : 0.f)), true, false, true);
	}
	BlobMeshes->MarkRenderStateDirty();
	{
		const float T = FMath::Clamp(Age / 0.55f, 0.f, 1.f);
		const FLinearColor Hot(1.f, 0.7f, 0.12f), Warm(1.f, 0.28f, 0.02f), Cool(0.35f, 0.04f, 0.015f);
		const FLinearColor C = T < 0.4f ? FMath::Lerp(Hot, Warm, T / 0.4f) : FMath::Lerp(Warm, Cool, (T - 0.4f) / 0.6f);
		SetFx(FireMat, C, FMath::Lerp(1.6f, 0.8f, T), FMath::Clamp(1.4f - 1.4f * T * T, 0.f, 1.f));
	}

	// 충격파 고리: 납작한 구가 퍼져 나감
	{
		const float T = FMath::Clamp(Age / 0.45f, 0.f, 1.f);
		const float S = Radius * (0.2f + 0.9f * (1.f - FMath::Square(1.f - T))) / 50.f;
		Ring->SetWorldScale3D(FVector(S, S, S * 0.025f));
		Ring->SetWorldLocation(Origin + FVector(0.f, 0.f, 25.f));
		SetFx(RingMat, FLinearColor(1.f, 0.55f, 0.15f), 1.2f, 0.28f * (1.f - T));
		Ring->SetVisibility(T < 1.f);
	}

	// 불꽃: 사방으로 튀어 포물선으로 떨어지며 작아짐
	for (FPart& P : Sparks)
	{
		P.Velocity.Z -= 1500.f * Dt;
		P.Velocity *= FMath::Max(0.f, 1.f - 1.2f * Dt);
		P.Position += P.Velocity * Dt;
		const float T = FMath::Clamp(Age / P.Life, 0.f, 1.f);
		const float Size = P.Size * (1.f - T * T) / 100.f;
		SparkMeshes->UpdateInstanceTransform(P.InstanceIndex, FTransform(FQuat::Identity, P.Position, FVector(FMath::Max(Size, 0.f))), true, false, true);
	}
	SparkMeshes->MarkRenderStateDirty();
	SetFx(SparkMat, FLinearColor(1.f, 0.6f, 0.1f), 2.5f, 1.f);

	// 연기: 위로 떠오르며 커지고 옅어짐
	for (FPart& P : Smokes)
	{
		P.Position += P.Velocity * Dt;
		P.Velocity *= FMath::Max(0.f, 1.f - 0.8f * Dt);
		const float T = FMath::Clamp(Age / P.Life, 0.f, 1.f);
		const float Size = P.Size * (0.4f + 0.9f * T) * FMath::Clamp(Age / 0.2f, 0.f, 1.f) / 100.f;
		SmokeMeshes->UpdateInstanceTransform(P.InstanceIndex, FTransform(FQuat::Identity, P.Position, FVector(Size)), true, false, true);
	}
	SmokeMeshes->MarkRenderStateDirty();
	{
		const float T = FMath::Clamp(Age / TotalSeconds, 0.f, 1.f);
		SetFx(SmokeMat, FLinearColor(0.3f, 0.26f, 0.24f), 1.f, 0.3f * FMath::Clamp(1.f - T, 0.f, 1.f) * FMath::Clamp(Age / 0.15f, 0.f, 1.f));
	}
}
