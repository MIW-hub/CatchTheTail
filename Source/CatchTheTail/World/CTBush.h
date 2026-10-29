// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTBush.generated.h"

class ACTCharacter;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * 부쉬. 안에 들어가면 같은 부쉬 안에 있는 사람을 뺀 다른 사람 화면에서 몸이 숨겨집니다. (꼬리는 그대로 보임)
 * - 숨어 있는 동안 잡기, 밀기 불가
 * - 오래 숨으면(기본 3초) 풀이 빠르게 흔들려 들킴. 모두 나가면 다시 천천히
 * 레벨에 놓은 뒤 박스(트리거) 크기와 메시를 조절하세요. 맵마다 몇 개만, 작게 두는 것을 권장합니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTBush : public AActor
{
	GENERATED_BODY()

public:
	ACTBush();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// 풀이 빠르게 흔들리는 중인가 (오래 숨은 사람이 있음)
	UFUNCTION(BlueprintPure, Category = "CT|Bush")
	bool IsAlerted() const { return bAlerted; }

protected:
	virtual void BeginPlay() override;

	// 안에 들어온 것을 감지하는 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Bush")
	TObjectPtr<UBoxComponent> Trigger;

	// 보이는 풀. 블루프린트에서 메시를 바꾸세요
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|Bush")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 이만큼(초) 계속 숨으면 풀이 빠르게 흔들림
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush", meta = (ClampMin = "0"))
	float AlertDelaySeconds = 3.f;

	// 평소 흔들림: 속도(회/초), 기울기(도)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush|Shake", meta = (ClampMin = "0"))
	float IdleShakeSpeed = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush|Shake", meta = (ClampMin = "0"))
	float IdleShakeAngle = 1.5f;

	// 들켰을 때 흔들림
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush|Shake", meta = (ClampMin = "0"))
	float AlertShakeSpeed = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Bush|Shake", meta = (ClampMin = "0"))
	float AlertShakeAngle = 6.f;

	// 들켰을 때 연출 (소리 등을 블루프린트에서)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Bush")
	void BP_OnAlertChanged(bool bNewAlerted);

private:
	// 서버: 안에 있는 사람을 다시 확인
	void ServerRefreshOccupants();

	UFUNCTION()
	void OnRep_Alerted();

	UPROPERTY(ReplicatedUsing = OnRep_Alerted)
	bool bAlerted = false;

	// 서버: 들어온 시각
	TMap<TWeakObjectPtr<ACTCharacter>, double> Occupants;

	float OccupantCheckAccumulator = 0.f;
	float ShakePhase = 0.f;
	FRotator MeshBaseRotation = FRotator::ZeroRotator;
};
