// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTTailPickupSpawner.generated.h"

class ACTTailPickup;
class UBoxComponent;

/**
 * 꼬리 아이템을 영역 안의 무작위 바닥 위치에 일정 간격으로 만들어 줍니다. 서버에서만 동작합니다.
 * 같은 시각에 맵에 있는 아이템 수는 MaxPickups 까지만 유지합니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTTailPickupSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACTTailPickupSpawner();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	// 스폰 영역 (XY 범위 안에서 위에서 아래로 바닥을 찾음)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TObjectPtr<UBoxComponent> SpawnBox;

	// 만들 아이템 클래스 (비면 기본 꼬리 아이템)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TSubclassOf<ACTTailPickup> PickupClass;

	// 스폰 간격(초) 최소 / 최대
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "0.5"))
	float SpawnIntervalMin = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "0.5"))
	float SpawnIntervalMax = 18.f;

	// 동시에 맵에 있을 수 있는 아이템 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "1"))
	int32 MaxPickups = 3;

	// 경기 중에만 스폰
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	bool bOnlySpawnWhilePlaying = true;

	// 바닥에서 띄우는 높이(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "0"))
	float SpawnHeightAboveGround = 60.f;

	// 아이템이 사라지기까지(초), 0이면 먹을 때까지 유지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "0"))
	float PickupLifeSeconds = 40.f;

	// 위에서 튕겨 나오듯 등장 (끄면 바닥 위 SpawnHeightAboveGround 에 가만히 생김)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup|Pop")
	bool bPopOutOnSpawn = true;

	// 시작 높이(cm, 바닥 기준)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup|Pop", meta = (EditCondition = "bPopOutOnSpawn", ClampMin = "0"))
	float PopStartHeight = 450.f;

	// 처음 위로 솟는 속도와 옆으로 퍼지는 속도 (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup|Pop", meta = (EditCondition = "bPopOutOnSpawn", ClampMin = "0"))
	float PopUpSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup|Pop", meta = (EditCondition = "bPopOutOnSpawn", ClampMin = "0"))
	float PopSideSpeed = 250.f;

	// 등장 후 먹을 수 있기까지 대기(초). 튀는 동안 공중에서 먹히지 않게
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup|Pop", meta = (EditCondition = "bPopOutOnSpawn", ClampMin = "0"))
	float PopPickupDelay = 0.5f;

	// 바닥 탐색 채널
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

private:
	bool TrySpawnOne();
	float PickInterval() const;

	TArray<TWeakObjectPtr<ACTTailPickup>> Spawned;
	float TimeToNextSpawn = 5.f;
};
