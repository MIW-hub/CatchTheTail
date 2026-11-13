// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CTCharacterMovementComponent.generated.h"

/**
 * 걷기/달리기가 처음엔 느리게 출발하고, 계속 가면 점점 빨라지는 느낌을 내는 이동 컴포넌트.
 * 현재 속도가 낮을수록 가속을 약하게 주어서, 속도가 붙을수록 가속이 커짐 (서버와 클라이언트가 같은 값으로 계산하므로 어긋나지 않음).
 *
 * 예전 느낌(바로 최대 가속)으로 되돌리려면:
 *  - 캐릭터 BP의 CharacterMovement 컴포넌트에서 bUseAccelerationRamp 를 끄거나,
 *  - CTCharacter.cpp 생성자의 SetDefaultSubobjectClass 줄을 주석 처리하고 원래 ACTCharacter() 로 되돌리면 됨.
 */
UCLASS()
class CATCHTHETAIL_API UCTCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	virtual float GetMaxAcceleration() const override;

	// 떨어질 때만 중력을 더 세게 (점프 높이는 그대로, 체공시간만 줄임). 1이면 올라갈 때와 같음
	virtual float GetGravityZ() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Movement", meta = (ClampMin = "1"))
	float FallGravityMultiplier = 2.f;

	// 끄면 기본 이동 (가속 곡선 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Movement")
	bool bUseAccelerationRamp = true;

	// 멈춰 있을 때 가속 비율 (0~1). 작을수록 출발이 느리고 둔함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Movement", meta = (ClampMin = "0.02", ClampMax = "1"))
	float StartAccelerationFactor = 0.55f;

	// 속도가 붙을수록 가속이 커지는 정도. 1이면 일정하게 증가, 클수록 처음에 더 오래 느림
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Movement", meta = (ClampMin = "1"))
	float AccelerationRampExponent = 1.2f;
};
