// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Type/CTTypes.h"
#include "CTCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

/**
 * 플레이어 캐릭터 (로봇).
 *
 * 기능
 *  - 이동 / 점프(공중 중복 점프 불가) / 달리기 / 3인칭 추적 카메라
 *  - 잡기(좌클릭, 쿨타임 없음) / 밀기(우클릭, 쿨타임 1초)
 *  - 속도 "버프 합산" 방식: 최종 배율 = 1.0 + 달리기 0.5 + 추격 0.2 ... (유령은 -0.5)
 *  - 유령 상태: 이동만 가능
 *
 * 규칙 판정(잡기 성공, 밀기, 달리기 시작)은 모두 서버가 합니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACTCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 유령이면 점프 불가 */
	virtual bool CanJumpInternal_Implementation() const override;

	// ===== 상태 읽기 =====
	UFUNCTION(BlueprintPure, Category = "CT|State")
	bool IsGhost() const;

	/** 현재 속도 배율 (1.0 + 모든 버프의 합) */
	UFUNCTION(BlueprintPure, Category = "CT|Speed")
	float GetSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "CT|Run")
	ECTRunState GetRunState() const { return RunState; }

	/** 달리기 중이면 남은 달리기 시간, 쿨타임 중이면 남은 쿨타임 (초) */
	UFUNCTION(BlueprintPure, Category = "CT|Run")
	float GetRunStateRemainingSeconds() const;

	// ===== 서버 전용 함수 =====
	/** 속도 버프 추가 (같은 이름이면 값만 바꿈) */
	void AddSpeedBuff(FName BuffId, float Value);

	/** 속도 버프 제거 */
	void RemoveSpeedBuff(FName BuffId);

	/** "추격" 버프를 켜야 하는지 다시 계산 (게임 중 + 유령 아님 → 켬) */
	void RefreshChaseBuff();

	/** PlayerState가 "유령 상태가 바뀌었다"고 알려줄 때 호출 (서버, 클라이언트 모두) */
	void OnGhostStateChanged(bool bNewIsGhost);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	// ===== 블루프린트에서 연결할 이벤트 (애니메이션, 이펙트용) =====
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnGrab();

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnPush();

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnGhostStateChanged(bool bNewIsGhost);

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// ===== 입력 (블루프린트에서 Input Action 에셋을 넣어 주세요) =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TObjectPtr<UInputAction> GrabAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TObjectPtr<UInputAction> PushAction;

	// ===== 속도 설정 (임의값. 엔진에서 확인하며 조정하세요) =====
	/** 기본 걷기 속도 (배율 1.0일 때) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Speed", meta = (ClampMin = "0"))
	float BaseWalkSpeed = 600.f;

	/** 달리기 버프 값 (+0.5 → 1.5배) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Speed")
	float RunBuffValue = 0.5f;

	/** 목표 추격 버프 값 (+0.2 → 1.2배) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Speed")
	float ChaseBuffValue = 0.2f;

	/** 유령 버프 값 (-0.5 → 0.5배) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Speed")
	float GhostBuffValue = -0.5f;

	// ===== 달리기 설정 =====
	/** Shift를 누르고 있을 때 최대로 달릴 수 있는 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Run", meta = (ClampMin = "0.1"))
	float RunDurationSeconds = 3.f;

	/** 달리기가 끝난 뒤 다시 달릴 수 있을 때까지 쿨타임(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Run", meta = (ClampMin = "0"))
	float RunCooldownSeconds = 5.f;

	/** 3초 전에 Shift를 떼도 쿨타임을 적용할지 (true면 적용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Run")
	bool bCooldownOnEarlyRelease = true;

	// ===== 잡기 / 밀기 설정 (임의값) =====
	/** 잡기 거리 (cm, 내 중심 ~ 상대 중심) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float GrabDistance = 150.f;

	/** 잡기 범위 각도 (정면 기준 좌우 몇 도까지) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0", ClampMax = "180"))
	float GrabHalfAngleDegrees = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushDistance = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0", ClampMax = "180"))
	float PushHalfAngleDegrees = 60.f;

	/** 밀려나는 속도 (수평) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushSpeed = 800.f;

	/** 밀 때 살짝 위로 띄우는 속도 (0이면 바닥을 따라 밀림) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushUpSpeed = 200.f;

	/** 밀기 쿨타임(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushCooldownSeconds = 1.f;

protected:
	// ===== 입력 처리 (내 컴퓨터에서 실행) =====
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnRunPressed();
	void OnRunReleased();
	void OnGrabPressed();
	void OnPushPressed();

	// ===== 서버에게 부탁하는 함수들 (RPC) =====
	UFUNCTION(Server, Reliable)
	void ServerSetRunInput(bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerGrab();

	UFUNCTION(Server, Reliable)
	void ServerPush();

	/** 모두의 화면에서 잡기/밀기 연출을 재생 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayGrab();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayPush();

	// ===== 속도 =====
	void RecalculateMaxWalkSpeed();

	UFUNCTION()
	void OnRep_SpeedBuffs();

	// ===== 달리기 (서버) =====
	void TryStartRun();
	void StopRun(bool bEndedNaturally);
	void ForceResetRun();
	void OnRunDurationEnded();
	void OnRunCooldownEnded();

	// ===== 도우미 =====
	/** 서버 기준 현재 시각 */
	double GetServerTime() const;

	/** Other가 내 정면 부채꼴(거리, 각도) 안에 있는가? (수평 기준) */
	bool IsInFrontCone(const AActor* Other, float MaxDistance, float HalfAngleDegrees) const;

private:
	/** 지금 붙어 있는 속도 버프들 (서버가 정하고 복제) */
	UPROPERTY(ReplicatedUsing = OnRep_SpeedBuffs)
	TArray<FCTSpeedBuff> SpeedBuffs;

	UPROPERTY(Replicated)
	ECTRunState RunState = ECTRunState::Idle;

	/** 달리기/쿨타임이 끝나는 서버 시각 */
	UPROPERTY(Replicated)
	double RunStateEndTime = 0.0;

	FTimerHandle RunTimerHandle;

	/** 마지막으로 민 시각 (서버) */
	double LastPushTime = -1000.0;
};
