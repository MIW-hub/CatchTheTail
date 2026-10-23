// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CTAnimInstance.generated.h"

class ACTCharacter;
class UCharacterMovementComponent;

UCLASS()
class CATCHTHETAIL_API UCTAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// 참조
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim")
	TObjectPtr<ACTCharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	// 설정
	// 이동 판정 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Anim|Settings", meta = (ClampMin = "0"))
	float MoveThreshold = 3.f;

	// 재생 속도 1.0 기준 이동 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Anim|Settings", meta = (ClampMin = "1"))
	float AnimReferenceSpeed = 600.f;

	// 재생 속도 하한/상한
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Anim|Settings", meta = (ClampMin = "0"))
	float MinMovePlayRate = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Anim|Settings", meta = (ClampMin = "0"))
	float MaxMovePlayRate = 2.f;

	// 이동
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	FVector Velocity = FVector::ZeroVector;

	// 수평 속도
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	float GroundSpeed = 0.f;

	// 걷기 속도 대비 비율
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	float NormalizedSpeed = 0.f;

	// 이동 애님 재생 속도
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	float MovePlayRate = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	bool bShouldMove = false;

	// 점프 / 공중
	// 공중 (점프+낙하)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	bool bIsFalling = false;

	// 상승 중 -> JumpStart
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	bool bIsJumping = false;

	// 하강 중 -> FallLoop
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	bool bIsFallingDown = false;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	float VerticalVelocity = 0.f;

	// 공중 체류 시간 (초)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	float TimeInAir = 0.f;

	// 캐릭터 상태
	// 달리기 중
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|State")
	bool bIsRunning = false;

	// 유령 상태
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|State")
	bool bIsGhost = false;

	// 액션 몽타주 재생 중
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|State")
	bool bIsPlayingAction = false;
};
