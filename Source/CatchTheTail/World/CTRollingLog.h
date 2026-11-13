// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTRollingLog.generated.h"

class ACTCharacter;
class ACTTailPickup;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * 구르는 통나무. 레벨에 놓은 자리에서 PathEnd 까지 지형을 따라 굴러가고, 잠시 쉬었다가 다시 처음부터 굴러옵니다.
 * 위치는 서버 시간으로 모든 화면이 같은 식으로 계산하기 때문에 복제 없이도 부드럽게 움직입니다.
 * 닿은 플레이어는 옆으로 튕겨나가고 꼬리를 하나 잃으며, 잃은 꼬리는 아이템이 되어 날아갑니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTRollingLog : public AActor
{
	GENERATED_BODY()

public:
	ACTRollingLog();

	virtual void Tick(float DeltaSeconds) override;

	// AI가 피하기 위해 읽는 값: 지금 굴러가는 중인가(쉬는 동안은 안 보임), 통나무 위치, 굴러가는 선(월드 좌표)
	bool IsRolling() const;
	FVector GetLogLocation() const;
	FVector GetPathStartWorld() const { return PathStart; }
	FVector GetPathEndWorld() const { return GetActorTransform().TransformPosition(PathEnd); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|RollingLog")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 굴러가는 끝 지점 (이 액터 기준 위치). 뷰포트의 마름모 점을 끌어 정함. 높이(Z)는 무시하고 지형을 따라감
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Path", meta = (MakeEditWidget = true))
	FVector PathEnd = FVector(3000.f, 0.f, 0.f);

	// 굴러가는 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Path", meta = (ClampMin = "50"))
	float RollSpeed = 700.f;

	// 도착 후 다시 굴러오기까지 쉬는 시간(초). 쉬는 동안은 보이지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Path", meta = (ClampMin = "0"))
	float WaitSeconds = 5.f;

	// 시작 시점을 어긋나게 하는 값(초). 통나무를 여러 개 놓을 때 서로 다르게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Path")
	float PhaseOffsetSeconds = 0.f;

	// 모양
	// 통나무 반지름(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Look", meta = (ClampMin = "10"))
	float LogRadius = 110.f;

	// 통나무 길이(cm). 굴러가는 방향과 직각으로 누워 있음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Look", meta = (ClampMin = "50"))
	float LogLength = 450.f;

	// 통나무 머티리얼 (비면 메시 기본)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Look")
	TObjectPtr<UMaterialInterface> LogMaterial;

	// 지형을 따라 오르내리는 부드러움 (클수록 빨리 따라감)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Look", meta = (ClampMin = "1"))
	float GroundFollowSpeed = 12.f;

	// 충돌
	// 한 번 맞은 사람을 다시 맞추기까지 대기(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit", meta = (ClampMin = "0"))
	float HitCooldownSeconds = 1.2f;

	// 옆으로 튕겨나가는 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit", meta = (ClampMin = "0"))
	float KnockbackSideSpeed = 900.f;

	// 위로 솟는 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit", meta = (ClampMin = "0"))
	float KnockbackUpSpeed = 500.f;

	// 적중 범위 배율 (1이면 통나무 모양 그대로)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit", meta = (ClampMin = "0.5"))
	float HitRangeScale = 1.f;

	// 밀려남 몽타주 재생
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit")
	bool bPlayPushedReaction = true;

	// 맞으면 꼬리를 잃음. 잃은 꼬리는 아이템이 되어 날아가고 누구든 주우면 되찾음 (경기 중에만)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit|Tail")
	bool bHitDropsTail = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit|Tail", meta = (ClampMin = "1"))
	int32 HitTailLoss = 1;

	// 날아가는 아이템 클래스 (비면 기본 꼬리 아이템)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit|Tail")
	TSubclassOf<ACTTailPickup> DroppedTailClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit|Tail", meta = (ClampMin = "0"))
	float DroppedTailLaunchSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit|Tail", meta = (ClampMin = "0"))
	float DroppedTailUpSpeed = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit|Tail", meta = (ClampMin = "0"))
	float DroppedTailPickupDelay = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RollingLog|Hit|Tail", meta = (ClampMin = "1"))
	float DroppedTailLifeSeconds = 25.f;

	// 캐릭터를 쳤을 때 (서버)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|RollingLog")
	void BP_OnHitCharacter(ACTCharacter* HitCharacter);

private:
	// 지금 시각의 상태를 계산: 굴러가는 중이면 true, 경로를 따라간 거리와 함께
	bool GetRollState(double ServerTime, float& OutDistance) const;

	// 경로 시작과 끝 (월드, XY만 의미 있음)
	FVector GetPathStart() const { return PathStart; }

	// 바닥 높이 (실패 시 이전 값)
	float FindGroundZ(const FVector& XY, float FallbackZ) const;

	void ApplyHits(const FVector& Location, const FVector& Direction, const FVector& Lateral);

	// 레벨에 놓인 처음 위치와 경로. 액터가 움직이므로 BeginPlay 에서 한 번만 계산해 둠
	FVector PathStart = FVector::ZeroVector;
	float PathLength = 1.f;
	FVector PathDirection = FVector::ForwardVector;
	float CurrentZ = 0.f;
	bool bZInitialized = false;
	bool bWasRolling = false;

	// 서버: 캐릭터별 마지막 적중 시각
	TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;
};
