// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Widgets/SWidget.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "CTGrappleComponent.generated.h"

class ACTCharacter;
class UAnimMontage;
class UAnimSequence;
class UTexture2D;

// 꼬리 잡기 대결에서의 역할
UENUM(BlueprintType)
enum class ECTGrappleRole : uint8
{
	None,
	Grabber, // 잡은 쪽 (꼬리를 뺏으려 당김)
	Victim,  // 잡힌 쪽 (뿌리치려 도망)
};

// 꼬리를 잡으면 시작되는 G 연타 줄다리기.
// 서버가 연타 횟수를 세서 게이지(0~1, 1이면 잡은 쪽 승리)를 움직이고, 한쪽이 끝까지 밀거나 시간이 끝나면 결판이 남.
// 잡은 쪽이 유리하게 시작하고, 같은 상대를 짧은 시간 안에 다시 잡으면 그 유리함이 줄어듦.
UCLASS(ClassGroup = (CT), meta = (BlueprintSpawnableComponent))
class CATCHTHETAIL_API UCTGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCTGrappleComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 서버: 잡은 쪽 컴포넌트에서 호출. 둘 중 하나라도 이미 대결 중이면 false
	bool BeginGrapple(ACTCharacter* Victim);

	// 로컬: G 입력 한 번 (서버로 전달)
	void LocalMash();

	UFUNCTION(BlueprintPure, Category = "CT|Grapple")
	bool IsGrappling() const { return Role != ECTGrappleRole::None; }

	UFUNCTION(BlueprintPure, Category = "CT|Grapple")
	ECTGrappleRole GetRole() const { return Role; }

	// 0~1, 1이면 잡은 쪽 승리
	UFUNCTION(BlueprintPure, Category = "CT|Grapple")
	float GetGauge() const { return Gauge; }

	// 내 쪽에서 본 진행도 (잡은 쪽은 게이지, 잡힌 쪽은 1 - 게이지)
	UFUNCTION(BlueprintPure, Category = "CT|Grapple")
	float GetMyProgress() const { return Role == ECTGrappleRole::Victim ? 1.f - Gauge : Gauge; }

	UFUNCTION(BlueprintPure, Category = "CT|Grapple")
	float GetRemainingSeconds() const;

protected:
	// 대결 제한 시간(초). 끝나면 게이지가 높은 쪽이 이김
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple", meta = (ClampMin = "0.5"))
	float GrappleSeconds = 3.5f;

	// 처음 잡았을 때 잡은 쪽의 시작 게이지 (0.6 = 60 : 40)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Rematch", meta = (ClampMin = "0", ClampMax = "1"))
	float StartGauge = 0.6f;

	// 같은 상대를 놓친 뒤 다시 잡을 때마다 시작 게이지에서 빼는 값 (0.05 = 60 -> 55 -> 50)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Rematch", meta = (ClampMin = "0", ClampMax = "1"))
	float RematchStartStep = 0.05f;

	// 시작 게이지의 하한 (0.5면 재도전이 거듭돼도 최소 반반)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Rematch", meta = (ClampMin = "0", ClampMax = "1"))
	float MinStartGauge = 0.5f;

	// 마지막 대결 뒤 이 시간(초)이 지나면 재도전 횟수를 잊음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Rematch", meta = (ClampMin = "0"))
	float RematchResetSeconds = 10.f;

	// F 한 번당 게이지 변화 (잡은 쪽이 조금 더 셈)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple", meta = (ClampMin = "0"))
	float GrabberGainPerMash = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple", meta = (ClampMin = "0"))
	float VictimGainPerMash = 0.04f;

	// 1초에 인정하는 최대 연타 횟수 (매크로, 과한 연타 방지)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple", meta = (ClampMin = "1"))
	int32 MaxMashPerSecond = 12;

	// AI 연타 속도 (초당, 대결마다 이 범위에서 무작위). 사람이 이기기 어려우면 낮춤
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|AI", meta = (ClampMin = "0"))
	float AIMashPerSecondMin = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|AI", meta = (ClampMin = "0"))
	float AIMashPerSecondMax = 5.f;

	// AI가 대결이 시작된 뒤 연타를 시작하기까지 걸리는 반응 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|AI", meta = (ClampMin = "0"))
	float AIMashStartDelaySeconds = 0.35f;

	// 대결 중 반복 재생할 몽타주 (비우면 재생 안 함). 반복되는 섹션이나 루프 몽타주를 권장
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Animation")
	TObjectPtr<UAnimMontage> GrabberMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Animation")
	TObjectPtr<UAnimMontage> VictimMontage;

	// 몽타주가 없을 때 대신 반복 재생할 애니메이션 (코드가 바로 몽타주로 만들어 재생). 기본값은 Pull_Tug_Pose(당기기), Run(도망, 제자리 달리기)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Animation")
	TObjectPtr<UAnimSequence> GrabberSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Animation")
	TObjectPtr<UAnimSequence> VictimSequence;

	// 위 애니메이션을 재생할 AnimBP 슬롯 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Animation")
	FName AnimSlotName = TEXT("DefaultSlot");

	// 뿌리쳤을 때 잡은 쪽이 밀려나는 속도(cm/s)와 위로 뜨는 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Escape", meta = (ClampMin = "0"))
	float EscapeKnockbackSpeed = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Escape", meta = (ClampMin = "0"))
	float EscapeKnockUpSpeed = 250.f;

	// 대결 중 이기고 있는 쪽 위치로 진 쪽이 끌려오는 속도: 1초에 둘 사이 거리의 이 비율 (0.1 = 10%)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Pull", meta = (ClampMin = "0"))
	float PullFractionPerSecond = 0.1f;

	// 끌려와도 이 거리(cm)보다 가까워지지는 않음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Pull", meta = (ClampMin = "0"))
	float MinPullDistance = 85.f;

	// 대결이 시작되면 두 캐릭터가 이 거리(cm)까지 가까이 붙음 (주먹 하나 정도 간격)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Pull", meta = (ClampMin = "0"))
	float GrappleHoldDistance = 100.f;

	// 도망치는 쪽 꼬리가 잡은 쪽 손을 향하는 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|Pull", meta = (ClampMin = "0.1"))
	float TailAimInterpSpeed = 14.f;

	// 내 화면에 G 연타 게이지를 보임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Grapple|UI")
	bool bShowGaugeUI = true;

