// Copyright Moon Party Games. All Rights Reserved.
#include "CTGameState.h"
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
	DOREPLIFETIME(ACTGameState, GameEndServerTime);
	DOREPLIFETIME(ACTGameState, RoundDurationSeconds);
	DOREPLIFETIME(ACTGameState, CountdownSeconds);
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

bool ACTGameState::IsCountdownActive() const
{
	const float Remaining = GetRoundRemainingSeconds();
	return Phase == ECTGamePhase::Playing && Remaining > 0.f && Remaining <= CountdownSeconds;
}

int32 ACTGameState::GetCountdownNumber() const
{
	return IsCountdownActive() ? FMath::CeilToInt(GetRoundRemainingSeconds()) : 0;
}

void ACTGameState::SetPhase(ECTGamePhase NewPhase)
{
	if (!HasAuthority()) { return; }
	if (Phase == NewPhase) { return; }

	Phase = NewPhase;
	ForceNetUpdate();
	OnRep_Phase();
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
