// Copyright Moon Party Games. All Rights Reserved.
#include "CTThrowable.h"
#include "../Core/CTGameMode.h"
#include "../Player/CTCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

ACTThrowable::ACTThrowable()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(30.f);

	// 물리 몸체: 구
	Body = CreateDefaultSubobject<USphereComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->InitSphereRadius(BodyRadius);
	Body->SetHiddenInGame(true);

	// 보이는 모양: 몸체에 붙이고 충돌은 끔
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Body);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 엔진 기본 구를 공으로 씀 (블루프린트에서 바꿀 수 있음)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded()) { Mesh->SetStaticMesh(SphereFinder.Object); }

	Body->SetCollisionProfileName(TEXT("PhysicsActor"));
	Body->SetSimulatePhysics(true);
	Body->SetNotifyRigidBodyCollision(true); // 맞은 것을 알기 위해 필요
	Body->SetCanEverAffectNavigation(false);
	Body->BodyInstance.bUseCCD = true; // 빠르게 던져도 뚫고 지나가지 않게
	Body->SetLinearDamping(0.05f);
	Body->SetAngularDamping(0.4f);
	Mesh->SetCastShadow(false);
	Mesh->SetCanEverAffectNavigation(false);
}

void ACTThrowable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACTThrowable, HeldBy);
}

ACTThrowable* ACTThrowable::Spawn(UWorld* World, TSubclassOf<ACTThrowable> ThrowableClass, const FVector& Location)
{
	if (!World) { return nullptr; }

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	UClass* ClassToSpawn = ThrowableClass ? ThrowableClass.Get() : ACTThrowable::StaticClass();
	return World->SpawnActor<ACTThrowable>(ClassToSpawn, Location, FRotator::ZeroRotator, Params);
}

void ACTThrowable::BeginPlay()
{
	Super::BeginPlay();

	// 블루프린트에서 바꾼 반지름과 크기를 적용 (모양 위치는 구 중심)
	Body->SetSphereRadius(BodyRadius);
	Mesh->SetRelativeScale3D(FVector(VisualScale));
	Body->SetMassOverrideInKg(NAME_None, MassKg, true);

	// 튕기는 정도와 마찰
	UPhysicalMaterial* PhysMaterial = NewObject<UPhysicalMaterial>(this);
	PhysMaterial->Restitution = Restitution;
	PhysMaterial->Friction = Friction;
	PhysMaterial->bOverrideRestitutionCombineMode = true;
	PhysMaterial->RestitutionCombineMode = EFrictionCombineMode::Max;
	Body->SetPhysMaterialOverride(PhysMaterial);

	// 색만 바꿔 쓰는 머티리얼 (없으면 메시 기본 머티리얼)
	if (UMaterialInterface* Base = bUseBallMaterial ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/VFX/M_CT_EnergyOrb.M_CT_EnergyOrb")) : nullptr)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("Color"), BallColor);
		Material->SetScalarParameterValue(TEXT("Intensity"), 1.2f);
		Mesh->SetMaterial(0, Material);
	}

	if (HasAuthority())
	{
		Body->OnComponentHit.AddDynamic(this, &ACTThrowable::HandleHit);
	}
}

void ACTThrowable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 들고 있던 사람의 손에서 빼 줌
	if (HasAuthority() && HeldBy)
	{
		HeldBy->ClearHeldThrowable(this);
		HeldBy = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ACTThrowable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 서버: 던진 뒤 일정 시간이 지났거나 거의 멈추면 위험이 끝남
	if (HasAuthority() && bDangerous)
	{
		const double Elapsed = GetWorld()->GetTimeSeconds() - ThrowTime;
		const bool bSlow = Elapsed > 0.6 && Body->GetPhysicsLinearVelocity().SizeSquared() < FMath::Square(150.f);
		if (Elapsed >= DangerSeconds || bSlow)
		{
			bDangerous = false;
			HitVictims.Reset();
			Thrower.Reset();
		}
	}
}

// 들기 / 던지기

void ACTThrowable::ApplyHeldState()
{
	if (HeldBy)
	{
		// 물리와 충돌을 끄고 캐릭터 머리 위에 붙임
		Body->SetSimulatePhysics(false);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (HasAuthority())
		{
			AttachToComponent(HeldBy->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SetActorRelativeLocation(HeldOffset);
		}
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Body->SetCollisionProfileName(TEXT("PhysicsActor"));
		Body->SetSimulatePhysics(true);
	}
}

void ACTThrowable::OnRep_HeldBy()
{
	ApplyHeldState();
}

void ACTThrowable::AttachToHolder(ACTCharacter* NewHolder)
{
	if (!HasAuthority() || !NewHolder) { return; }

	bDangerous = false;
	HitVictims.Reset();
	Thrower.Reset();
	HeldBy = NewHolder;
	ApplyHeldState();
	ForceNetUpdate();
}

void ACTThrowable::Release()
{
	if (!HasAuthority() || !HeldBy) { return; }

	HeldBy = nullptr;
	ApplyHeldState();
	ForceNetUpdate();
}

void ACTThrowable::Throw(const FVector& Velocity)
{
	if (!HasAuthority() || !HeldBy) { return; }

	ACTCharacter* ThrowingCharacter = HeldBy;
	Release();

	// 던진 사람 몸에 바로 맞지 않게 잠깐 무시
	if (UPrimitiveComponent* ThrowerRoot = Cast<UPrimitiveComponent>(ThrowingCharacter->GetRootComponent()))
	{
		Body->IgnoreComponentWhenMoving(ThrowerRoot, true);
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, ThrowerRoot]()
		{
			if (ThrowerRoot) { Body->IgnoreComponentWhenMoving(ThrowerRoot, false); }
		}), ThrowerSafeSeconds, false);
	}

	Thrower = ThrowingCharacter;
	bDangerous = true;
	ThrowTime = GetWorld()->GetTimeSeconds();
	HitVictims.Reset();

	Body->SetPhysicsLinearVelocity(Velocity);
	Body->SetPhysicsAngularVelocityInDegrees(FVector(0.f, 720.f, 0.f));
}

// 맞힘 (서버)

void ACTThrowable::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || !bDangerous) { return; }

	ACTCharacter* Victim = Cast<ACTCharacter>(OtherActor);
	if (!Victim || HitVictims.Contains(Victim)) { return; }
	HitVictims.Add(Victim);

	if (ACTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTGameMode>() : nullptr)
	{
		GameMode->HandleThrowableHit(Thrower.Get(), Victim, GetVelocity());
	}
}
