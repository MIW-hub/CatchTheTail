// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTGameRule.h"
#include "CTRule_TailChase.generated.h"

// 꼬리잡기 모드 규칙. 지금은 ACTGameMode 안의 기존 꼬리잡기 코드를 그대로 부르기만 함
// (모드 구조 분리 2단계. 코드 본체를 이 클래스로 옮기는 것은 3단계)
UCLASS()
class CATCHTHETAIL_API UCTRule_TailChase : public UCTGameRule
{
	GENERATED_BODY()

public:
	virtual ECTGameModeType GetModeType() const override { return ECTGameModeType::TailChase; }

	virtual void OnMatchStart() override;
	virtual void OnRoundStart(int32 RoundNumber) override;
	virtual void ComputeResult(TArray<ACTPlayerState*>& OutWinners, bool& bOutTie) override;
	virtual bool HandleGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim) override;
	virtual bool FinalizeGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim) override;

private:
	// 라운드마다 색을 중복 없이 무작위로 배정하고 목표 색을 정함 (직전 색은 되도록 피함)
	void AssignColorsAndTargets();
};
