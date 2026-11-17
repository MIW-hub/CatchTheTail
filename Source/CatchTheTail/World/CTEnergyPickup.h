// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTEnergyPickup.generated.h"

class ACTCharacter;
class USphereComponent;
class UStaticMeshComponent;

// 보물찾기 모드의 에너지. 닿는 캐릭터가 먹으면 서버의 게임 규칙(UCTGameRule::OnEnergyPicked)에 알리고 사라짐.
// 꼬리 아이템(ACTTailPickup)과 달리 가볍게 만들어 한 라운드에 100개를 깔 수 있음
UCLASS(NotBlueprintable)
class CATCHTHETAIL_API ACTEnergyPickup : public AActor
{
	GENERATED_BODY()

public:
	ACTEnergyPickup();

	virtual void Tick(float DeltaSeconds) override;

	// 서버: 위치에 에너지 하나를 만듦. Amount 는 먹었을 때 늘어나는 양
	// FallDelay 가 0 이상이면 그만큼 기다렸다가 하늘에서 떨어져 Location 에 내려앉음 (내려앉기 전에는 먹을 수 없음). 음수면 바로 그 자리에 놓임
	static ACTEnergyPickup* Spawn(UWorld* World, const FVector& Location, int32 Amount = 1, float FallDelay = -1.f);

	// 서버: 맞은 사람의 몸(From)에서 Location 으로 터지듯 튀어 나가 내려앉음
	static ACTEnergyPickup* SpawnBurst(UWorld* World, const FVector& From, const FVector& Location, int32 Amount = 1);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetAmount() const { return Amount; }

	// 바닥에 내려앉아 먹을 수 있는 상태인가 (하늘에서 떨어지는 중이면 false)
	bool IsLanded() const { return bLanded; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "CT|Energy")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "CT|Energy")
	TObjectPtr<UStaticMeshComponent> Visual;

	// 먹으면 늘어나는 에너지
	UPROPERTY(EditDefaultsOnly, Category = "CT|Energy", meta = (ClampMin = "1"))
	int32 Amount = 1;

	// 보이는 크기와 도는 속도
	UPROPERTY(EditDefaultsOnly, Category = "CT|Energy")
	float VisualScale = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "CT|Energy")
	float SpinDegreesPerSecond = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "CT|Energy")
	FLinearColor EnergyColor = FLinearColor(1.f, 0.85f, 0.1f, 1.f);

	// 떨어지기 시작하기까지 기다리는 시간(초), 음수면 떨어지지 않음. 처음 한 번만 복제
	UPROPERTY(Replicated)
	float FallDelay = -1.f;

	// 하늘에서 떨어질 때 시작 높이(cm)와 걸리는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "CT|Energy")
	float DropHeight = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "CT|Energy")
	float DropSeconds = 0.9f;

	// 터지듯 튀어 나갈 때: 시작 위치(놓이는 자리 기준 상대), 가장 높이 솟는 높이(cm), 걸리는 시간(초). 처음 한 번만 복제
	UPROPERTY(Replicated)
	FVector LaunchOffset = FVector::ZeroVector;

	UPROPERTY(Replicated)
	float ArcHeight = 0.f;

	UPROPERTY(Replicated)
	float FallSeconds = 0.9f;

private:
	void EnableTrigger();
	float SpawnTime = 0.f;
	bool bLanded = true;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bConsumed = false;
};
