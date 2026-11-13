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
	static ACTEnergyPickup* Spawn(UWorld* World, const FVector& Location, int32 Amount = 1);

	int32 GetAmount() const { return Amount; }

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

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bConsumed = false;
};
