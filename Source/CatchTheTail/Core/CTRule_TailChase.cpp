// Copyright Moon Party Games. All Rights Reserved.
#include "CTRule_TailChase.h"
#include "CTGameMode.h"
#include "CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTGrappleComponent.h"
#include "../World/CTTailPickup.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UCTRule_TailChase::OnMatchStart()
{
	ACTGameMode* Mode = GetGameMode();
	if (!Mode) { return; }

	// 플레이어 초기화
	for (ACTPlayerState* PlayerState : Mode->CollectPlayers())
	{
		PlayerState->SetColors(ECTColor::None, ECTColor::None); // 직전 색 없음
		PlayerState->SetTailCount(Mode->InitialTailCount);
		PlayerState->SetIsGhost(false);
		PlayerState->SetPhoneReadyServerTime(0.0); // 시작하자마자 한 번은 걸 수 있음
	}
}

void UCTRule_TailChase::OnRoundStart(int32 RoundNumber)
{
	// 0) 맵에 남은 꼬리 아이템을 모두 치움. 라운드가 끝나기 전에 일부러 기믹에 맞아 꼬리를 떨어뜨리고,
	//    새 라운드의 보충 꼬리와 함께 그 아이템까지 먹는 꼼수를 막음 (새 아이템은 스포너가 다시 만듦)
	if (UWorld* World = GetWorld())
	{
		TArray<ACTTailPickup*> LeftoverPickups;
		for (TActorIterator<ACTTailPickup> It(World); It; ++It) { LeftoverPickups.Add(*It); }
		for (ACTTailPickup* Pickup : LeftoverPickups) { Pickup->Destroy(); }
		if (LeftoverPickups.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[CT] 새 라운드 시작: 맵에 남은 꼬리 아이템 %d개 제거"), LeftoverPickups.Num());
		}
	}

	// 1) 유령 부활
	for (ACTPlayerState* PlayerState : GetGameMode()->CollectPlayers())
	{
		PlayerState->SetIsGhost(false);
	}

	// 1-2) 꼬리 부족한 플레이어에게 채움
	for (ACTPlayerState* PlayerState : GetGameMode()->CollectPlayers())
	{
		if (PlayerState->GetTailCount() < GetGameMode()->MinTailCountAtRoundStart)
		{
			UE_LOG(LogTemp, Log, TEXT("[CT] 라운드 시작 꼬리 보충: %s  %d → %d"), *PlayerState->GetPlayerName(), PlayerState->GetTailCount(), GetGameMode()->MinTailCountAtRoundStart);
			PlayerState->SetTailCount(GetGameMode()->MinTailCountAtRoundStart);
		}
	}

	// 2) 색/목표 재배정
	AssignColorsAndTargets();
}

void UCTRule_TailChase::ComputeResult(TArray<ACTPlayerState*>& OutWinners, bool& bOutTie)
{
	ACTGameMode* Mode = GetGameMode();
	if (!Mode) { return; }

	// 꼬리 최다 플레이어 찾기
	const TArray<ACTPlayerState*> Players = Mode->CollectPlayers();
	int32 MaxTailCount = -1;
	for (const ACTPlayerState* PlayerState : Players)
	{
		MaxTailCount = FMath::Max(MaxTailCount, PlayerState->GetTailCount());
	}

	OutWinners.Reset();
	for (ACTPlayerState* PlayerState : Players)
	{
		if (PlayerState->GetTailCount() == MaxTailCount)
		{
			OutWinners.Add(PlayerState);
		}
	}

	bOutTie = OutWinners.Num() > 1;
}

bool UCTRule_TailChase::HandleGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	ACTGameMode* Mode = GetGameMode();
	const ACTGameState* CTGameState = Mode ? Mode->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }
	if (!Grabber || !Victim || Grabber == Victim) { return false; }

	// 유령은 잡기/잡힘 불가
	if (Grabber->IsGhost() || Victim->IsGhost()) { return false; }

	// 목표 색만 가능
	if (Grabber->GetTargetColor() == ECTColor::None) { return false; }
	if (Victim->GetAssignedColor() != Grabber->GetTargetColor()) { return false; }

	// 꼬리 줄다리기: 둘 다 대결에 들어감. 결과는 UCTGrappleComponent 가 FinalizeTailGrabbed 로 알림
	if (Mode->bUseGrappleStruggle)
	{
		ACTCharacter* GrabberCharacter = Cast<ACTCharacter>(Grabber->GetPawn());
		ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn());
		if (GrabberCharacter && VictimCharacter && GrabberCharacter->GetGrapple() && VictimCharacter->GetGrapple())
		{
			return GrabberCharacter->GetGrapple()->BeginGrapple(VictimCharacter);
		}
	}

	return FinalizeGrab(Grabber, Victim);
}

