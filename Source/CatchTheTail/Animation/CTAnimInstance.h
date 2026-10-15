// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CTAnimInstance.generated.h"

class ACTCharacter;
class UCharacterMovementComponent;

/**
 * 로봇 캐릭터용 애니메이션 인스턴스. (ABP_CTCharacter의 부모 클래스로 지정하세요)
 *
 * 하는 일: 매 프레임 캐릭터 상태를 읽어서 AnimGraph가 쓸 변수로 정리합니다.
 *  - 이동: GroundSpeed, bShouldMove, MovePlayRate → Idle / Walk / Run 블렌드스페이스
 *  - 점프: bIsFalling, bIsJumping, bIsFallingDown, TimeInAir → JumpStart / Loop / Land 스테이트
 *  - 상태: bIsRunning, bIsGhost → 달리기 자세, 유령(전자파) 자세
 *
 * 잡기 / 밀기 / 잡힘 / 밀려남은 캐릭터가 몽타주로 재생합니다.
 * AnimGraph에 "DefaultSlot"(또는 상체만 쓰려면 "UpperBody") 슬롯 노드를 꼭 넣어 주세요.
 *
 * 모든 값은 복제된 정보(이동 속도, 이동 모드, 버프, 유령 여부)로 계산하므로
 * 내 캐릭터와 다른 사람 캐릭터가 같은 방식으로 움직입니다.
 */
UCLASS()
class CATCHTHETAIL_API UCTAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// ===== 참조 =====
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim")
	TObjectPtr<ACTCharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	// ===== 설정 =====
	/** 이 속도보다 빠르면 "움직이는 중"으로 봅니다. (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Anim|Settings", meta = (ClampMin = "0"))
	float MoveThreshold = 3.f;

	/** 이동 애니메이션이 원래 속도(재생 속도 1.0)로 보이는 이동 속도 (cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Anim|Settings", meta = (ClampMin = "1"))
	float AnimReferenceSpeed = 600.f;

	/** 재생 속도 제한 (너무 느리거나 빠르게 보이지 않도록) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Anim|Settings", meta = (ClampMin = "0"))
	float MinMovePlayRate = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Anim|Settings", meta = (ClampMin = "0"))
	float MaxMovePlayRate = 2.f;

	// ===== 이동 =====
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	FVector Velocity = FVector::ZeroVector;

	/** 수평 이동 속도 (cm/s) */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	float GroundSpeed = 0.f;

	/** 기본 걷기 속도 대비 비율 (1.0 = 걷기, 1.5 = 달리기, 0.5 = 유령) */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	float NormalizedSpeed = 0.f;

	/** 이동 애니메이션 재생 속도 (버프로 빨라지면 발도 빨라지게) */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	float MovePlayRate = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Move")
	bool bShouldMove = false;

	// ===== 점프 / 공중 =====
	/** 공중에 있음 (점프, 낙하 모두) */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	bool bIsFalling = false;

	/** 공중 + 올라가는 중 → JumpStart */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	bool bIsJumping = false;

	/** 공중 + 내려가는 중 → FallLoop */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	bool bIsFallingDown = false;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	float VerticalVelocity = 0.f;

	/** 공중에 머문 시간(초). 아주 짧은 낙하에는 착지 동작을 생략할 때 사용 */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|Jump")
	float TimeInAir = 0.f;

	// ===== 캐릭터 상태 =====
	/** Shift 달리기 중 */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|State")
	bool bIsRunning = false;

	/** 유령(전자파) 상태 */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|State")
	bool bIsGhost = false;

	/** 잡기/밀기 등 행동 몽타주 재생 중 */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Anim|State")
	bool bIsPlayingAction = false;
};
