// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/OnlineReplStructs.h"
#include "CTMatchSettings.h"
#include "CTLobbyGameMode.generated.h"

class ACTPlayerState;

// 대기 로비 게임모드
UCLASS()
class CATCHTHETAIL_API ACTLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACTLobbyGameMode();

	virtual void InitGameState() override;
	// 카운트다운 중 새 입장 거절
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// 이탈 시 시작 조건 재확인
	virtual void Logout(AController* Exiting) override;
	// Seamless Travel 플레이어는 PostLogin이 없어 여기서 초기화
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	// 서버, 준비 변경 (카운트다운 중 불가)
	void SetPlayerReady(APlayerController* Requester, bool bReady);

	// 서버, 방장 시작 요청
	bool TryStartGame(APlayerController* Requester);

	// 서버, 방장 맵 선택 (카운트다운 중 불가)
	bool SelectMap(APlayerController* Requester, int32 MapIndex);

	// 서버, 방장 경기 설정 변경 (시작 확정 후 불가)
	bool ChangeMatchSetting(APlayerController* Requester, ECTMatchSetting Setting, int32 Direction);

	// 서버, 팀 변경 요청 (보물찾기). 반대 팀에 자리가 있으면 바로 이동, 가득 찼으면 모두에게 요청 패널이 뜸
	void RequestTeamChange(APlayerController* Requester);

	// 서버, 요청 패널의 수락 (요청자가 가려는 팀의 사람만). 두 사람의 팀이 서로 바뀜
	void AcceptTeamSwap(APlayerController* Accepter, ACTPlayerState* RequesterState);

	// 서버, 채팅을 로비 전원에게 전달
	void BroadcastChat(APlayerController* Sender, const FString& Message);

protected:
	// 시작에 필요한 최소 인원 (방장 포함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby", meta = (ClampMin = "1"))
	int32 MinPlayersToStart = 3;

	// 시작 버튼 후 맵 이동까지(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby", meta = (ClampMin = "0"))
	float StartDelaySeconds = 3.f;

	// 로비에 표시할 시작 꼬리 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby", meta = (ClampMin = "0"))
	int32 LobbyTailCount = 1;

private:
	// 한 팀 최대 사람 수, 요청이 유지되는 시간(초, 다시 신청할 수 있는 간격도 같음)
	static constexpr int32 TeamCapacity = 4;
	static constexpr float TeamSwapRequestSeconds = 5.f;

	int32 CountHumansInTeam(int32 TeamId) const;
	bool IsTeamMode() const;

	// 요청 목록 정리 (만료, 이탈, 준비, 자리 생김 등). 일정 간격으로 호출
	void UpdateTeamRequests();

	// 로비에서 정해 둘 팀 색 (경기가 시작되어도 그대로 씀)
	void PrepareTeamColors();

	FTimerHandle TeamRequestTimerHandle;
	TMap<TWeakObjectPtr<APlayerState>, double> LastTeamRequestTime;

	// 공통 초기화: 방장 표시, 준비 해제, 기록 초기화
	void InitLobbyPlayer(APlayerController* NewPlayer);

	void TravelToGameMap();

	// 인원 부족 시 시작 취소
	void RecheckStartCondition();

	FTimerHandle StartTimerHandle;
	FTimerHandle RecheckTimerHandle;
};
