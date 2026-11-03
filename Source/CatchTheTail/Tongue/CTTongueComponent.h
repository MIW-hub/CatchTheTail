// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "CTTongueComponent.generated.h"

UCLASS(ClassGroup = (CT), meta = (BlueprintSpawnableComponent))
class CATCHTHETAIL_API UCTTongueComponent : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	UCTTongueComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	// 붙은 본 기준 자세 상쇄
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Attach")
	bool bCompensateAttachBoneRefPose = true;

	// 흔들림 축 (컴포넌트 공간)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave")
	FVector WaveAxis = FVector(0.f, 0.f, 1.f);

	// 정지 시 끝 본 각도 (도)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float IdleAmplitudeDegrees = 4.f;

	// 최대 속도 시 끝 본 각도 (도)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float MoveAmplitudeDegrees = 25.f;

	// 정지 시 초당 횟수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float IdleFrequency = 1.2f;

	// 최대 속도 시 초당 횟수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float MoveFrequency = 4.5f;

	// 본당 위상 지연 (rad)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float PhasePerBoneRadians = 0.9f;

	// 끝으로 갈수록 커지는 지수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float AmplitudeExponent = 1.f;

	// 최대 흔들림 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Speed", meta = (ClampMin = "1"))
	float FullWaveSpeed = 800.f;

	// 속도 반응 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Speed", meta = (ClampMin = "0.1"))
	float SpeedResponse = 6.f;

private:
	// 메시 준비 후 1회
	bool InitializeChain();

	void CompensateAttachBone();

	// 흔들 본 (루트 제외)
	TArray<FName> ChainBones;

	// 본별 기준 회전 (컴포넌트 공간)
	TArray<FQuat> RefComponentRotations;

	bool bChainInitialized = false;
	float Phase = 0.f;
	float SmoothedAlpha = 0.f;
};
