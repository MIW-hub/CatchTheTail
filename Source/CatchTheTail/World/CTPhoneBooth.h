// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTPhoneBooth.generated.h"

class ACTCharacter;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * 전화부스. 안에 들어가 잠깐 서 있으면 내 목표까지의 거리를 대략(구간으로) 알려 줍니다. 방향은 알려 주지 않습니다.
 * - 쓰는 동안은 움직이거나 행동할 수 없어서 위험합니다 (밖에서 꼬리를 잡힐 수 있음)
 * - 사용 횟수는 플레이어마다 한 판에 정해진 만큼 (GameMode 의 PhoneUsesPerGame)
 * - 사용한 사람에게만 결과가 가고, 찾아진 목표에게는 "누군가 당신을 찾았습니다"가 감
 * - 한 번 쓰면 나갔다가 다시 들어와야 다시 쓸 수 있고, 부스는 잠깐 쉬어서 눌러앉는 것이 이득이 아님
 */
UCLASS()
class CATCHTHETAIL_API ACTPhoneBooth : public AActor
{
	GENERATED_BODY()

public:
	ACTPhoneBooth();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	// 부스 안을 감지하는 박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth")
	TObjectPtr<UBoxComponent> Trigger;

	// 보이는 부스. 블루프린트에서 메시를 바꾸세요
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 전화를 거는 데 걸리는 시간(초). 이 동안 움직일 수 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0.1"))
	float UseDurationSeconds = 2.f;

	// 한 번 쓴 뒤 이 부스가 쉬는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0"))
	float BoothCooldownSeconds = 8.f;

	// 거리 표시 단위(m). 10이면 "약 10m, 20m, 30m..."
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "1"))
	float DistanceStepMeters = 10.f;

	// 안내 문구를 띄워 두는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|PhoneBooth", meta = (ClampMin = "0.5"))
	float NoticeSeconds = 5.f;

	// 사용 시작 / 끝 연출 (소리 등을 블루프린트에서). 서버에서 호출됨
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

	// 사용 가능한 캐릭터인가 (게임 중, 유령 아님, 색 배정됨)
	static bool CanUsePhone(const ACTCharacter* Character);

	TWeakObjectPtr<ACTCharacter> CurrentUser;
	double UseEndTime = 0.0;
	double CooldownEndTime = 0.0;

	// 나갔다가 다시 들어와야 쓸 수 있는 사람 (방금 쓴 사람, 횟수 없음을 이미 안내한 사람)
	TSet<TWeakObjectPtr<ACTCharacter>> WaitForExit;

	float TickAccumulator = 0.f;
};
