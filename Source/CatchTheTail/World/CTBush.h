// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTBush.generated.h"

class ACTCharacter;
class UBoxComponent;
class UStaticMeshComponent;

// 숨는 풀. 안에 있으면 다른 화면에서 몸이 가려짐
UCLASS()
class CATCHTHETAIL_API ACTBush : public AActor
{
	GENERATED_BODY()

public:
	ACTBush();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// 오래 숨은 사람이 있어 흔들리는 중
	UFUNCTION(BlueprintPure, Category = "CT|Bush")
	bool IsAlerted() const { return bAlerted; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Bush")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Bush")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 이 시간(초) 숨으면 들킴
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush", meta = (ClampMin = "0"))
	float AlertDelaySeconds = 3.f;

	// 평소 흔들림 속도(회/초), 기울기(도)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush|Shake", meta = (ClampMin = "0"))
	float IdleShakeSpeed = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush|Shake", meta = (ClampMin = "0"))
	float IdleShakeAngle = 1.5f;

	// 들켰을 때 흔들림
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush|Shake", meta = (ClampMin = "0"))
	float AlertShakeSpeed = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush|Shake", meta = (ClampMin = "0"))
	float AlertShakeAngle = 6.f;

	// 들킴 연출 훅
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Bush")
	void BP_OnAlertChanged(bool bNewAlerted);

private:
	// 서버: 점유자 재확인
	void ServerRefreshOccupants();

	UFUNCTION()
	void OnRep_Alerted();

	UPROPERTY(ReplicatedUsing = OnRep_Alerted)
	bool bAlerted = false;

	// 서버: 입장 시각
	TMap<TWeakObjectPtr<ACTCharacter>, double> Occupants;

	float OccupantCheckAccumulator = 0.f;
	float ShakePhase = 0.f;
	FRotator MeshBaseRotation = FRotator::ZeroRotator;
};
