// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CTAIController.generated.h"

class ACTCharacter;
class ACTTailPickup;
class ACTEnergyPickup;
class ACTThrowable;

/**
 * AI 플레이어 조종자 (서버에서만 동작).
 * - 시야(앞쪽 부채꼴 + 가까운 둘레, 벽이 가리면 못 봄) 또는 발소리 범위(움직이는 상대)에 들어온 상대만 알아챔
 * - 목표 색 상대를 알아채면 쫓아가서 잡음. 시야에서 벽으로 놓쳐도 기억하는 동안 길을 돌아 계속 추적
 * - 나를 노리는 상대를 알아채면 도망치고, 붙으면 밀어냄
 * - 목표가 아닌 상대도 가까우면 가끔 밀 수 있음
 * 길찾기는 NavMesh를 쓰고, 없으면 직선으로 움직임
 */
UCLASS()
class CATCHTHETAIL_API ACTAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACTAIController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnUnPossess() override;

protected:
	// 꼬리 대결 중이거나 잡힌 동작이 끝난 뒤, 탐색을 이 시간(초)만큼 더 쉼
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float GrappledPauseSeconds = 0.6f;

	// 달리기를 켠 뒤 이 시간(초) 동안은 끄지 않음 (판단이 흔들려 달리기가 깜빡이는 것을 막음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float RunMinHoldSeconds = 1.5f;

	// 판단 간격(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0.02"))
	float ThinkInterval = 0.12f;

	// 시야 거리(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float SightRange = 2200.f;

	// 시야각 절반(도). 정면 기준
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0", ClampMax = "180"))
	float SightHalfAngleDegrees = 100.f;

	// 이 거리 안은 뒤쪽이어도 보임(벽에 가리면 못 봄)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float CloseSenseRadius = 300.f;

	// 발소리가 들리는 거리(cm). 0이면 오디오 설정의 발소리 범위를 따름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float HearingRadius = 0.f;

	// 발소리가 나는 최소 속도(cm/s). 가만히 선 상대는 안 들림
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float HearingMinSpeed = 120.f;

	// 시야 눈높이(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI")
	float EyeHeight = 70.f;

	// 놓친 목표를 기억하는 시간(초). 이 동안은 마지막 위치로 계속 추적
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float ChaseMemorySeconds = 5.f;

	// 이 거리 안에 나를 노리는 상대가 있으면 도망 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float FleeStartDistance = 1300.f;

	// 이 거리 안이면 달려서 도망/추격 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float RunDistance = 800.f;

	// 도망 목적지를 찾는 반경(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "100"))
	float FleeSampleRadius = 1800.f;

	// 아무도 못 알아챘을 때 돌아다니는 반경(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "100"))
	float WanderRadius = 2500.f;

	// 맵 가운데 쪽에서 놀게 하는 기능 (켜면 가장자리로 흩어지지 않음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Center")
	bool bPreferMapCenter = true;

	// 맵 가운데를 직접 지정 (끄면 NavMesh 범위의 가운데를 자동으로 씀)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Center", meta = (EditCondition = "bPreferMapCenter"))
	bool bUseCenterOverride = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Center", meta = (EditCondition = "bUseCenterOverride"))
	FVector CenterOverride = FVector::ZeroVector;

	// 가운데에서 이 거리(cm) 안이 주로 노는 구역
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Center", meta = (ClampMin = "100"))
	float CenterRoamRadius = 3000.f;

	// 가운데에서 이 거리(cm)보다 멀어지면 다음 목적지를 가운데 구역에서 고름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Center", meta = (ClampMin = "100"))
	float CenterPullDistance = 3500.f;

	// 배회 목적지를 고를 때 가운데에 가까운 곳을 선호하는 정도 (0 = 상관없음, 1 = 거의 항상)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Center", meta = (ClampMin = "0", ClampMax = "1"))
	float WanderCenterBias = 0.7f;

	// 도망 목적지를 고를 때 가운데에서 먼 곳을 싫어하는 정도 (0 = 상관없음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Center", meta = (ClampMin = "0"))
	float FleeCenterWeight = 0.5f;

	// 목표가 아닌 상대가 밀 수 있는 거리에 있을 때 한 번 판단마다 밀 확률 (0~1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0", ClampMax = "1"))
	float RandomPushChance = 0.08f;

	// 자동차, 통나무가 이 거리 안(옆으로)을 지나가면 피함 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Hazard", meta = (ClampMin = "0"))
	float HazardAvoidRadius = 420.f;

	// 다가오는 위험물을 몇 초 앞까지 내다볼지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Hazard", meta = (ClampMin = "0"))
	float HazardLookAheadSeconds = 1.8f;

	// 배회, 도망 목적지를 위험물 경로에서 이만큼 떨어뜨림 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Hazard", meta = (ClampMin = "0"))
	float HazardPathClearance = 450.f;

	// 꼬리 아이템이 보이는 거리(cm). 보이면 다른 행동보다 먼저 먹으러 감
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float PickupSightRange = 2500.f;

	// 아이템을 놓친 뒤 마지막 위치로 계속 가는 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float PickupMemorySeconds = 3.f;

	// 나를 노리는 상대가 이 거리 안이면 아이템보다 도망이 먼저 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI", meta = (ClampMin = "0"))
	float PickupAbortHunterDistance = 600.f;

	// 나를 쫓는 사람으로 의심하는 최대 거리(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Suspicion", meta = (ClampMin = "0"))
	float SuspectRange = 1300.f;

	// 나에게 다가오는 속도가 이 이상이면 쫓아오는 걸로 봄 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Suspicion", meta = (ClampMin = "0"))
	float SuspectClosingSpeed = 180.f;

	// 이 시간(초) 동안 계속 다가오면 도망 (낮을수록 겁이 많음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|AI|Suspicion", meta = (ClampMin = "0"))
	float SuspectSeconds = 0.7f;

