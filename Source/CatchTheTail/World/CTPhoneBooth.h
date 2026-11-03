// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Type/CTTypes.h"
#include "CTPhoneBooth.generated.h"

class ACTCharacter;
class UBoxComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

/**
 * 전화부스. 안에서 F를 누르면 팝업이 뜨고, 목표나 헌터 중 하나를 골라 거리와 방향을 알아냄.
 * 통화하는 동안은 움직일 수 없고, 한 번 건 뒤에는 플레이어마다 쿨타임이 지나야 다시 걸 수 있음.
 */
UCLASS()
class CATCHTHETAIL_API ACTPhoneBooth : public AActor
{
	GENERATED_BODY()

public:
	ACTPhoneBooth();

	virtual void Tick(float DeltaSeconds) override;

	// 이 캐릭터가 부스 안에 있는가
	bool IsCharacterInside(const ACTCharacter* Character) const;

	// 캐릭터가 들어가 있는 부스 (없으면 nullptr)
	static ACTPhoneBooth* FindBoothContaining(const ACTCharacter* Character);

	// 게임 중, 유령이 아님, 색 배정됨
	static bool CanUsePhone(const ACTCharacter* Character);

	// 서버: 통화 시작 요청. 쓸 수 없는 상태면 이유만 안내함
	void ServerRequestCall(ACTCharacter* Character, ECTPhoneQuery Query);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 통화 시간(초). 이동 불가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0.1"))
	float UseDurationSeconds = 2.f;

	// 거리 표시 단위(m)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "1"))
	float DistanceStepMeters = 10.f;

	// 결과 안내 문구와 방향 화살표 표시 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0.5"))
	float NoticeSeconds = 5.f;

	// 부스에 들어왔을 때 "F: 전화" 안내 표시 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0.5"))
	float EnterPromptSeconds = 3.f;

	// 사용 시작/종료 훅 (서버)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|PhoneBooth")
	void BP_OnUseStarted(ACTCharacter* User);

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|PhoneBooth")
	void BP_OnUseFinished(ACTCharacter* User, bool bSuccess);

private:
	void ServerTick();
	void FinishUse();
	void CancelUse();

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	TWeakObjectPtr<ACTCharacter> CurrentUser;
	ECTPhoneQuery CurrentQuery = ECTPhoneQuery::Target;
	double UseEndTime = 0.0;

	float TickAccumulator = 0.f;
};
