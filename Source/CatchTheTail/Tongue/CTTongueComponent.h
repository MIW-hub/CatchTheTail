// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "CTTongueComponent.generated.h"

/**
 * 로봇 캐릭터의 혀. 캐릭터 메시의 소켓(기본: Head 본)에 붙어서 물결치듯 흔들립니다.
 *
 * 동작
 *  - 혀 메시(TONGUE)의 본 체인(Tongue_01 → 02 → 03 → 04)을 코드로 회전시킵니다.
 *    애니메이션 블루프린트가 필요 없습니다. (Poseable 메시 컴포넌트)
 *  - 본마다 위상을 늦춘 사인파를 적용해서 뿌리에서 끝으로 파동이 전달되는 모양을 만듭니다.
 *  - 가만히 있으면 작고 느리게, 이동 속도가 빠를수록 크고 빠르게 흔들립니다.
 *  - 이동 속도는 복제된 캐릭터 속도에서 읽으므로 모든 클라이언트에서 같은 모양으로 보입니다.
 *    (연출만 하는 컴포넌트라 서버가 따로 계산하거나 복제하지 않습니다.)
 *
 * 붙이는 위치
 *  - 혀 메시는 캐릭터와 같은 모델 좌표계로 만들어져 있습니다. 그래서 소켓 본의 기준 자세(레퍼런스 포즈)를
 *    자동으로 상쇄해서, 소켓이 움직여도 입 위치에 맞게 따라갑니다.
 */
UCLASS(ClassGroup = (CT), meta = (BlueprintSpawnableComponent))
class CATCHTHETAIL_API UCTTongueComponent : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	UCTTongueComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	// ===== 붙이기 =====
	/** 붙은 본의 기준 자세를 상쇄할지. 혀 메시가 이미 소켓 기준으로 만들어졌다면 끄세요. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Attach")
	bool bCompensateAttachBoneRefPose = true;

	// ===== 흔들림 =====
	/** 흔들리는 축 (혀 메시 컴포넌트 공간). Z=좌우로 물결, X 또는 Y=위아래로 물결 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave")
	FVector WaveAxis = FVector(0.f, 0.f, 1.f);

	/** 가만히 있을 때 가장 끝 본이 흔들리는 각도 (도) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float IdleAmplitudeDegrees = 4.f;

	/** 최대 속도로 이동할 때 가장 끝 본이 흔들리는 각도 (도) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float MoveAmplitudeDegrees = 25.f;

	/** 가만히 있을 때 초당 흔들림 횟수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float IdleFrequency = 1.2f;

	/** 최대 속도로 이동할 때 초당 흔들림 횟수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float MoveFrequency = 4.5f;

	/** 본 하나를 지날 때마다 늦어지는 위상 (라디안). 클수록 파도 물결이 촘촘해짐 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float PhasePerBoneRadians = 0.9f;

	/** 끝으로 갈수록 커지는 정도. 1이면 비례해서 커지고, 0이면 모든 본이 같은 각도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Wave", meta = (ClampMin = "0"))
	float AmplitudeExponent = 1.f;

	// ===== 속도 반응 =====
	/** 이 속도(cm/s) 이상이면 최대로 흔들림 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Speed", meta = (ClampMin = "1"))
	float FullWaveSpeed = 800.f;

	/** 이동 속도에 반응하는 부드러움. 클수록 빨리 반응 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tongue|Speed", meta = (ClampMin = "0.1"))
	float SpeedResponse = 6.f;

private:
	/** 본 목록과 기준 자세를 읽어 둡니다. 메시가 준비된 뒤에 한 번만 */
	bool InitializeChain();

	/** 붙은 본의 기준 자세만큼 위치/회전을 상쇄 */
	void CompensateAttachBone();

	/** 흔들 본 이름들 (루트 본은 제외, 부모가 먼저 오는 순서) */
	TArray<FName> ChainBones;

	/** 각 본의 기준 자세 회전 (컴포넌트 공간) */
	TArray<FQuat> RefComponentRotations;

	bool bChainInitialized = false;
	float Phase = 0.f;
	float SmoothedAlpha = 0.f;
};
