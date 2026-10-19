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
		// 클라이언트에서는 아직 도착하지 않은 PlayerState가 nullptr일 수 있습니다.
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
	// 경기 시작 전 카운트다운: 남은 시간 전체가 카운트다운입니다.
	if (Phase == ECTGamePhase::Countdown)
	{
		return GetStartCountdownRemainingSeconds() > 0.f;
	}

	// 경기 중 라운드 전환 카운트다운: 끝나기 CountdownSeconds 전부터
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