private:
	UFUNCTION(Server, Unreliable)
	void ServerMash();

	UFUNCTION()
	void OnRep_Role();

	ACTCharacter* GetCharacter() const;
	double ServerNow() const;

	// 서버: 상태 시작/정리 (둘 다 호출해야 함)
	void SetState(ECTGrappleRole NewRole, ACTCharacter* NewPartner, float NewGauge, double NewEndTime);
	void ClearState();

	// 서버: 연타 한 번 반영
	void ApplyMash();

	// 서버: 잡은 쪽 컴포넌트에서만. 결판
	void Finish(bool bGrabberWon);

	// 역할이 바뀐 뒤 몽타주, 이동 잠금, UI 갱신 (서버와 클라이언트 공통)
	void ApplyRoleEffects();
	void ShowGaugeUI(bool bShow);

	// 역할에 맞는 몽타주(또는 애니메이션)를 재생하고 ActiveMontage 에 기록
	void StartMontage();

	// 도망치는 쪽 꼬리를 잡은 쪽 두 손의 가운데로 향하게 함 (모든 화면)
	void UpdateTailAim(float DeltaTime);

	// 서버: 이기고 있는 쪽 위치로 진 쪽을 조금씩 끌어당김
	void UpdatePullDrift(float DeltaTime);

	// 서버: 잡은 사람이 보는 방향을 기준 축으로, 둘 다 같은 방향을 보고 잡은 사람 바로 앞에 일직선으로 서게 함
	void UpdateFormation(float DeltaTime);

	// 잡은 사람 기준 축 (수평). 잡은 사람이 보는 방향. 잡히는 쪽은 이 방향으로 도망가는 자세
	FVector GrappleAxis = FVector::ForwardVector;

	bool bTailAimed = false;

	UPROPERTY(ReplicatedUsing = OnRep_Role)
	ECTGrappleRole Role = ECTGrappleRole::None;

	UPROPERTY(Replicated)
	TObjectPtr<ACTCharacter> Partner;

	UPROPERTY(Replicated)
	float Gauge = 0.5f;

	// 서버 시계 기준 종료 시각
	UPROPERTY(Replicated)
	double EndServerTime = 0.0;

	// 서버: 재도전 기억 (잡힌 쪽 컴포넌트에 기록)
	TWeakObjectPtr<ACTCharacter> LastGrabber;
	int32 RematchCount = 0;
	double LastEscapeTime = -1000.0;

	// 서버: 최근 연타 시각 (속도 제한)
	TArray<double> RecentMashTimes;

	// 서버: AI 자동 연타
	float AIMashRate = 6.f;
	float AIMashAccum = 0.f;
	float AIMashDelayLeft = 0.f;

	bool bMovementLocked = false;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveMontage;

	TSharedPtr<SWidget> GaugeWidget;

	// 게이지 UI에 쓰는 그림 (화면에 떠 있는 동안 유지)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> GaugeTextures;

	TSharedPtr<FSlateBrush> FrameBrush;
	TSharedPtr<FSlateBrush> TrackBrush;
	TSharedPtr<FSlateBrush> FillBrush;
	TSharedPtr<FSlateBrush> GrabIconBrush;
	TSharedPtr<FSlateBrush> MouseIconBrush;
	TSharedPtr<FProgressBarStyle> BarStyle;
};
