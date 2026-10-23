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

	// 충돌은 스폰 볼륨이 떨어지는 동안 / 착지한 뒤로 나누어 정해 줍니다. (기본은 막힘)
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

	// 카메라는 막지 않습니다. (벽에 쓰는 충돌과 달리, 떨어진 물체 때문에 카메라가 당겨지지 않게)
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
