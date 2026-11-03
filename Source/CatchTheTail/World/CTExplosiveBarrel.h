// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Core/CTAudioSettings.h"
#include "CTExplosiveBarrel.generated.h"

class ACTCharacter;
class ACTTailPickup;
class UMaterialInterface;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ECTBarrelState : uint8
{
	Idle,       // 대기 중. 누가 닿으면 도화선이 타기 시작
	Burning,    // 닿은 뒤 폭발을 기다리는 중
	Exploded,   // 터진 뒤 다시 나타날 때까지
};

/**
 * 폭발 드럼통. 플레이어가 닿으면 FuseSeconds 뒤에 터져서 반경 안의 플레이어를 날리고 꼬리를 떨어뜨립니다.
 * 터진 뒤 RespawnSeconds 가 지나면 다시 나타납니다 (0이면 한 번만).
 * 상태는 서버가 정하고 복제되며, 도화선이 타는 동안의 깜빡임은 각 화면이 서버 시간으로 계산합니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTExplosiveBarrel : public AActor
{
	GENERATED_BODY()

public:
	ACTExplosiveBarrel();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Barrel")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 이 범위에 들어온 플레이어가 닿은 것으로 봄
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Barrel")
	TObjectPtr<USphereComponent> TouchTrigger;

	// 닿은 뒤 폭발까지 걸리는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel", meta = (ClampMin = "0.1"))
	float FuseSeconds = 3.f;

	// 터진 뒤 다시 나타나기까지(초). 0이면 다시 나타나지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel", meta = (ClampMin = "0"))
	float RespawnSeconds = 15.f;

	// 도화선이 타는 동안 깜빡이는 빠르기(초당 횟수). 폭발이 가까울수록 빨라짐
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Look", meta = (ClampMin = "0"))
	float BlinkStartRate = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Look", meta = (ClampMin = "0"))
	float BlinkEndRate = 10.f;

	// 깜빡일 때 커지는 정도 (0.1이면 최대 10% 커짐)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Look", meta = (ClampMin = "0"))
	float BlinkScaleAmount = 0.12f;

	// 도화선이 타는 동안 덧씌우는 머티리얼 (예: 빨간색 번쩍임). 비워두면 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Look")
	TObjectPtr<UMaterialInterface> BurningOverlayMaterial;

	// 도화선이 타기 시작할 때 / 폭발할 때 소리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Look")
	TObjectPtr<USoundBase> FuseSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Look")
	TObjectPtr<USoundBase> ExplosionSound;

	// 폭발
	// 폭발 반경(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion", meta = (ClampMin = "50"))
	float ExplosionRadius = 450.f;

	// 바깥으로 날아가는 속도(cm/초). 가운데일수록 세고 가장자리는 MinSpeedRatio 배
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion", meta = (ClampMin = "0"))
	float KnockbackSpeed = 1300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion", meta = (ClampMin = "0", ClampMax = "1"))
	float MinSpeedRatio = 0.5f;

	// 위로 솟는 속도(cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion", meta = (ClampMin = "0"))
	float KnockbackUpSpeed = 700.f;

	// 쓰러지는 몽타주 재생
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion")
	bool bPlayPushedReaction = true;

	// 맞으면 꼬리를 잃음. 잃은 꼬리는 아이템이 되어 날아가고 누구든 주우면 되찾음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion|Tail")
	bool bHitDropsTail = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion|Tail", meta = (ClampMin = "1"))
	int32 HitTailLoss = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion|Tail")
	TSubclassOf<ACTTailPickup> DroppedTailClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion|Tail", meta = (ClampMin = "0"))
	float DroppedTailLaunchSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion|Tail", meta = (ClampMin = "0"))
	float DroppedTailUpSpeed = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion|Tail", meta = (ClampMin = "0"))
	float DroppedTailPickupDelay = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Barrel|Explosion|Tail", meta = (ClampMin = "1"))
	float DroppedTailLifeSeconds = 25.f;

	// 도화선이 타기 시작했을 때 (모든 화면). 불꽃 연출용
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Barrel")
	void BP_OnFuseStarted();

	// 폭발했을 때 (모든 화면). 폭발 이펙트용
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Barrel")
	void BP_OnExploded();

	// 폭발에 휘말린 캐릭터 (서버)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Barrel")
	void BP_OnHitCharacter(ACTCharacter* HitCharacter);

private:
	UFUNCTION()
	void HandleTouchBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_State();

	// 지정한 소리가 없을 때 쓰는 기본 소리 (모든 화면)
	void PlayDefaultSfx(ECTSfx Sfx) const;

	// 서버: 도화선 시작 / 폭발 / 다시 나타남
	void StartFuse();
	void Explode();
	void Respawn();

	// 상태 바뀐 것을 화면에 반영 (모든 화면)
	void ApplyStateVisuals(ECTBarrelState OldState);

	bool IsPlaying() const;
	double GetServerNow() const;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	ECTBarrelState State = ECTBarrelState::Idle;

	// 도화선이 폭발하는 서버 시각
	UPROPERTY(Replicated)
	double ExplodeServerTime = 0.0;

	ECTBarrelState AppliedState = ECTBarrelState::Idle;
	FVector BaseScale = FVector::OneVector;
	FTimerHandle FuseTimer;
	FTimerHandle RespawnTimer;
};
