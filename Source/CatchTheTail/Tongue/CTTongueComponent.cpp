// Copyright Moon Party Games. All Rights Reserved.
#include "CTTongueComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

UCTTongueComponent::UCTTongueComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// 이동을 방해하거나 다른 것과 부딪히지 않는 연출용 메시
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);

	// 기본 혀 메시. (블루프린트에서 다른 메시로 바꿀 수 있습니다)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> TongueAsset(TEXT("/Game/Character/Socket/TONGUE.TONGUE"));
	if (TongueAsset.Succeeded())
	{
		SetSkinnedAssetAndUpdate(TongueAsset.Object);
	}
}

void UCTTongueComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bCompensateAttachBoneRefPose)
	{
		CompensateAttachBone();
	}
}

void UCTTongueComponent::CompensateAttachBone()
{
	const USkeletalMeshComponent* ParentMesh = Cast<USkeletalMeshComponent>(GetAttachParent());
	const FName SocketBone = GetAttachSocketName();
	const USkeletalMesh* ParentAsset = ParentMesh ? ParentMesh->GetSkeletalMeshAsset() : nullptr;
	if (!ParentAsset || SocketBone.IsNone()) { return; }

	const FReferenceSkeleton& RefSkeleton = ParentAsset->GetRefSkeleton();
	const int32 BoneIndex = RefSkeleton.FindBoneIndex(SocketBone);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 혀를 붙일 본 '%s'을(를) 캐릭터 메시에서 찾지 못했습니다."), *SocketBone.ToString());
		return;
	}

	// 소켓 본의 기준 자세(컴포넌트 공간) = 자식 → 부모 순으로 곱함
	const TArray<FTransform>& RefPose = RefSkeleton.GetRefBonePose();
	FTransform BoneComponentTransform = FTransform::Identity;
	for (int32 Index = BoneIndex; Index != INDEX_NONE; Index = RefSkeleton.GetParentIndex(Index))
	{
		BoneComponentTransform = BoneComponentTransform * RefPose[Index];
	}

	// 혀 메시는 캐릭터와 같은 모델 좌표로 만들어져 있으므로, 붙은 본의 기준 자세를 되돌려 줍니다.
	SetRelativeTransform(BoneComponentTransform.Inverse());
}

bool UCTTongueComponent::InitializeChain()
{
	const USkinnedAsset* Asset = GetSkinnedAsset();
	if (!Asset || !RequiredBones.IsValid()) { return false; }

	const FReferenceSkeleton& RefSkeleton = Asset->GetRefSkeleton();
	const TArray<FTransform>& RefPose = RefSkeleton.GetRefBonePose();
	const int32 BoneCount = RefSkeleton.GetNum();
	if (BoneCount < 2) { return false; }

	// 컴포넌트 공간 기준 자세 (부모가 항상 자식보다 앞 번호)
	TArray<FTransform> ComponentPose;
	ComponentPose.SetNum(BoneCount);
	for (int32 Index = 0; Index < BoneCount; ++Index)
	{
		const int32 ParentIndex = RefSkeleton.GetParentIndex(Index);
		ComponentPose[Index] = (ParentIndex == INDEX_NONE) ? RefPose[Index] : RefPose[Index] * ComponentPose[ParentIndex];
	}

	ChainBones.Reset();
	RefComponentRotations.Reset();
	for (int32 Index = 1; Index < BoneCount; ++Index) // 0번(루트)은 그대로 둡니다.
	{
		ChainBones.Add(RefSkeleton.GetBoneName(Index));
		RefComponentRotations.Add(ComponentPose[Index].GetRotation());
	}

	bChainInitialized = ChainBones.Num() > 0;
	return bChainInitialized;
}

void UCTTongueComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 화면이 없는 전용 서버에서는 연출이 필요 없습니다.
	if (GetNetMode() == NM_DedicatedServer) { return; }

	if (!bChainInitialized && !InitializeChain()) { return; }

	// 이동 속도에 따라 흔들림 세기를 부드럽게 따라갑니다.
	const AActor* OwnerActor = GetOwner();
	const float Speed = OwnerActor ? OwnerActor->GetVelocity().Size() : 0.f;
	const float TargetAlpha = FMath::Clamp(Speed / FMath::Max(FullWaveSpeed, 1.f), 0.f, 1.f);
	SmoothedAlpha = FMath::FInterpTo(SmoothedAlpha, TargetAlpha, DeltaTime, SpeedResponse);

	const float Frequency = FMath::Lerp(IdleFrequency, MoveFrequency, SmoothedAlpha);
	const float AmplitudeDegrees = FMath::Lerp(IdleAmplitudeDegrees, MoveAmplitudeDegrees, SmoothedAlpha);
	Phase = FMath::Fmod(Phase + UE_TWO_PI * Frequency * DeltaTime, UE_TWO_PI); // 주파수가 바뀌어도 끊기지 않게 누적

	FVector Axis = WaveAxis.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		Axis = FVector::UpVector;
	}

	// 루트 쪽 본부터 차례로 회전. 부모가 먼저 바뀌므로 자식은 그 위치를 따라갑니다.
	const int32 ChainLength = ChainBones.Num();
	for (int32 Index = 0; Index < ChainLength; ++Index)
	{
		const float Weight = FMath::Pow(static_cast<float>(Index + 1) / ChainLength, AmplitudeExponent);
		const float AngleRadians = FMath::DegreesToRadians(AmplitudeDegrees * Weight) * FMath::Sin(Phase - Index * PhasePerBoneRadians);
		const FQuat NewRotation = FQuat(Axis, AngleRadians) * RefComponentRotations[Index];

		SetBoneRotationByName(ChainBones[Index], NewRotation.Rotator(), EBoneSpaces::ComponentSpace);
	}
}
