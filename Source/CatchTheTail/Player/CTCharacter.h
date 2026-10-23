// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Type/CTTypes.h"
#include "CTCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UCurveFloat;
class UAnimMontage;
class UMaterialInstanceDynamic;
class ACTTail;
class UCTTongueComponent;
struct FInputActionValue;

UCLASS()
class CATCHTHETAIL_API ACTCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACTCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	// 유령 점프 불가
	virtual bool CanJumpInternal_Implementation() const override;

	// 늦게 온 PlayerState 유령 상태 동기화
	virtual void OnRep_PlayerState() override;

	// 상태 읽기
	UFUNCTION(BlueprintPure, Category = "CT|State")
	bool IsGhost() const;

	// 속도 배율 (1 + 버프 합)
	UFUNCTION(BlueprintPure, Category = "CT|Speed")
	float GetSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "CT|Run")
	ECTRunState GetRunState() const { return RunState; }

	// 달리기/쿨타임 남은 시간
	UFUNCTION(BlueprintPure, Category = "CT|Run")
	float GetRunStateRemainingSeconds() const;

	// 달리기 게이지 0~1
	UFUNCTION(BlueprintPure, Category = "CT|Run")
	float GetRunGaugePercent() const;

	// 밀기 준비도 0~1 (로컬만 정확)
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	float GetPushReadyPercent() const;

	// 배율 1.0 걷기 속도
	UFUNCTION(BlueprintPure, Category = "CT|Speed")
	float GetBaseWalkSpeed() const { return BaseWalkSpeed; }

	// 잡기/밀기 가능 여부
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	bool CanUseActions() const;

	// 이동/점프/달리기 가능 여부
	UFUNCTION(BlueprintPure, Category = "CT|State")
	bool CanMoveNow() const;

	// 행동별 몽타주
	UFUNCTION(BlueprintPure, Category = "CT|Animation")
	UAnimMontage* GetActionMontage(ECTCharacterAction Action) const;

	// 서버 전용 함수
	// 같은 이름이면 값 갱신
	void AddSpeedBuff(FName BuffId, float Value);

	void RemoveSpeedBuff(FName BuffId);

	// 추격 버프 재계산
	void RefreshChaseBuff();

	// 유령 상태 변경 통지
	void OnGhostStateChanged(bool bNewIsGhost);

	// AssignedColor 머티리얼 반영
	void RefreshBodyColor();

	// 꼬리 수 변경 시 모양 갱신
	void RefreshTailVisual();

	// 꼬리 본 상대 변환
	void ApplyTailOffset();

	// GrabWindow 노티파이용, 서버 판정 on/off
	void OnGrabNotifyBegin();
	void OnGrabNotifyEnd();

	// 잡기 동작 중 (서버만 정확)
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	bool IsGrabActive() const { return bGrabActive; }

	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	ACTTail* GetOwnedTail() const { return OwnedTail; }

	// 전원 화면 행동 연출 (서버 호출)
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAction(ECTCharacterAction Action, bool bSuccess);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 블루프린트에서 연결할 이벤트
	// 행동 연출 (전원 화면)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnAction(ECTCharacterAction Action, bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnGhostStateChanged(bool bNewIsGhost);

	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// 카메라
	// 뒤 시점 거리(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "100"))
	float CameraDistanceBehind = 450.f;

	// 위 시점 거리(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "100"))
	float CameraDistanceTop = 900.f;

	// 뒤 시점 기준 Pitch
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "-89", ClampMax = "30"))
	float BehindPitch = -10.f;

	// 위 시점 기준 Pitch
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "-89", ClampMax = "0"))
	float TopPitch = -70.f;

	// 각도→거리 곡선, 없으면 SmoothStep
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	TObjectPtr<UCurveFloat> DistanceCurve;

	// 거리 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "0.1"))
	float CameraDistanceInterpSpeed = 8.f;

	/**
	 * 카메라가 캐릭터에 이 거리(cm)보다 가까워지면 내 화면에서 내 캐릭터(몸, 혀, 꼬리)를 숨깁니다.
	 * 벽에 바짝 붙어서 스프링암이 짧아졌을 때 카메라가 몸 안으로 들어와 화면이 가려지는 것을 막습니다.
	 * 다른 사람의 화면에는 영향이 없습니다. 0이면 사용하지 않음
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "0"))
	float CameraHideDistance = 130.f;

	/** 숨긴 뒤 다시 보이게 하는 거리 = 숨기는 거리 + 이 값(cm). 경계에서 깜빡이지 않게 해 줍니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "0"))
	float CameraHideHysteresis = 30.f;

	// 카메라 충돌 구 크기(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "1"))
	float CameraProbeSize = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "30", ClampMax = "120"))
	float CameraFieldOfView = 70.f;

	// 입력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TObjectPtr<UInputAction> MoveAction;

	// 카메라 회전, 마우스 Y Negate 필요
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

	// 애니메이션 몽타주
	// 잡기 (왼손)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> GrabMontage;

	// 밀기 (오른손)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> PushMontage;

	// 꼬리 잡힘 반응
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> CaughtMontage;

	// 밀려남 반응
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> PushedMontage;

	// 달리기 반복 몽타주, 섹션명=RunMontageLoopSection
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> RunMontage;

	// 반복 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	FName RunMontageLoopSection = TEXT("Default");

	// 몽타주 기준 속도(cm/s), 실속도 비례 재생
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "1"))
	float RunMontageReferenceSpeed = 297.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float RunMontageMinPlayRate = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float RunMontageMaxPlayRate = 2.f;

	// 이 속도 미만이면 달리기 모션 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0"))
	float RunMontageMinSpeed = 50.f;

	// 행동 몽타주 재생 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float ActionMontagePlayRate = 1.f;

	// 혀
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tongue")
	TObjectPtr<UCTTongueComponent> TongueMesh;

	// 꼬리
	// 꼬리 액터 클래스, 비면 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	TSubclassOf<ACTTail> TailClass;

	// 꼬리 붙일 본
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	FName TailSocketName = TEXT("Hips");

	// 꼬리 시작 위치 (모델 좌표, +Y 앞 +Z 위)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	FVector TailOffsetLocation = FVector(0.f, -60.f, 29.f);

	// 꼬리 회전, +Z로 이어지므로 Roll 90이면 눕힘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	FRotator TailOffsetRotation = FRotator(0.f, 0.f, 90.f);

	// 꼬리 전체 크기 (몸통 97cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	FVector TailOffsetScale = FVector(0.5f, 0.5f, 0.5f);

	// 몸 색상
	// 머티리얼 Vector 파라미터명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Color")
	FName BodyColorParameterName = TEXT("BodyColor");

	// 색 없을 때 색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Color")
	FLinearColor NoColorTint = FLinearColor::White;

	// 유령 몸 색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Color")
	FLinearColor GhostBodyColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.f);

	// 색칠할 슬롯, 비면 전체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Color")
	TArray<int32> BodyColorMaterialSlots;

	// 속도 설정
	// 기본 걷기 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Speed", meta = (ClampMin = "0"))
	float BaseWalkSpeed = 600.f;

	// 달리기 버프 (+0.5)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Speed")
	float RunBuffValue = 0.5f;

	// 추격 버프 (+0.2)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Speed")
	float ChaseBuffValue = 0.2f;

	// 유령 버프 (-0.5)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Speed")
	float GhostBuffValue = -0.5f;

	// 달리기 설정
	// 최대 달리기 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Run", meta = (ClampMin = "0.1"))
	float RunDurationSeconds = 3.f;

	// 달리기 쿨타임(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Run", meta = (ClampMin = "0"))
	float RunCooldownSeconds = 5.f;

	// 일찍 뗐을 때도 쿨타임 적용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Run")
	bool bCooldownOnEarlyRelease = true;

	// 잡기 / 밀기 설정
	// 잡기 거리(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float GrabDistance = 150.f;

	// true: Grab Window 구간만 판정, 노티파이 없으면 타이머
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat")
	bool bUseMontageGrabWindow = true;

	// 판정 최대 시간(초), 안전장치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0.1"))
	float GrabWindowMaxSeconds = 1.5f;

	// true: 꼬리 접촉 필요, false: 정면 부채꼴
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat")
	bool bRequireTailTouch = true;

	// 잡기 입력 후 판정 시작까지(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float GrabWindowDelaySeconds = 0.1f;

	// 판정 지속(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0.05"))
	float GrabActiveSeconds = 0.35f;

	// 부채꼴 반각
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0", ClampMax = "180"))
	float GrabHalfAngleDegrees = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushDistance = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0", ClampMax = "180"))
	float PushHalfAngleDegrees = 60.f;

	// 밀려나는 수평 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushSpeed = 800.f;

	// 밀 때 상승 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushUpSpeed = 200.f;

	// 밀기 쿨타임(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushCooldownSeconds = 1.f;

protected:
	// 카메라 초기 설정
	void ApplyCameraSettings();

	// Pitch별 카메라 거리 보간 (로컬만)
	void UpdateCameraDistance(float DeltaSeconds);

	/** 카메라가 너무 가까우면 내 캐릭터를 내 화면에서만 숨기고, 멀어지면 다시 보이게 합니다. */
	void UpdateCameraClip();
	void SetHiddenFromOwnCamera(bool bNewHidden);

	// 입력 처리
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnRunPressed();
	void OnRunReleased();
	void OnGrabPressed();
	void OnPushPressed();

	// 서버에게 부탁하는 함수들 (RPC)
	UFUNCTION(Server, Reliable)
	void ServerSetRunInput(bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerGrab();

	UFUNCTION(Server, Reliable)
	void ServerPush();

	// 애니메이션
	// 로컬에서만 몽타주 재생
	void PlayActionMontage(ECTCharacterAction Action);

	// 잡기/밀기 몽타주 중지
	void StopAttackMontages();

	// 달리기 몽타주 반복/정지 (매 프레임)
	void UpdateRunMontage();

	// 꼬리 / 잡기 동작 구간 (서버)
	void SpawnTail();
	// 타이머 방식 잡기 구간
	void BeginGrabWindow();

	// 잡기 구간 Duration초 개방
	void StartGrabWindow(float Duration);

	// GrabMontage에 노티파이 있는지
	bool HasGrabWindowNotify() const;
	void EndGrabWindow();
	void TickGrabWindow();

	// 구방식: 정면 부채꼴 즉시 잡기
	bool TryGrabByCone();

	UFUNCTION()
	void OnRep_OwnedTail();

	// 속도
	void RecalculateMaxWalkSpeed();

	UFUNCTION()
	void OnRep_SpeedBuffs();

	// 달리기 (서버)
	void TryStartRun();
	void StopRun(bool bEndedNaturally);
	void ForceResetRun();
	void OnRunDurationEnded();
	void OnRunCooldownEnded();

	// 도우미
	double GetServerTime() const;

	// Other가 정면 부채꼴 안인지
	bool IsInFrontCone(const AActor* Other, float MaxDistance, float HalfAngleDegrees) const;

private:
	// 속도 버프 (서버 결정, 복제)
	UPROPERTY(ReplicatedUsing = OnRep_SpeedBuffs)
	TArray<FCTSpeedBuff> SpeedBuffs;

	UPROPERTY(Replicated)
	ECTRunState RunState = ECTRunState::Idle;

	// 달리기/쿨타임 종료 서버 시각
	UPROPERTY(Replicated)
	double RunStateEndTime = 0.0;

	FTimerHandle RunTimerHandle;

	// 지금 카메라가 가까워서 내 캐릭터를 숨긴 상태인가 (내 컴퓨터)
	bool bHiddenFromOwnCamera = false;

	// 마지막 밀기 시각 (서버)
	double LastPushTime = -1000.0;

	// 마지막 밀기 입력 시각 (로컬)
	double LocalLastPushTime = -1000.0;

	// 꼬리 (서버 생성, 복제)
	UPROPERTY(ReplicatedUsing = OnRep_OwnedTail)
	TObjectPtr<ACTTail> OwnedTail;

	// 잡기 구간 중 (서버)
	bool bGrabActive = false;
	double GrabWindowEndTime = 0.0;
	// 이 시각까지 잡기 입력 무시 (서버)
	double GrabLockEndTime = 0.0;
	FTimerHandle GrabWindowStartTimerHandle;
	FTimerHandle GrabWindowTickTimerHandle;

	// 슬롯별 몸 색 동적 머티리얼
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BodyMaterials;
};
