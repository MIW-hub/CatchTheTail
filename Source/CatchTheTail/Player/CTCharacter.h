// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Type/CTTypes.h"
#include "../Core/CTAudioSettings.h"
#include "CTCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UCurveFloat;
class UAnimMontage;
class UAnimSequence;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class ACTTail;
class ACTBush;
class UCTTongueComponent;
class UCTEmoteBubbleComponent;
class UCTGrappleComponent;
class UTexture2D;
struct FInputActionValue;

// 이모트 한 칸 (숫자 키 1, 2, 3...에 순서대로 대응)
USTRUCT(BlueprintType)
struct FCTEmoteSlot
{
	GENERATED_BODY()

	// 말풍선 안에 채울 그림 (이모지). 아틀라스를 쓰면 비워 둠
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote")
	TObjectPtr<UTexture2D> Icon;

	// 아틀라스(여러 이모지가 모인 한 장의 그림). 있으면 Icon 대신 이 그림의 AtlasCell 칸만 잘라 씀
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote")
	TObjectPtr<UTexture2D> Atlas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "1"))
	int32 AtlasColumns = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "1"))
	int32 AtlasRows = 3;

	// 칸 번호. 왼쪽 위가 0, 오른쪽으로 갔다가 다음 줄 (3x3이면 0~8)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote")
	int32 AtlasCell = 0;

	// 캐릭터가 재생할 동작. 비면 말풍선만 뜸
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote")
	TObjectPtr<UAnimMontage> Montage;

	// 몽타주 대신 애니메이션을 바로 넣을 수 있음 (몽타주가 비어 있을 때만 사용, 코드가 몽타주로 만들어 재생)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote")
	TObjectPtr<UAnimSequence> Sequence;

	// 설정 화면 등에 쓸 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote")
	FText Label;

	// 아틀라스에서 이 칸의 UV 범위. 아틀라스가 없으면 전체(0~1)
	void GetUVRegion(FVector2D& OutMin, FVector2D& OutMax) const
	{
		OutMin = FVector2D(0.0, 0.0);
		OutMax = FVector2D(1.0, 1.0);
		if (!Atlas) { return; }
		const int32 Columns = FMath::Max(1, AtlasColumns);
		const int32 Rows = FMath::Max(1, AtlasRows);
		const FVector2D CellSize(1.0 / Columns, 1.0 / Rows);
		OutMin = FVector2D((AtlasCell % Columns) * CellSize.X, FMath::Clamp(AtlasCell / Columns, 0, Rows - 1) * CellSize.Y);
		OutMax = OutMin + CellSize;
	}

	UTexture2D* GetTexture() const { return Atlas ? Atlas.Get() : Icon.Get(); }
};

UCLASS()
class CATCHTHETAIL_API ACTCharacter : public ACharacter
{
	GENERATED_BODY()

	// 잡기 대결이 달리기 해제, 이동 잠금 등에 접근
	friend class UCTGrappleComponent;

public:
	ACTCharacter(const FObjectInitializer& ObjectInitializer);

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

	// 밀기 쿨타임 남은 시간(초) (로컬만 정확)
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	float GetPushCooldownRemainingSeconds() const;

	// 잡기 준비도 0~1 (로컬만 정확)
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	float GetGrabReadyPercent() const;

	// 잡기 쿨타임 남은 시간(초) (로컬만 정확)
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	float GetGrabCooldownRemainingSeconds() const;

	// 쿨타임 설정값 (HUD 표시용)
	float GetRunCooldownSeconds() const { return RunCooldownSeconds; }
	float GetGrabCooldownSeconds() const { return GrabCooldownSeconds; }
	float GetPushCooldownSeconds() const { return PushCooldownSeconds; }

	// 배율 1.0 걷기 속도
	UFUNCTION(BlueprintPure, Category = "CT|Speed")
	float GetBaseWalkSpeed() const { return BaseWalkSpeed; }

	// 잡기/밀기 가능 여부
	UFUNCTION(BlueprintPure, Category = "CT|Combat")
	bool CanUseActions() const;

	// 이동/점프/달리기 가능 여부
	UFUNCTION(BlueprintPure, Category = "CT|State")
	bool CanMoveNow() const;

	// 부쉬 안 여부
	UFUNCTION(BlueprintPure, Category = "CT|Gimmick")
	bool IsInBush() const { return CurrentBush != nullptr; }

