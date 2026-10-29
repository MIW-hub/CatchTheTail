// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTPhoneBooth.generated.h"

class ACTCharacter;
class UBoxComponent;
class UStaticMeshComponent;

// 전화부스. 쓰면 목표까지 거리를 구간으로 알림
UCLASS()
class CATCHTHETAIL_API ACTPhoneBooth : public AActor
{
	GENERATED_BODY()

public:
	ACTPhoneBooth();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 통화 시간(초). 이동 불가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0.1"))
	float UseDurationSeconds = 2.f;

	// 부스 재사용 대기(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0"))
	float BoothCooldownSeconds = 8.f;

	// 거리 표시 단위(m)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "1"))
	float DistanceStepMeters = 10.f;

	// 안내 문구 표시 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0.5"))
	float NoticeSeconds = 5.f;

	// 사용 시작/종료 훅 (서버)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|PhoneBooth")
	void BP_OnUseStarted(ACTCharacter* User);

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|PhoneBooth")
	void BP_OnUseFinished(ACTCharacter* User, bool bSuccess);

private:
	void ServerTick(double Now);
	void BeginUse(ACTCharacter* Character, double Now);
	void FinishUse(double Now);
	void CancelUse();
	bool IsCharacterInside(const ACTCharacter* Character) const;

	// 게임 중, 유령 아님, 색 배정됨
	static bool CanUsePhone(const ACTCharacter* Character);

	TWeakObjectPtr<ACTCharacter> CurrentUser;
	double UseEndTime = 0.0;
	double CooldownEndTime = 0.0;

	// 나갔다 와야 다시 쓸 수 있는 사람
	TSet<TWeakObjectPtr<ACTCharacter>> WaitForExit;

	float TickAccumulator = 0.f;
};
