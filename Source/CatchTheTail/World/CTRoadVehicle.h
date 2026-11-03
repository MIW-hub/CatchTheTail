// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTRoadVehicle.generated.h"

class ACTCharacter;
class ACTTailPickup;
class USceneComponent;
class USkeletalMeshComponent;
class USplineComponent;

/**
 * 도로를 따라 계속 달리는 자동차. 스플라인 곡선(닫힌 고리)을 따라 일정한 속도로 돌고, 닿은 플레이어는 쓰러지며 꼬리를 잃습니다.
 * 위치는 서버 시간으로 모든 화면이 같은 식으로 계산하기 때문에 복제 없이도 같은 자리에 보입니다.
 * 경로는 이 액터의 스플라인 점을 뷰포트에서 끌어 고칩니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTRoadVehicle : public AActor
{
	GENERATED_BODY()

public:
	ACTRoadVehicle();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle")
	TObjectPtr<USceneComponent> Root;

	// 달리는 경로. 닫힌 고리로 쓰며 점을 끌어 코너 곡선을 만듦
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle")
	TObjectPtr<USplineComponent> Path;

	// 자동차 모양 (경로를 따라 움직임)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle")
	TObjectPtr<USkeletalMeshComponent> CarMesh;

	// 달리는 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Path", meta = (ClampMin = "50"))
	float DriveSpeed = 900.f;

	// 경로에서 출발하는 자리를 어긋나게 하는 값(초). 같은 경로에 여러 대를 놓을 때 서로 다르게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Path")
	float PhaseOffsetSeconds = 0.f;

	// 경로를 반대로 달림
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Path")
	bool bReverse = false;

	// 자동차를 경로 위로 띄우는 높이(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Look")
	float HeightOffset = 0.f;

	// 경기 중이 아닐 때도 달림 (끄면 경기 중에만 움직임)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Path")
	bool bDriveOutsideMatch = true;

	// 충돌
	// 적중 범위의 절반 크기(cm). X=앞뒤, Y=좌우, Z=높이. 자동차 모양에 맞게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit")
	FVector HitHalfExtent = FVector(190.f, 120.f, 100.f);

	// 적중 범위 중심을 차 중심에서 위로 올리는 높이(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit")
	float HitCenterHeight = 100.f;

	// 한 번 맞은 사람을 다시 맞추기까지 대기(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit", meta = (ClampMin = "0"))
	float HitCooldownSeconds = 1.5f;

	// 앞으로 튕겨나가는 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit", meta = (ClampMin = "0"))
	float KnockbackForwardSpeed = 700.f;

	// 옆으로 비켜나가는 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit", meta = (ClampMin = "0"))
	float KnockbackSideSpeed = 600.f;

	// 위로 솟는 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit", meta = (ClampMin = "0"))
	float KnockbackUpSpeed = 550.f;

	// 밀려남 몽타주 재생
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit")
	bool bPlayPushedReaction = true;

	// 맞으면 꼬리를 잃음. 잃은 꼬리는 아이템이 되어 날아가고 누구든 주우면 되찾음 (경기 중에만)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit|Tail")
	bool bHitDropsTail = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit|Tail", meta = (ClampMin = "1"))
	int32 HitTailLoss = 1;

	// 날아가는 아이템 클래스 (비면 기본 꼬리 아이템)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit|Tail")
	TSubclassOf<ACTTailPickup> DroppedTailClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit|Tail", meta = (ClampMin = "0"))
	float DroppedTailLaunchSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit|Tail", meta = (ClampMin = "0"))
	float DroppedTailUpSpeed = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit|Tail", meta = (ClampMin = "0"))
	float DroppedTailPickupDelay = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|RoadVehicle|Hit|Tail", meta = (ClampMin = "1"))
	float DroppedTailLifeSeconds = 25.f;

	// 캐릭터를 쳤을 때 (서버)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|RoadVehicle")
	void BP_OnHitCharacter(ACTCharacter* HitCharacter);

private:
	void ApplyHits(const FVector& Location, const FQuat& Rotation);

	// 서버: 캐릭터별 마지막 적중 시각
	TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;
};
