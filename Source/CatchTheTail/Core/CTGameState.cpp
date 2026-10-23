// Copyright Moon Party Games. All Rights Reserved.
#include "CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "Net/UnrealNetwork.h"

ACTGameState::ACTGameState()
{
	SetNetUpdateFrequency(10.f);
}

void ACTGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACTGameState, Phase);
	DOREPLIFETIME(ACTGameState, RoundNumber);
	DOREPLIFETIME(ACTGameState, RoundEndServerTime);
	DOREPLIFETIME(ACTGameState, StartServerTime);
	DOREPLIFETIME(ACTGameState, LobbyMinPlayers);
	DOREPLIFETIME(ACTGameState, RoomCode);
	DOREPLIFETIME(ACTGameState, ReturnToLobbyServerTime);
	DOREPLIFETIME(ACTGameState, GameEndServerTime);
	DOREPLIFETIME(ACTGameState, RoundDurationSeconds);
	DOREPLIFETIME(ACTGameState, CountdownSeconds);
	DOREPLIFETIME(ACTGameState, Winners);
	DOREPLIFETIME(ACTGameState, bIsTie);
}

TArray<ACTPlayerState*> ACTGameState::GetWinners() const
{
	TArray<ACTPlayerState*> Result;
	for (ACTPlayerState* Winner : Winners)
	{
		// 클라에서 PlayerState 미도착 시 nullptr
		if (Winner)
		{
			Result.Add(Winner);
		}
	}
	return Result;
}

bool ACTGameState::IsWinner(const APlayerState* PlayerState) const
{
	return PlayerState && Winners.Contains(PlayerState);
}

void ACTGameState::SetGameResult(const TArray<ACTPlayerState*>& NewWinners, bool bNewIsTie)
{
	if (!HasAuthority()) { return; }

	Winners.Reset();
	for (ACTPlayerState* Winner : NewWinners)
	{
		Winners.Add(Winner);
	}
	bIsTie = bNewIsTie;
	ForceNetUpdate();
}

float ACTGameState::GetRoundRemainingSeconds() const
{
	if (Phase != ECTGamePhase::Playing) { return 0.f; }
	return static_cast<float>(FMath::Max(0.0, RoundEndServerTime - GetServerWorldTimeSeconds()));
}

float ACTGameState::GetGameRemainingSeconds() const
{
	if (Phase != ECTGamePhase::Playing) { return 0.f; }
	return static_cast<float>(FMath::Max(0.0, GameEndServerTime - GetServerWorldTimeSeconds()));
}

float ACTGameState::GetStartCountdownRemainingSeconds() const
{
	if (Phase != ECTGamePhase::Countdown) { return 0.f; }
	return static_cast<float>(FMath::Max(0.0, StartServerTime - GetServerWorldTimeSeconds()));
}

bool ACTGameState::IsCountdownActive() const
{
	// 시작 전 카운트다운은 남은 시간 전부
	if (Phase == ECTGamePhase::Countdown)
	{
		return GetStartCountdownRemainingSeconds() > 0.f;
	}

	// 라운드 전환 카운트다운. 종료 CountdownSeconds 전부터
	const float Remaining = GetRoundRemainingSeconds();
	return Phase == ECTGamePhase::Playing && Remaining > 0.f && Remaining <= CountdownSeconds;
}

int32 ACTGameState::GetCountdownNumber() const
{
	if (!IsCountdownActive()) { return 0; }
	return FMath::CeilToInt(Phase == ECTGamePhase::Countdown ? GetStartCountdownRemainingSeconds() : GetRoundRemainingSeconds());
}

void ACTGameState::SetPhase(ECTGamePhase NewPhase)
{
	if (!HasAuthority()) { return; }
	if (Phase == NewPhase) { return; }

	Phase = NewPhase;
	ForceNetUpdate();
	OnRep_Phase();
}

void ACTGameState::SetReturnToLobbyTime(double NewServerTime)
{
	if (!HasAuthority()) { return; }

	ReturnToLobbyServerTime = NewServerTime;
	ForceNetUpdate();
}

float ACTGameState::GetReturnToLobbyRemainingSeconds() const
{
	if (Phase != ECTGamePhase::Finished || ReturnToLobbyServerTime <= 0.0) { return 0.f; }
	return static_cast<float>(FMath::Max(0.0, ReturnToLobbyServerTime - GetServerWorldTimeSeconds()));
}

void ACTGameState::SetRoomCode(const FString& NewRoomCode)
{
	if (!HasAuthority()) { return; }

	RoomCode = NewRoomCode;
	ForceNetUpdate();
}

void ACTGameState::SetLobbyMinPlayers(int32 NewMinPlayers)
{
	if (!HasAuthority()) { return; }

	LobbyMinPlayers = FMath::Max(1, NewMinPlayers);
	ForceNetUpdate();
}

float ACTGameState::GetLobbyStartRemainingSeconds() const
{
	if (Phase != ECTGamePhase::Lobby || StartServerTime <= 0.0) { return 0.f; }
	return static_cast<float>(FMath::Max(0.0, StartServerTime - GetServerWorldTimeSeconds()));
}

int32 ACTGameState::GetLobbyReadyCount() const
{
	int32 Count = 0;
	for (const APlayerState* PlayerState : PlayerArray)
	{
		const ACTPlayerState* CTPlayerState = Cast<ACTPlayerState>(PlayerState);
		if (CTPlayerState && !CTPlayerState->IsHost() && CTPlayerState->IsReady()) { ++Count; }
	}
	return Count;
}

bool ACTGameState::CanStartLobby() const
{
	int32 PlayerCount = 0;
	for (const APlayerState* PlayerState : PlayerArray)
	{
		const ACTPlayerState* CTPlayerState = Cast<ACTPlayerState>(PlayerState);
		if (!CTPlayerState || CTPlayerState->IsOnlyASpectator()) { continue; }

		++PlayerCount;
		if (!CTPlayerState->IsHost() && !CTPlayerState->IsReady()) { return false; }
	}
	return PlayerCount >= LobbyMinPlayers;
}

void ACTGameState::SetStartCountdown(double NewStartServerTime)
{
	if (!HasAuthority()) { return; }

	StartServerTime = NewStartServerTime;
	ForceNetUpdate();
}

void ACTGameState::SetRoundInfo(int32 NewRoundNumber, double NewRoundEndServerTime)
{
	if (!HasAuthority()) { return; }

	RoundEndServerTime = NewRoundEndServerTime;
	const bool bRoundChanged = (RoundNumber != NewRoundNumber);
	RoundNumber = NewRoundNumber;
	ForceNetUpdate();

	if (bRoundChanged)
	{
		OnRep_RoundNumber();
	}
}

void ACTGameState::SetGameInfo(double NewGameEndServerTime, float NewRoundDurationSeconds, float NewCountdownSeconds)
{
	if (!HasAuthority()) { return; }

	GameEndServerTime = NewGameEndServerTime;
	RoundDurationSeconds = NewRoundDurationSeconds;
	CountdownSeconds = NewCountdownSeconds;
	ForceNetUpdate();
}

void ACTGameState::OnRep_Phase()
{
	OnPhaseChanged.Broadcast(Phase);
}

void ACTGameState::OnRep_RoundNumber()
{
	OnRoundChanged.Broadcast(RoundNumber);
}