	UFUNCTION(BlueprintPure, Category = "CT|Gimmick")
	ACTBush* GetCurrentBush() const { return CurrentBush; }

	// 전화부스 사용 중
	UFUNCTION(BlueprintPure, Category = "CT|Gimmick")
	bool IsBoothLocked() const { return bBoothLocked; }

	// 서버 전용: 부쉬 출입, 부스 사용 중 잠금
	void SetCurrentBush(ACTBush* NewBush);
	void SetBoothLocked(bool bNewLocked);

	// 조종자 화면 안내 문구 (서버 호출)
	UFUNCTION(Client, Reliable)
	void ClientShowNotice(const FText& Message, float Seconds);

	// 전화 결과 (서버 호출): 안내 문구와, 원하면 TargetLocation을 가리키는 화살표
	UFUNCTION(Client, Reliable)
	void ClientShowPhoneResult(const FText& Message, float Seconds, bool bShowArrow, FVector TargetLocation);

	// 행동별 몽타주
	UFUNCTION(BlueprintPure, Category = "CT|Animation")
	UAnimMontage* GetActionMontage(ECTCharacterAction Action) const;

	// 팔 늘리기 (만화식 당기기). AnimBP 의 Transform (Modify) Bone 에 연결해서 사용
	// 팔(Arm, ForeArm)은 GetArmStretchScale3D, 손(Hand)은 GetArmStretchHandScale3D 를 Scale 에 넣음
	UFUNCTION(BlueprintCallable, Category = "CT|Animation")
	void SetArmStretchTarget(float NewScale) { ArmStretchTarget = FMath::Max(NewScale, 0.1f); }

	// 현재 늘어난 배율 (1 = 기본)
	UFUNCTION(BlueprintPure, Category = "CT|Animation")
	float GetArmStretch() const { return ArmStretch; }

	// 잡기 중 팔이 늘어나는 최대 배율
	float GetArmStretchGrabScale() const { return ArmStretchGrabScale; }

	// 꼬리 잡기 F 연타 대결
	UCTGrappleComponent* GetGrapple() const { return Grapple; }

	UFUNCTION(BlueprintPure, Category = "CT|Grapple")
	bool IsGrappling() const;

	// 잡힌 동작(CaughtMontage)이 재생 중인가
	UFUNCTION(BlueprintPure, Category = "CT|Animation")
	bool IsPlayingCaughtMontage() const;

	// 밀려 넘어지는 동작과 일어나는 동작이 재생 중인가 (경직 상태)
	UFUNCTION(BlueprintPure, Category = "CT|Animation")
	bool IsPlayingKnockdownMontage() const;

	// 팔 뼈 스케일: 뼈 길이 축만 ArmStretch, 나머지 1
	UFUNCTION(BlueprintPure, Category = "CT|Animation")
	FVector GetArmStretchScale3D() const;

	// 손 뼈 스케일: 팔이 늘어난 만큼 되돌려 손 크기 유지
	UFUNCTION(BlueprintPure, Category = "CT|Animation")
	FVector GetArmStretchHandScale3D() const;

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

	// AI 조종용 (서버 전용). 사람의 입력과 같은 규칙(쿨타임, 상태 검사)을 서버에서 그대로 적용함
	void AIGrab();
	void AIPush();
	void AISetRun(bool bRun);

	// AI가 거리를 잴 때 쓰는 값
	float GetGrabDistance() const { return GrabDistance; }
	float GetPushDistance() const { return PushDistance; }
	bool IsInFrontOf(const AActor* Other, float MaxDistance, float HalfAngleDegrees) const { return IsInFrontCone(Other, MaxDistance, HalfAngleDegrees); }

	// 지금 내 목소리를 보내는 중인가 (눌러서 말하기를 누르고 있음)
	UFUNCTION(BlueprintPure, Category = "CT|Voice")
	bool IsVoiceTransmitting() const { return bVoiceTransmitting; }

	UFUNCTION(BlueprintPure, Category = "CT|Tail")
	ACTTail* GetOwnedTail() const { return OwnedTail; }

	// 전원 화면 행동 연출 (서버 호출)
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAction(ECTCharacterAction Action, bool bSuccess);

	// 서버 호출: 기믹에 맞은 소리를 전원 화면에서 재생
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitSfx(ECTSfx Sfx);

