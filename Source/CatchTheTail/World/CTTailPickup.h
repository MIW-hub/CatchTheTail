// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTTailPickup.generated.h"

class ACTCharacter;
class UMaterialInterface;
class UProjectileMovementComponent;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class USphereComponent;

/**
 * 꼬리 아이템. 닿는 플레이어의 꼬리가 늘어납니다 (기본 +1).
 * 레벨에 직접 놓거나 TailPickupSpawner 가 만들 수 있고, 낙하물에 맞은 플레이어의 꼬리가 하나 튕겨 나올 때도 이 아이템이 날아갑니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTTailPickup : public AActor
{
	GENERATED_BODY()

public:
	ACTTailPickup();

	virtual void Tick(float DeltaSeconds) override;

	// 서버: 이 방향/속도로 날려 보냄 (튕겨 나온 꼬리). 땅에 닿으면 몇 번 튀고 멈춤.
	// SpawnActorDeferred 로 만든 직후, FinishSpawning 전에 불러야 함
	void LaunchAway(const FVector& Velocity, float NewPickupDelaySeconds, float NewLifeSeconds);

	int32 GetTailAmount() const { return TailAmount; }

	/**
	 * 서버: 캐릭터가 꼬리를 TailLoss 개(있는 만큼) 잃고, 잃은 수만큼 꼬리 아이템이 날아감 (경기 중에만).
	 * 낙하물, 구르는 통나무 등 꼬리를 잃는 기믹이 함께 씀. 실제로 잃은 수를 돌려줌
	 * @param Direction 아이템이 흩어질 기준 방향(수평)
	 */
	static int32 DropTailsFromCharacter(ACTCharacter* Character, int32 TailLoss, const FVector& Direction, TSubclassOf<ACTTailPickup> PickupClass,
		float LaunchSpeed, float UpSpeed, float PickupDelay, float LifeSeconds, UObject* OwnerObject);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TObjectPtr<USphereComponent> Trigger;

	// 보이는 부분 (회전, 위아래 흔들림 대상). 받침 + 몸통 + 끝 캡
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TObjectPtr<USkeletalMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TObjectPtr<USkeletalMeshComponent> CapMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TObjectPtr<UProjectileMovementComponent> Movement;

	// 먹으면 늘어나는 꼬리 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "1"))
	int32 TailAmount = 1;

	// 유령도 먹을 수 있음. 먹으면 그 자리에서 되살아나며 꼬리가 최소 TailAmount 개가 됨 (기권한 플레이어는 제외)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	bool bAllowGhostPickup = true;

	// 있으면 몸통(tail_02)과 캡(tail_01)에 이 머티리얼을 씀. 받침(tail_03)은 그대로 (비면 모두 메시 기본 머티리얼)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	TObjectPtr<UMaterialInterface> ItemMaterial;

	// 놓인 뒤 먹을 수 있기까지 대기(초). 튕겨 나온 꼬리를 주인이 바로 되먹지 않게 함
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "0"))
	float PickupDelaySeconds = 0.f;

	// 놓인 뒤 사라지기까지(초), 0이면 계속 유지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "0"))
	float LifeSeconds = 0.f;

	// 제자리에서 도는 속도(도/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup")
	float SpinDegreesPerSecond = 90.f;

	// 위아래로 흔들리는 높이(cm)와 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "0"))
	float BobHeight = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|TailPickup", meta = (ClampMin = "0"))
	float BobSpeed = 2.5f;

	// 먹었을 때 (서버)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|TailPickup")
	void BP_OnPickedUp(ACTCharacter* Picker);

private:
	// 서버: 닿은 캐릭터가 먹을 수 있으면 먹음
	void TryPickup(ACTCharacter* Character);

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	double PickupReadyTime = 0.0;
	bool bConsumed = false;
	bool bLaunchConfigured = false;
	float VisualTime = 0.f;
};
