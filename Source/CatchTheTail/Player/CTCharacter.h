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

/**
 * 플레이어 캐릭터 (로봇).
 *
 * 기능
 *  - 이동 / 점프(공중 중복 점프 불가) / 달리기 / 자유 회전 3인칭 카메라 (마우스 회전, 각도에 따라 거리 보간, 벽에 닿으면 가까워짐)
 *  - 잡기(좌클릭, 쿨타임 없음) / 밀기(우클릭, 쿨타임 1초)
 *  - 속도 "버프 합산" 방식: 최종 배율 = 1.0 + 달리기 0.5 + 추격 0.2 ... (유령은 -0.5)
 *  - 유령 상태: 이동만 가능
 *  - 애니메이션: 이동/점프는 UCTAnimInstance가 상태를 읽어 재생,
 *               잡기/밀기/잡힘/밀려남은 몽타주로 재생 (MulticastPlayAction)
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
	virtual void Tick(float DeltaSeconds) override;

	/** 유령이면 점프 불가 */
	virtual bool CanJumpInternal_Implementation() const override;

	/** 클라이언트: PlayerState가 캐릭터보다 늦게 도착해도 유령 상태를 맞춰 줍니다. */
	virtual void OnRep_PlayerState() override;

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

	/**
	 * 달리기 게이지 (0~1, UI용)
	 * 대기: 1 / 달리는 중: 남은 달리기 시간 비율 / 쿨타임: 충전된 비율
	 */
	UFUNCTION(BlueprintPure, Category = "CT|Run")
	float GetRunGaugePercent() const;

	/** 밀기 준비 정도 (0~1, 1이면 사용 가능). 내 캐릭터에서만 정확합니다. */
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	float GetPushReadyPercent() const;

	/** 배율 1.0일 때의 걷기 속도 (애니메이션 속도 계산용) */
	UFUNCTION(BlueprintPure, Category = "CT|Speed")
	float GetBaseWalkSpeed() const { return BaseWalkSpeed; }

	/** 지금 잡기/밀기를 쓸 수 있는가? (게임 중 + 유령 아님) */
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	bool CanUseActions() const;

	/**
	 * 이동, 점프, 달리기를 할 수 있는가?
	 * 경기가 시작되고 내 색이 배정된 뒤에만 true입니다. (시작 전, 종료 후에는 움직일 수 없음)
	 */
	UFUNCTION(BlueprintPure, Category = "CT|State")
	bool CanMoveNow() const;

	/** 행동에 해당하는 몽타주 (없으면 nullptr) */
	UFUNCTION(BlueprintPure, Category = "CT|Animation")
	UAnimMontage* GetActionMontage(ECTCharacterAction Action) const;

	// ===== 서버 전용 함수 =====
	/** 속도 버프 추가 (같은 이름이면 값만 바꿈) */
	void AddSpeedBuff(FName BuffId, float Value);

	/** 속도 버프 제거 */
	void RemoveSpeedBuff(FName BuffId);

	/** "추격" 버프를 켜야 하는지 다시 계산 (게임 중 + 유령 아님 → 켬) */
	void RefreshChaseBuff();

	/** PlayerState가 "유령 상태가 바뀌었다"고 알려줄 때 호출 (서버, 클라이언트 모두) */
	void OnGhostStateChanged(bool bNewIsGhost);

	/** 내 색(AssignedColor)을 캐릭터 머티리얼의 BodyColorParameterName 파라미터에 반영합니다. (서버, 클라이언트 모두) */
	void RefreshBodyColor();

	/** 꼬리 수가 바뀌었을 때 꼬리 모양(몸통 개수)을 다시 맞춥니다. (PlayerState가 호출) */
	void RefreshTailVisual();

	/** AnimNotifyState(CT Grab Window)가 호출합니다. 구간이 시작/끝날 때 서버만 잡기 판정을 켜고 끕니다. */
	void OnGrabNotifyBegin();
	void OnGrabNotifyEnd();

	/** 지금 잡기 동작 중인가? (이 동안에만 상대 꼬리에 닿을 수 있음. 서버에서만 정확) */
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	bool IsGrabActive() const { return bGrabActive; }

	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	ACTTail* GetOwnedTail() const { return OwnedTail; }

	/**
	 * 모두의 화면에서 행동 연출(몽타주 + 블루프린트 이벤트)을 재생합니다. 서버에서 호출하세요.
	 * 내가 직접 한 잡기/밀기는 버튼을 누른 순간 이미 재생했으므로 몽타주를 다시 틀지 않습니다.
	 * @param bSuccess 잡기 성공 여부 (잡기 외에는 항상 true)
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAction(ECTCharacterAction Action, bool bSuccess);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ===== 블루프린트에서 연결할 이벤트 (이펙트, 사운드용) =====
	/** 잡기/밀기/잡힘/밀려남 연출. 모든 사람의 화면에서 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnAction(ECTCharacterAction Action, bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Events")
	void BP_OnGhostStateChanged(bool bNewIsGhost);

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// ===== 카메라 (마우스로 자유롭게 회전. 위에서 볼 땐 멀리, 뒤에서 볼 땐 가깝게) =====
	/** 뒤에서 볼 때(수평에 가까울 때)의 카메라 거리(cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "100"))
	float CameraDistanceBehind = 450.f;

	/** 위에서 내려다볼 때의 카메라 거리(cm). 넓게 보이도록 더 멀리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "100"))
	float CameraDistanceTop = 900.f;

	/** "뒤에서 보는" 기준 각도(Pitch). 이 각도 이상 위를 보면 가장 가깝습니다. (보통 -10) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "-89", ClampMax = "30"))
	float BehindPitch = -10.f;

	/** "위에서 보는" 기준 각도(Pitch). 이 각도 이상 내려다보면 가장 멉니다. (보통 -70) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "-89", ClampMax = "0"))
	float TopPitch = -70.f;

	/**
	 * 각도 → 거리 보간 곡선 (선택). 가로축 0~1(0=뒤에서 봄, 1=위에서 봄), 세로축 0~1(0=가까움, 1=멈).
	 * 비워 두면 부드러운 S자 곡선(SmoothStep)을 씁니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	TObjectPtr<UCurveFloat> DistanceCurve;

	/** 카메라 거리가 목표를 따라가는 속도. 클수록 빨리 따라감 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "0.1"))
	float CameraDistanceInterpSpeed = 8.f;

	/** 벽에 막힌 카메라가 플레이어 쪽으로 당겨질 때, 충돌 검사에 쓰는 공의 크기(cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "1"))
	float CameraProbeSize = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "30", ClampMax = "120"))
	float CameraFieldOfView = 70.f;

	// ===== 입력 (블루프린트에서 Input Action 에셋을 넣어 주세요) =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TObjectPtr<UInputAction> MoveAction;

	/** 카메라 회전(마우스 이동) 입력. Axis2D, 마우스 Y는 Negate 필요 */
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

	// ===== 애니메이션 몽타주 (블루프린트에서 넣어 주세요. 비워 두면 재생하지 않음) =====
	/** 잡기 (왼손 뻗기) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> GrabMontage;

	/** 밀기 (오른손 밀기) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> PushMontage;

	/** 꼬리를 잡혔을 때 반응 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> CaughtMontage;

	/** 밀려났을 때 반응 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> PushedMontage;

	/**
	 * 달리기 몽타주. Shift를 누르고 움직이는 동안 반복 재생합니다. (비워 두면 재생하지 않음)
	 * 몽타주에 반복할 섹션이 필요합니다. 섹션 이름을 아래 RunMontageLoopSection과 같게 하세요.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> RunMontage;

	/** 달리기 몽타주에서 반복할 섹션 이름. 끝나면 스스로 다시 이어집니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	FName RunMontageLoopSection = TEXT("Default");

	/** 달리기 몽타주가 원래 이동하던 속도(cm/s). 실제 속도가 빠를수록 이 값 대비 비율만큼 빨리 재생합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "1"))
	float RunMontageReferenceSpeed = 297.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float RunMontageMinPlayRate = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float RunMontageMaxPlayRate = 2.f;

	/** 이 속도(cm/s)보다 느리면 Shift를 눌러도 달리기 동작을 재생하지 않습니다. (제자리에서 달리기 방지) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0"))
	float RunMontageMinSpeed = 50.f;

	/** 행동 몽타주 재생 속도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float ActionMontagePlayRate = 1.f;

	// ===== 혀 (Head 본에 붙어 이동할 때 물결치듯 흔들림) =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Tongue")
	TObjectPtr<UCTTongueComponent> TongueMesh;

	// ===== 꼬리 =====
	/** 캐릭터가 가질 꼬리 액터 (BP_CTTail 등을 지정. 비워 두면 꼬리를 만들지 않음) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	TSubclassOf<ACTTail> TailClass;

	/** 꼬리를 붙일 메시의 본(또는 소켓) 이름. 소켓을 따로 만들 필요 없이 본에 바로 붙습니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	FName TailSocketName = TEXT("Hips");

	/**
	 * 꼬리 시작(받침) 위치 (캐릭터 모델 좌표 기준, cm). 기본 자세에서 꼬리가 놓일 곳입니다.
	 * 모델은 +Y가 앞이고 +Z가 위입니다. 기본값은 엉덩이 뒤쪽 바닥 근처입니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	FVector TailOffsetLocation = FVector(0.f, -60.f, 29.f);

	/** 꼬리 회전 (캐릭터 모델 좌표 기준). 꼬리는 +Z 방향으로 이어지므로 Roll 90이면 바닥에 눕혀 뒤로 길게 끌립니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	FRotator TailOffsetRotation = FRotator(0.f, 0.f, 90.f);

	/** 꼬리 전체 크기. 몸통이 1개 97cm라 7개면 길어지므로, 캐릭터에 맞게 줄이세요. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Tail")
	FVector TailOffsetScale = FVector(0.5f, 0.5f, 0.5f);

	// ===== 몸 색상 (M_Player의 흰색 영역을 마스크로 칠함) =====
	/** 머티리얼에 만든 Vector Parameter 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Color")
	FName BodyColorParameterName = TEXT("BodyColor");

	/** 색이 아직 없을 때(None) 쓸 색. 흰색이면 원본 텍스처 그대로 보입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Color")
	FLinearColor NoColorTint = FLinearColor::White;

	/** 꼬리를 잡혀서 유령이 되었을 때 몸 색 (회색) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Color")
	FLinearColor GhostBodyColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.f);

	/** 색을 칠할 메시 머티리얼 슬롯 번호. 비워 두면 모든 슬롯 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Color")
	TArray<int32> BodyColorMaterialSlots;

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

	/** true: 잡기 몽타주의 "CT Grab Window" 구간 동안만 닿으면 잡힘. 몽타주에 그 노티파이가 없으면 아래 타이머로 대신합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat")
	bool bUseMontageGrabWindow = true;

	/** 몽타주 구간이 끝나지 않을 때를 대비한 최대 판정 시간(초). 안전장치입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0.1"))
	float GrabWindowMaxSeconds = 1.5f;

	/** true: 상대 꼬리(ACTTail)에 닿아야 잡힘. false: 예전 방식(정면 부채꼴 안의 상대를 바로 잡음) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat")
	bool bRequireTailTouch = true;

	/** 잡기 버튼을 누른 뒤 손이 닿기 시작할 때까지의 시간(초). 잡기 애니메이션의 손 뻗는 타이밍에 맞추세요. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float GrabWindowDelaySeconds = 0.1f;

	/** 꼬리에 닿을 수 있는 시간(초). 이 동안에만 잡을 수 있습니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0.05"))
	float GrabActiveSeconds = 0.35f;

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
	/** 카메라 충돌, 시야각 등 처음 설정 적용 */
	void ApplyCameraSettings();

	/** 위/아래 보는 각도(Pitch)에 따라 카메라 거리를 보간 (내 화면에서만) */
	void UpdateCameraDistance(float DeltaSeconds);

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

	// ===== 애니메이션 =====
	/** 행동 몽타주를 이 컴퓨터에서만 재생 */
	void PlayActionMontage(ECTCharacterAction Action);

	/** 잡기/밀기 몽타주 중지 (유령이 될 때) */
	void StopAttackMontages();

	/** 달리기 중이면 달리기 몽타주를 반복 재생하고, 아니면 멈춥니다. (모든 컴퓨터, 매 프레임) */
	void UpdateRunMontage();

	// ===== 꼬리 / 잡기 동작 구간 (서버) =====
	void SpawnTail();
	/** 타이머 방식의 잡기 구간 시작 (GrabActiveSeconds 동안) */
	void BeginGrabWindow();

	/** 잡기 구간을 Duration초 동안 엽니다. 몽타주 방식은 노티파이가 끝을 알려 주므로 최대 시간만 둡니다. */
	void StartGrabWindow(float Duration);

	/** GrabMontage에 CT Grab Window 노티파이가 있는가? */
	bool HasGrabWindowNotify() const;
	void EndGrabWindow();
	void TickGrabWindow();

	/** 예전 방식: 정면 부채꼴 안의 목표 색 캐릭터를 바로 잡음 (bRequireTailTouch가 꺼져 있을 때) */
	bool TryGrabByCone();

	UFUNCTION()
	void OnRep_OwnedTail();

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

	/** 마지막으로 밀기 버튼을 누른 시각 (내 컴퓨터. 쿨타임 중에는 몽타주를 틀지 않기 위해) */
	double LocalLastPushTime = -1000.0;

	/** 이 캐릭터의 꼬리 (서버가 만들고 복제) */
	UPROPERTY(ReplicatedUsing = OnRep_OwnedTail)
	TObjectPtr<ACTTail> OwnedTail;

	/** 잡기 동작 구간 중인가 (서버) */
	bool bGrabActive = false;
	double GrabWindowEndTime = 0.0;
	/** 이 시각까지는 잡기 입력을 다시 받지 않음 (서버) */
	double GrabLockEndTime = 0.0;
	FTimerHandle GrabWindowStartTimerHandle;
	FTimerHandle GrabWindowTickTimerHandle;

	/** 몸 색 변경용 동적 머티리얼 (슬롯별) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BodyMaterials;
};