	// 이모트 재생 요청. SlotIndex 는 숫자 키 순서(0이 키 1번)이고 EmoteLoadout 에 등록된 이모트가 나옴. 서버 확인 뒤 전원 화면에 동작과 말풍선이 나옴
	UFUNCTION(BlueprintCallable, Category = "CT|Emote")
	void RequestEmote(int32 SlotIndex);

	// 숫자 키 자리(0부터)에 이모트 목록의 LibraryIndex 번 이모트를 등록 (이 컴퓨터만)
	UFUNCTION(BlueprintCallable, Category = "CT|Emote")
	void SetEmoteLoadout(int32 KeyPosition, int32 LibraryIndex);

	UFUNCTION(BlueprintPure, Category = "CT|Emote")
	const TArray<FCTEmoteSlot>& GetEmoteLibrary() const { return EmoteSlots; }

	UFUNCTION(BlueprintPure, Category = "CT|Emote")
	const TArray<int32>& GetEmoteLoadout() const { return EmoteLoadout; }

	// 서버 호출: 날아가는 방향(수평)에 따라 넘어지는 연출을 고름. 보는 방향으로 날아가면 앞으로 엎어지고, 반대면 뒤로 넘어짐
	void PlayKnockedDown(const FVector& KnockDirection);

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

	// 카메라가 이 거리(cm)보다 가까우면 내 캐릭터 숨김, 0이면 끔
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "0"))
	float CameraHideDistance = 130.f;

	// 숨김 해제 여유 거리(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "0"))
	float CameraHideHysteresis = 30.f;

	// 카메라 충돌 구 크기(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "1"))
	float CameraProbeSize = 15.f;

	// 카메라 벽 충돌 시 당김, 끄면 시점은 안 튐
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	bool bCameraCollision = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera", meta = (ClampMin = "30", ClampMax = "120"))
	float CameraFieldOfView = 70.f;

	// 점프
	// 점프 시작 속도(cm/s). 높이 = 속도^2 / (2 x 980 x 중력배율). 예전 값은 속도 700, 배율 1 (높이 약 250cm, 체공 1.4초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Jump", meta = (ClampMin = "100"))
	float JumpVelocity = 860.f;

	// 올라갈 때 중력 배율. 높이를 그대로 두려면 JumpVelocity 와 함께 올림 (속도^2 / 배율 이 같으면 높이 같음)
	// 떨어질 때 추가 배율은 캐릭터무브먼트의 FallGravityMultiplier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Jump", meta = (ClampMin = "0.1"))
	float JumpGravityScale = 1.5f;

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

	// 눌러서 말하기 (음성 채팅). 비어 있으면 /Game/Input/IA_PushToTalk
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Voice")
	TObjectPtr<UInputAction> VoiceAction;

	// 이 거리(cm) 안에서는 목소리가 최대 크기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Voice", meta = (ClampMin = "0"))
	float VoiceInnerRadius = 300.f;

	// 최대 크기 거리에서 이만큼(cm) 더 멀어지면 안 들림
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Voice", meta = (ClampMin = "0"))
	float VoiceFalloffDistance = 1500.f;

	// 이모트 목록 (전체). 기본으로 아틀라스 3장의 27개가 들어 있음. BP에서 그림과 동작을 바꿀 수 있음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Emote")
	TArray<FCTEmoteSlot> EmoteSlots;

	// 숫자 키 1, 2, 3...에 등록된 이모트 (EmoteSlots 번호). 칸 수만큼 키가 연결됨 (최대 9)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Emote")
	TArray<int32> EmoteLoadout = { 18, 19, 2, 24 }; // 안녕!, 고마워!, 도와줘!, 흥! (모두 동작이 연결된 칸)

	// 연속으로 누르는 것을 막는 간격(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Emote", meta = (ClampMin = "0"))
	float EmoteCooldownSeconds = 0.5f;

	// 이모트 동작 중 이 속도(cm/s)보다 빨라지면 동작만 끊음 (말풍선은 유지)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Emote", meta = (ClampMin = "0"))
	float EmoteCancelSpeed = 50.f;

	// 머리 위 말풍선
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Emote")
	TObjectPtr<UCTEmoteBubbleComponent> EmoteBubble;

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

	// 밀려남 반응 (맞은 방향으로 앞으로 엎어짐)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> PushedMontage;

	// 앞으로 엎어진 뒤 일어나기 몽타주, 비면 바로 풀림
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> GetUpMontage;

	// 밀려남 반응 (맞은 방향의 반대로, 뒤로 넘어짐). 비면 PushedMontage 를 씀
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> PushedBackMontage;

	// 뒤로 넘어진 뒤 일어나기 몽타주, 비면 바로 풀림
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> GetUpBackMontage;

	// 유령 걷기 반복 몽타주, 섹션명=GhostWalkLoopSection
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	TObjectPtr<UAnimMontage> GhostWalkMontage;

	// 유령 걷기 반복 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation")
	FName GhostWalkLoopSection = TEXT("Loop");

	// 유령 걷기 몽타주 기준 속도(cm/s), 실속도 비례 재생
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "1"))
	float GhostWalkReferenceSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float GhostWalkMinPlayRate = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float GhostWalkMaxPlayRate = 1.5f;

	// 이 속도 미만이면 유령 걷기 모션 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0"))
	float GhostWalkMinSpeed = 30.f;

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

	// 꼬리 잡기 대결 (F 연타 줄다리기)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Grapple")
	TObjectPtr<UCTGrappleComponent> Grapple;

	// true면 AnimBP 없이 코드로 팔 뼈를 직접 늘림. AnimBP 에서 직접 연결했다면 끔
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation|ArmStretch")
	bool bApplyArmStretchToPose = true;

	// 팔 늘리기: 뼈의 길이 축. 길이 대신 굵어지면 다른 축으로 바꿈 (X, Y, Z 중 하나만 1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation|ArmStretch")
	FVector ArmStretchAxis = FVector(1.0, 0.0, 0.0);

	// 잡기 몽타주 중 팔이 최대로 늘어나는 배율 (1 = 안 늘림)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation|ArmStretch", meta = (ClampMin = "1"))
	float ArmStretchGrabScale = 1.5f;

	// 늘어나고 줄어드는 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation|ArmStretch", meta = (ClampMin = "0.1"))
	float ArmStretchInterpSpeed = 14.f;

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
	float GrabDistance = 170.f;

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

	// 밀림 거리 (피해자 키 배수)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushDistanceInHeights = 1.f;

	// 밀기 몽타주 재생 속도 (잡기 등은 ActionMontagePlayRate)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Animation", meta = (ClampMin = "0.1"))
	float PushMontagePlayRate = 2.f;

	// 밀 때 상승 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushUpSpeed = 200.f;

	// 밀기 쿨타임(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float PushCooldownSeconds = 3.f;

	// 잡기 쿨타임(초), 입력 순간부터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Combat", meta = (ClampMin = "0"))
	float GrabCooldownSeconds = 1.5f;