bool UCTRule_TailChase::FinalizeGrab(ACTPlayerState* Grabber, ACTPlayerState* Victim)
{
	ACTGameMode* Mode = GetGameMode();
	const ACTGameState* CTGameState = Mode ? Mode->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing) { return false; }
	if (!Grabber || !Victim || Grabber == Victim) { return false; }
	if (Grabber->IsGhost() || Victim->IsGhost()) { return false; }
	if (Grabber->GetTargetColor() == ECTColor::None || Victim->GetAssignedColor() != Grabber->GetTargetColor()) { return false; }

	// 잡힌 쪽은 꼬리를 잃고 유령. 잡은 쪽은 빼앗은 만큼 얻음 (최소 1개)
	const int32 Taken = Mode->bTakeAllTailsOnGrab ? FMath::Max(Victim->GetTailCount(), 1) : 1;
	Grabber->AddTailCount(Taken);
	Victim->SetTailCount(FMath::Max(Victim->GetTailCount() - Taken, 0));
	Victim->SetIsGhost(true);

	// 잡힘 연출, 다른 경로도 동일 재생
	if (ACTCharacter* VictimCharacter = Cast<ACTCharacter>(Victim->GetPawn()))
	{
		VictimCharacter->MulticastPlayAction(ECTCharacterAction::Caught, true);
		VictimCharacter->MulticastPlayCatchVfx(); // 별이 팅기는 효과
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] %s 이(가) %s 의 꼬리를 잡음"), *Grabber->GetPlayerName(), *Victim->GetPlayerName());
	Mode->OnTailGrabbed(Grabber, Victim);
	return true;
}

void UCTRule_TailChase::AssignColorsAndTargets()
{
	TArray<ACTPlayerState*> Players = GetGameMode()->CollectPlayers();

	const int32 PlayerCount = Players.Num();
	const int32 AssignCount = FMath::Min(PlayerCount, CTColorCount);
	if (PlayerCount > CTColorCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 플레이어(%d명)가 색(%d개)보다 많습니다. 초과한 플레이어는 색이 없습니다."), PlayerCount, CTColorCount);
	}

	// 셔플 후 앞에서부터 배정. 직전 색 충돌 적은 배치 선택
	TArray<ECTColor> Pool;
	for (int32 Index = 0; Index < CTColorCount; ++Index)
	{
		Pool.Add(static_cast<ECTColor>(Index));
	}

	TArray<ECTColor> BestPool = Pool;
	int32 BestConflictCount = MAX_int32;

	for (int32 Attempt = 0; Attempt < 32 && BestConflictCount > 0; ++Attempt)
	{
		// Fisher-Yates
		for (int32 Index = Pool.Num() - 1; Index > 0; --Index)
		{
			Pool.Swap(Index, FMath::RandRange(0, Index));
		}

		int32 ConflictCount = 0;
		for (int32 PlayerIndex = 0; PlayerIndex < AssignCount; ++PlayerIndex)
		{
			if (Players[PlayerIndex]->GetAssignedColor() == Pool[PlayerIndex])
			{
				++ConflictCount;
			}
		}

		if (ConflictCount < BestConflictCount)
		{
			BestConflictCount = ConflictCount;
			BestPool = Pool;
		}
	}

	// 사용 중인 색 정렬 (목표 색 계산용)
	TArray<uint8> UsedColorValues;
	for (int32 PlayerIndex = 0; PlayerIndex < AssignCount; ++PlayerIndex)
	{
		UsedColorValues.Add(static_cast<uint8>(BestPool[PlayerIndex]));
	}
	UsedColorValues.Sort();

	for (int32 PlayerIndex = 0; PlayerIndex < PlayerCount; ++PlayerIndex)
	{
		ACTPlayerState* PlayerState = Players[PlayerIndex];

		if (PlayerIndex >= AssignCount)
		{
			PlayerState->SetColors(ECTColor::None, ECTColor::None);
			continue;
		}

		const ECTColor MyColor = BestPool[PlayerIndex];
		ECTColor Target = ECTColor::None;

		if (GetGameMode()->bTargetFromActiveColorsOnly)
		{
			// 혼자면 목표 없음
			if (AssignCount > 1)
			{
				const int32 MyIndex = UsedColorValues.IndexOfByKey(static_cast<uint8>(MyColor));
				const int32 NextIndex = (MyIndex + 1) % UsedColorValues.Num();
				Target = static_cast<ECTColor>(UsedColorValues[NextIndex]);
			}
		}
		else
		{
			// 색 순서 다음. 분홍 → 빨강
			Target = static_cast<ECTColor>((static_cast<int32>(MyColor) + 1) % CTColorCount);
		}

		PlayerState->SetColors(MyColor, Target);
	}
}
