// Copyright Moon Party Games. All Rights Reserved.
#include "CTRule_TailChase.h"
#include "CTGameMode.h"

void UCTRule_TailChase::OnMatchStart()
{
	if (ACTGameMode* Mode = GetGameMode()) { Mode->TailChase_InitPlayers(); }
}

void UCTRule_TailChase::OnRoundStart(int32 RoundNumber)
{
	if (ACTGameMode* Mode = GetGameMode()) { Mode->TailChase_BeginRound(); }
}

void UCTRule_TailChase::ComputeResult(TArray<ACTPlayerState*>& OutWinners, bool& bOutTie)
{
	if (ACTGameMode* Mode = GetGameMode()) { Mode->TailChase_ComputeResult(OutWinners, bOutTie); }
}

bool UCTRule_TailChase::HandleGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	ACTGameMode* Mode = GetGameMode();
	return Mode ? Mode->TailChase_HandleGrab(Grabber, Victim) : false;
}