protected:
	// 카메라 초기 설정
	void ApplyCameraSettings();

	// Pitch별 카메라 거리 보간 (로컬만)
	void UpdateCameraDistance(float DeltaSeconds);

	// 걷기/점프/착지 소리, 각자 재생
	void UpdateMovementSounds(float DeltaSeconds);

	void UpdateCameraClip();
// 카메라 근접 시 내 캐릭터 숨김

	// 부쉬 속 캐릭터 숨김 (매 프레임)
	void UpdateBushVisibility();
	void SetHiddenFromOwnCamera(bool bNewHidden);

	// 부쉬 안일 때 내 캐릭터 실루엣
	void UpdateBushXRay();

	// 내 화면 전용 겹친 몸
	UPROPERTY(VisibleAnywhere, Category = "CT|Camera")
	TObjectPtr<USkeletalMeshComponent> XRayMesh;

	// 겹친 몸 머티리얼 (깊이 검사 끔)
	UPROPERTY(EditDefaultsOnly, Category = "CT|Camera")
	TObjectPtr<UMaterialInterface> XRayMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> XRayMaterials;

	bool bXRayActive = false;
	bool bXRaySetup = false;

	// 입력 처리
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnRunPressed();
	void OnRunReleased();
	void OnGrabPressed();

	// 마우스 왼쪽 클릭: 꼬리 줄다리기 중에만 게이지 연타로 인정
	void OnGrappleMouseClick();
	void OnPushPressed();
	void OnVoicePressed();
	void OnVoiceReleased();

	// 이 캐릭터 주인의 목소리를 캐릭터 위치에서 거리에 따라 들리게 함 (모든 화면)
	void EnsureVoiceTalker();
	bool bVoiceTalkerCreated = false;
	bool bVoiceTransmitting = false;
	bool bVoiceKeyHeld = false;

	// 키 입력과 설정(마이크 사용, 상시 말하기)에 맞춰 전송 상태를 맞춤 (로컬)
	void UpdateVoiceTransmit();

	// 거리 감쇠
	UPROPERTY(Transient)
	TObjectPtr<class USoundAttenuation> VoiceAttenuation;

	// 서버 RPC
	UFUNCTION(Server, Reliable)
	void ServerSetRunInput(bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerGrab();

	UFUNCTION(Server, Reliable)
	void ServerPush();

	// 이모트 고르는 창 (R 키로 열고 닫음)
	void ToggleEmoteMenu();
	void CloseEmoteMenu();

	UPROPERTY(Transient)
	TObjectPtr<class UCTEmoteSelectWidget> EmoteMenu;

	// 이모트 고르는 창의 모양 (WBP_CTEmoteSelectWidget). 없으면 R 키를 눌러도 열리지 않음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Emote")
	TSubclassOf<class UCTEmoteSelectWidget> EmoteMenuClass;

	bool bEmoteMenuPrevCursor = false;

	// 이모트 창 키를 눌러 열고 닫은 마지막 시각 (너무 빠른 반복 방지)
	double LastEmoteMenuToggleTime = -1000.0;

	// 이모트
	void OnEmoteKeyPressed(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerPlayEmote(int32 SlotIndex);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEmote(int32 SlotIndex);

	// 이모트 동작을 이동하면 끊음
	void UpdateEmoteMontage();

	// 서버: 마지막 이모트 시각, 로컬: 마지막 입력 시각
	double LastEmoteServerTime = -1000.0;
	double LocalLastEmoteTime = -1000.0;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveEmoteMontage;

	// 팔 늘리기: 매 틱 목표값으로 보간
	void UpdateArmStretch(float DeltaSeconds);

	// 포즈가 확정된 직후 팔 뼈를 늘림 (AnimBP 불필요)
	void ApplyArmStretchToPose();
	FDelegateHandle ArmStretchPoseHandle;

	// 현재 배율 / 수동 목표 (줄다리기 등에서 SetArmStretchTarget 으로 지정)
	float ArmStretch = 1.f;
	float ArmStretchTarget = 1.f;

	// 애니메이션
	// 로컬에서만 몽타주 재생
	void PlayActionMontage(ECTCharacterAction Action);

	// 잡기/밀기 몽타주 중지
	void StopAttackMontages();

	// 달리기 몽타주 반복/정지 (매 프레임)
	void UpdateRunMontage();
	void UpdateGhostWalkMontage();

	// 넘어짐 끝나면 일어나기 재생
	UFUNCTION()
	void HandlePushedMontageEnded(UAnimMontage* Montage, bool bInterrupted);

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

	// 발소리 간격 이동 거리(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Sound", meta = (ClampMin = "10", AllowPrivateAccess = "true"))
	float FootstepDistance = 190.f;

	// 발소리 최소 속도(cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Sound", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	float MinFootstepSpeed = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Sound", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	float FootstepVolume = 0.55f;

	// 착지 소리 최소 낙하 속도(cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Sound", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	float LandMinFallSpeed = 300.f;

	// 거리 기반 발소리, 기본은 노티파이 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Sound", meta = (AllowPrivateAccess = "true"))
	bool bUseDistanceFootsteps = false;

	// 발소리 이동 거리 누적
	float FootstepDistanceAccum = 0.f;
	bool bWasOnGroundForSound = true;
	float LastFallSpeedForSound = 0.f;

	// 근접 카메라로 숨긴 상태 (로컬)
	bool bHiddenFromOwnCamera = false;

	// 마지막 밀기 시각 (서버)
	double LastPushTime = -1000.0;

	// 마지막 밀기 입력 시각 (로컬)
	double LocalLastPushTime = -1000.0;

	// 마지막 잡기 시각 (서버)
	double LastGrabTime = -1000.0;

	// 마지막 잡기 입력 시각 (로컬)
	double LocalLastGrabTime = -1000.0;

	// 지금 들어가 있는 부쉬 (서버 결정, 복제)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentBush)
	TObjectPtr<ACTBush> CurrentBush;

	UFUNCTION()
	void OnRep_CurrentBush();

	// 전화부스 사용 중 (서버 결정, 복제)
	UPROPERTY(Replicated)
	bool bBoothLocked = false;

	// 지금 몸을 숨기고 있는가 (이 컴퓨터)
	bool bBodyHiddenByBush = false;

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
