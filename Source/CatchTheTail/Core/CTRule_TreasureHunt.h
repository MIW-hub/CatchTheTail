// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTGameRule.h"
#include "CTRule_TreasureHunt.generated.h"

class ACTEnergyPickup;

// 보물찾기 모드 규칙 (4:4 팀전, 3라운드). 규칙 전체는 docs/Mode_TreasureHunt_Plan.md 참고.
// 구현됨: 팀 배정과 팀 색, 라운드별 에너지 100개 배치, 에너지 수집과 꼬리 길이 연동(10개당 1개), 라운드 정산(동점이면 양 팀 승점 +1),
//         라운드마다 무작위 안전 스폰(서로 8m 이상), 잡으면 2초 멈춤(빼앗기 없음), 결과 계산(무승부 표시)
// 아직 없음: 오브젝트 던지기, 팀 음성, 로비 팀 편성 UI, 우리 팀 에너지 복제
UCLASS()
class CATCHTHETAIL_API UCTRule_TreasureHunt : public UCTGameRule
{
	GENERATED_BODY()

public:
	virtual ECTGameModeType GetModeType() const override { return ECTGameModeType::TreasureHunt; }

	virtual void OnMatchStart() override;
	virtual void OnRoundStart(int32 RoundNumber) override;
	virtual float GetGameDurationSeconds(float DefaultSeconds) const override;
	virtual bool OnRoundTimeUp(int32 RoundNumber) override;
	virtual void OnGameEnding() override;
	virtual void ComputeResult(TArray<ACTPlayerState*>& OutWinners, bool& bOutTie) override;
	virtual bool HandleGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim) override;
	virtual bool OnEnergyPicked(ACTCharacter* Picker, int32 Amount) override;
	virtual bool UsesTargetColors() const override { return false; }

private:
	// 경기 구성 (기획서 기준)
	static constexpr int32 TotalRounds = 3;
	static constexpr int32 EnergyPerRound = 100;
	static constexpr int32 EnergyPerTail = 10;
	static constexpr float GrabFreezeSeconds = 2.f;
	static constexpr float SpawnMinDistance = 800.f; // 8m

	// 플레이어 에너지와 꼬리 길이를 함께 바꿈 (꼬리 1개 = 에너지 10개)
	void SetPlayerEnergy(ACTPlayerState* Player, int32 NewEnergy) const;

	// 팀 에너지 합계 (서버 값)
	int32 GetTeamEnergy(int32 TeamId) const;

	// 이번 라운드를 정산 (한 라운드에 한 번만)
	void ScoreRound(int32 RoundNumber);

	// 라운드 시작 준비
	void ClearEnergies();
	void SpawnEnergies();
	void PlacePlayersRandomly();

	// 갈 수 있는 바닥에서 막히지 않은 임의의 위치를 하나 고름. 못 찾으면 false
	bool PickClearPoint(const FVector& Origin, float Radius, float ClearRadius, float ClearHalfHeight, FVector& OutPoint) const;

	int32 CurrentRound = 0;
	bool bRoundScored = false;
	int32 RoundWins[2] = { 0, 0 };

	// 개인이 먹은 총 에너지 (던져서 잃은 것은 빼지 않음). 결과 점수용
	TMap<TWeakObjectPtr<ACTPlayerState>, int32> CollectedTotal;
};
