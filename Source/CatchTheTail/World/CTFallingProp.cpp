// Copyright Moon Party Games. All Rights Reserved.
#include "CTFallingProp.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

ACTFallingProp::ACTFallingProp()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicatingMovement(true);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetMobility(EComponentMobility::Movable);

	// 충돌은 스폰 볼륨이 낙하/착지 때 지정
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

	// 카메라 차단 안 함
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void ACTFallingProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTFallingProp, MeshAsset);
	DOREPLIFETIME(ACTFallingProp, UniformScale);
}

void ACTFallingProp::Setup(UStaticMesh* InMesh, float InUniformScale)
{
	MeshAsset = InMesh;
	UniformScale = FMath::Max(InUniformScale, 0.01f);
	ApplySetup();
}

void ACTFallingProp::OnRep_Setup()
{
	ApplySetup();
}

void ACTFallingProp::ApplySetup()
{
	if (MeshComponent && MeshAsset)
	{
		MeshComponent->SetStaticMesh(MeshAsset);
	}
	SetActorScale3D(FVector(UniformScale));
}