private:
	void Think();

	// 위험물(자동차, 통나무, 떨어지는 물건, 드럼통)을 피함. 피하는 중이면 true (다른 행동보다 우선)
	bool AvoidHazards(ACTCharacter& Me, double Now);

	// 이 지점이 위험물의 경로 가까이인가 (배회, 도망 목적지를 고를 때 거름)
	bool IsNearHazardPath(const FVector& Point) const;

	void RefreshHazardList(double Now);

	// Other가 나를 노리는 것 같은가 (계속 똑바로 다가오면 의심 점수가 쌓임). 의심되면 true
	bool UpdateSuspicion(ACTCharacter& Other, const FVector& MyLocation, float DeltaTime, double Now);
	void DecayUnseenSuspects(float DeltaTime, double Now);

	// 눈에 보이는(앞쪽 시야, 가까운 둘레, 벽에 안 가림) 꼬리 아이템 중 가장 가까운 것을 향해 감. 가는 중이면 true
	// bIgnoreHazards: 유령은 위험물에 안 맞으므로 도로 근처 아이템도 먹으러 감
	bool GoToTailPickup(ACTCharacter& Me, double Now, bool bIgnoreHazards);

	// 보물찾기 모드: 가장 가까운 에너지를 주우러 다니고, 없으면 돌아다님 (위험물은 피함)
	void ThinkTreasure(ACTCharacter& Me, double Now);
	TArray<TWeakObjectPtr<ACTEnergyPickup>> EnergyList;
	TArray<TWeakObjectPtr<ACTThrowable>> ThrowableList;
	double NextThrowTime = 0.0;
	double HoldStartTime = 0.0;
	double NextEnergyRefreshTime = 0.0;

	// 위치가 눈에 보이는가 (거리, 부채꼴, 시선이 안 가림)
	bool SeesLocation(const ACTCharacter& Me, const FVector& Location, const AActor* IgnoredActor) const;

	// Other를 알아챘는가 (발소리 또는 시야)
	bool Senses(const ACTCharacter& Me, const ACTCharacter& Other, const AActor* ExtraIgnoredActor = nullptr) const;

	// 이동 명령. 길찾기가 안 되면 직선 이동으로 바꿈
	void MoveToPoint(const FVector& Destination, float AcceptRadius);
	void MoveToTarget(const ACTCharacter& Target, float AcceptRadius);
	void Halt();
	void FaceLocation(ACTCharacter& Me, const FVector& Location) const;

	// 달리기 켜기/끄기. 달리기를 시작하자마자 판단이 바뀌어 바로 끄면 "일찍 멈춤" 쿨타임에 걸려
	// 달리기가 깜빡이고 못 달리므로, 켠 뒤 RunMinHoldSeconds 동안은 끄기를 무시함
	void SetAIRun(ACTCharacter& Me, bool bRun);
	double RunStartedAt = -1000.0;

	// 도망 목적지: 나를 노리는 상대에게서 가장 먼 갈 수 있는 곳
	FVector PickFleePoint(const FVector& MyLocation, const FVector& HunterLocation) const;
	FVector PickWanderPoint(const FVector& MyLocation) const;

	// 놀이 구역의 가운데 (NavMesh 범위의 가운데, 없으면 시작점들의 평균)
	FVector GetPlayAreaCenter() const;
	mutable FVector CachedCenter = FVector::ZeroVector;
	mutable bool bCenterCached = false;

	// 대결, 잡힘으로 쉬는 중
	double PausedUntil = 0.0;
	bool bPaused = false;

	double NextThinkTime = 0.0;
	double NextMoveCommandTime = 0.0;
	double NextWanderTime = 0.0;
	double LastPushTime = -1000.0;
	double NextDebugLogTime = 0.0;

	// 위험물 목록과 속도 추적
	TArray<TWeakObjectPtr<AActor>> Hazards;
	TMap<TWeakObjectPtr<AActor>, FVector> HazardPrevLocations;
	double NextHazardScanTime = 0.0;
	double LastHazardThinkTime = 0.0;

	// 추격자 의심
	struct FSuspect
	{
		float Score = 0.f;
		FVector LastLocation = FVector::ZeroVector;
		double LastSeenTime = 0.0;
		bool bTouched = false;
	};
	TMap<TWeakObjectPtr<ACTCharacter>, FSuspect> Suspects;
	double LastSuspicionTime = 0.0;

	// 꼬리 아이템 목록과 놓친 뒤 잠시 기억
	TArray<TWeakObjectPtr<ACTTailPickup>> Pickups;
	TWeakObjectPtr<ACTTailPickup> RememberedPickup;
	double LastPickupSeenTime = -1000.0;

	// 놓친 목표 기억
	TWeakObjectPtr<ACTCharacter> RememberedPrey;
	FVector RememberedPreyLocation = FVector::ZeroVector;
	double LastPreySenseTime = -1000.0;

	// 길찾기 실패 시 직선 이동
	bool bDirectMove = false;
	FVector DirectMoveTarget = FVector::ZeroVector;
};
