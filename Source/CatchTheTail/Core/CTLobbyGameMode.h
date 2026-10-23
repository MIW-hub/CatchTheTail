// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/OnlineReplStructs.h"
#include "CTLobbyGameMode.generated.h"

class ACTPlayerState;

/**
 * 대기 로비 게임모드. 플레이어가 로비 맵을 돌아다니며 준비하고, 방장이 시작하면 게임 맵으로 Seamless Travel 합니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACTLobbyGameMode();

	virtual void InitGameState() override;
	// 시작 카운트다운 중에는 새 입장을 거절
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// 참가자가 나가면 시작 조건을 다시 확인
	virtual void Logout(AController* Exiting) override;
	// 게임이 끝나고 Seamless Travel로 돌아온 플레이어는 PostLogin이 호출되지 않으므로 여기서 같은 초기화
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	// 서버. 준비 상태 변경. 시작 카운트다운 중에는 변경 불가
	void SetPlayerReady(APlayerController* Requester, bool bReady);

	// 서버. 방장의 시작 요청. 조건을 만족하면 카운트다운 후 게임 맵으로 이동. 성공 시 true
	bool TryStartGame(APlayerController* Requester);

	// 서버. 채팅을 로비의 모든 플레이어에게 전달 (보낸 사람 포함)
	void BroadcastChat(APlayerController* Sender, const FString& Message);

protected:
	// 시작에 필요한 최소 인원 (방장 포함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby", meta = (ClampMin = "1"))
	int32 MinPlayersToStart = 3;

	// 시작 버튼을 누른 뒤 맵 이동까지 걸리는 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby", meta = (ClampMin = "0"))
	float StartDelaySeconds = 3.f;

	// 로비에서도 보여 줄 시작 꼬리 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby", meta = (ClampMin = "0"))
	int32 LobbyTailCount = 1;

private:
	// PostLogin / 게임에서 복귀 공통 초기화: 방장 표시, 준비 해제, 게임 기록 초기화
	void InitLobbyPlayer(APlayerController* NewPlayer);

	void TravelToGameMap();

	// 시작 카운트다운 중 인원이 줄어 조건이 깨졌으면 시작을 취소
	void RecheckStartCondition();

	FTimerHandle StartTimerHandle;
	FTimerHandle RecheckTimerHandle;
};
