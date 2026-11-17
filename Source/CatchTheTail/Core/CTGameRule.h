// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CTMatchSettings.h"
#include "CTGameRule.generated.h"

class ACTGameMode;
class ACTPlayerState;
class ACTCharacter;
struct FCTRoundBreakInfo;

// 게임 모드마다 달라지는 규칙을 맡는 객체 (서버에서만 존재, 복제하지 않음).
// 접속, 카운트다운, 시간 진행, AI 생성, 로비 복귀 같은 공통 흐름은 ACTGameMode 가 하고,
// 경기 시작 / 라운드 시작 / 결과 계산 / 잡기 처리처럼 모드마다 다른 부분만 여기서 처리함.
UCLASS(Abstract)
class CATCHTHETAIL_API UCTGameRule : public UObject
{
	GENERATED_BODY()

public:
	// 만든 직후 소유 GameMode 연결
	void Init(ACTGameMode* InGameMode);

	// 이 규칙이 맡은 모드
	virtual ECTGameModeType GetModeType() const PURE_VIRTUAL(UCTGameRule::GetModeType, return ECTGameModeType::TailChase;);

	// 경기 시작 때 (플레이어 초기화)
	// 서버: 게임 시작 카운트다운이 시작될 때 (플레이어를 미리 제자리에 놓을 수 있음)
	virtual void OnCountdownStart() {}
	virtual void OnMatchStart() {}

	// 라운드가 시작될 때 (라운드 시간 설정 전에 부름)
	virtual void OnRoundStart(int32 RoundNumber) {}

	// 경기가 끝났을 때 승자 계산. 동점이면 bOutTie = true 이고 승자는 공동 승자
	virtual void ComputeResult(TArray<ACTPlayerState*>& OutWinners, bool& bOutTie) {}

	// 꼬리잡기 시도 처리. 시작됐거나 성립하면 true
	virtual bool HandleGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim) { return false; }

	// 잡기가 성립했을 때 점수 변경 / 유령 처리 (대결에서 이긴 뒤나 대결이 꺼진 경우). 처리했으면 true
	virtual bool FinalizeGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim) { return false; }

	// 경기 전체 시간(초). 기본은 GameMode 의 설정값, 보물찾기는 라운드 수 x 라운드 길이
	virtual float GetGameDurationSeconds(float DefaultSeconds) const { return DefaultSeconds; }

	// 라운드 시간이 끝났을 때 (다음 라운드를 시작하기 전). 경기를 끝내야 하면 true
	virtual bool OnRoundTimeUp(int32 RoundNumber) { return false; }

	// 경기를 끝내고 결과를 계산하기 직전 (마지막 정산용)
	virtual void OnGameEnding() {}

	// 캐릭터가 에너지를 먹었을 때. 처리했으면 true (에너지는 사라짐)
	virtual bool OnEnergyPicked(ACTCharacter* Picker, int32 Amount) { return false; }

	// 던진 오브젝트가 캐릭터에 맞았을 때 (서버). Thrower 는 없을 수 있음
	virtual void OnThrowableHit(ACTCharacter* Thrower, ACTCharacter* Victim, const FVector& Velocity) {}

	// 꼬리 아이템(ACTTailPickup)을 쓰는 모드인가. 보물찾기는 쓰지 않음 (부딪히면 에너지를 떨굼)
	virtual bool UsesTailPickups() const { return true; }

	// 환경 충돌체(차, 통나무, 드럼통, 낙하물)에 맞았을 때. 규칙이 직접 처리하면 잃은 개수를 돌려줌 (UsesTailPickups 가 false 인 모드)
	virtual int32 OnHazardHit(ACTCharacter* Victim, const FVector& Direction) { return 0; }

	// 라운드 사이 쉬는 시간(초). 0 이면 쉬지 않고 바로 다음 라운드 (결과 팝업과 다음 라운드 준비에 쓰임)
	virtual float GetRoundBreakSeconds() const { return 0.f; }

	// 쉬는 시간이 시작될 때 팝업에 보여줄 내용을 채움 (라운드 정산 직후)
	virtual void FillRoundBreakInfo(FCTRoundBreakInfo& OutInfo, int32 RoundNumber) {}

	// 쉬는 시간 중에 다음 라운드를 미리 준비 (에너지, 오브젝트 재배치, 플레이어 이동). OnRoundStart 는 이미 준비된 것을 다시 하지 않음
	virtual void PrepareNextRound(int32 NextRoundNumber) {}

	// 목표 색(내가 노리는 색) 규칙을 쓰는 모드인가
	virtual bool UsesTargetColors() const { return true; }

protected:
	ACTGameMode* GetGameMode() const { return GameMode.Get(); }

private:
	TWeakObjectPtr<ACTGameMode> GameMode;
};
