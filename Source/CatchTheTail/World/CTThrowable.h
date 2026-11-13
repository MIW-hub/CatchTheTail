// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTThrowable.generated.h"

class ACTCharacter;
class UStaticMeshComponent;

// 보물찾기 모드의 던지기 오브젝트 (농구공처럼 물리가 적용되어 월드에 계속 남음).
// 꼬리잡기 키로 집어 들고(머리 위), 같은 키로 캐릭터가 보는 방향으로 던짐. 던져서 캐릭터에 맞으면 서버의 게임 규칙이 에너지를 떨어뜨림.
// 맵마다 다른 모양을 쓰려면 이 클래스를 블루프린트로 상속해 메시만 바꾸고, 그 맵의 GameMode 의 TreasureThrowableClass 에 지정함.
UCLASS(Blueprintable)
class CATCHTHETAIL_API ACTThrowable : public AActor
{
	GENERATED_BODY()

public:
	ACTThrowable();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// 서버: 위치에 하나 만듦
	static ACTThrowable* Spawn(UWorld* World, TSubclassOf<ACTThrowable> ThrowableClass, const FVector& Location);

	// 누가 들고 있는가 / 집을 수 있는 상태인가 (아무도 안 들고, 던져서 날아가는 중이 아님)
	ACTCharacter* GetHolder() const { return HeldBy; }
	bool CanBePickedUp() const { return !HeldBy && !bDangerous; }

	// 서버: 캐릭터가 머리 위로 듦 / 던짐 / 내려놓음
	void AttachToHolder(ACTCharacter* NewHolder);
	void Throw(const FVector& Velocity);
	void Release();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Throwable")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 보이는 크기 배율 (엔진 기본 구 100cm 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Throwable", meta = (ClampMin = "0.1"))
	float VisualScale = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Throwable", meta = (ClampMin = "0.1"))
	float MassKg = 3.f;

	// 튕기는 정도, 마찰
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Throwable", meta = (ClampMin = "0", ClampMax = "1"))
	float Restitution = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Throwable", meta = (ClampMin = "0"))
	float Friction = 0.6f;

	// 던진 뒤 사람을 맞힐 수 있는 시간(초). 이 시간이 지나거나 거의 멈추면 평범한 물체로 돌아감
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Throwable", meta = (ClampMin = "0.1"))
	float DangerSeconds = 2.5f;

	// 던진 사람을 맞히지 않는 시간(초). 던지자마자 자기 몸에 맞지 않게 함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Throwable", meta = (ClampMin = "0"))
	float ThrowerSafeSeconds = 0.4f;

	// 들었을 때 캐릭터 몸 기준 위치 (머리 위)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Throwable")
	FVector HeldOffset = FVector(0.f, 0.f, 135.f);

	// 보이는 색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Throwable")
	FLinearColor BallColor = FLinearColor(1.f, 0.45f, 0.1f, 1.f);

private:
	UFUNCTION()
	void OnRep_HeldBy();

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	// 들었을 때는 물리를 끄고 몸에 붙이고, 놓으면 물리를 켬 (서버와 클라이언트 모두)
	void ApplyHeldState();

	UPROPERTY(ReplicatedUsing = OnRep_HeldBy)
	TObjectPtr<ACTCharacter> HeldBy;

	// 서버 전용: 던져서 날아가는 중인가, 누가 던졌는가, 이번 던지기에서 맞은 대상
	bool bDangerous = false;
	double ThrowTime = 0.0;
	TWeakObjectPtr<ACTCharacter> Thrower;
	TSet<TWeakObjectPtr<AActor>> HitVictims;
};
