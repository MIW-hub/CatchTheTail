// Copyright Moon Party Games. All Rights Reserved.
#include "CTTreasureHudWidget.h"
#include "../Core/CTGameState.h"
#include "../Player/CTPlayerState.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCTTreasureHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ACTGameState* GameState = GetCTGameState();
	const ACTPlayerState* MyState = GetOwningCTPlayerState();
	if (!GameState) { return; }

	// 우리 팀 / 상대 팀 번호 (팀이 아직 없으면 0번 대 1번으로 보여줌)
	const int32 MyTeam = (MyState && MyState->GetTeamId() >= 0) ? MyState->GetTeamId() : 0;
	const int32 EnemyTeam = (MyTeam == 0) ? 1 : 0;

	// 라운드 번호와 남은 시간
	const int32 Round = GameState->GetRoundNumber();
	if (Text_RoundNumber)
	{
		Text_RoundNumber->SetText(FText::Format(NSLOCTEXT("CT", "UI_Treasure_Round", "{0} / {1}"), FMath::Max(Round, 1), TotalRounds));
	}
	if (Text_RoundTimer)
	{
		Text_RoundTimer->SetText(FormatTime(GameState->GetRoundRemainingSeconds()));
	}
	if (Round != LastRound)
	{
		LastRound = Round;
		BP_OnRoundChanged(Round);
	}

	// 라운드 승점
	const int32 MyWins = GameState->GetTeamRoundWins(MyTeam);
	const int32 EnemyWins = GameState->GetTeamRoundWins(EnemyTeam);
	if (Text_MyTeamWins) { Text_MyTeamWins->SetText(FText::AsNumber(MyWins)); }
	if (Text_EnemyTeamWins) { Text_EnemyTeamWins->SetText(FText::AsNumber(EnemyWins)); }
	if (MyWins != LastMyWins || EnemyWins != LastEnemyWins)
	{
		LastMyWins = MyWins;
		LastEnemyWins = EnemyWins;
		BP_OnRoundWinsChanged(MyWins, EnemyWins);
	}

	// 팀 색 칩
	auto TeamTint = [&](int32 TeamId)
	{
		const ECTColor Color = GameState->GetTeamColor(TeamId);
		return Color == ECTColor::None ? FallbackTeamColor : ACTPlayerState::GetLinearColorOf(Color);
	};
	if (Image_MyTeamColor) { Image_MyTeamColor->SetColorAndOpacity(TeamTint(MyTeam)); }
	if (Image_EnemyTeamColor) { Image_EnemyTeamColor->SetColorAndOpacity(TeamTint(EnemyTeam)); }

	// 에너지: 내 것만 알 수 있음 (서버가 본인에게만 보냄)
	const int32 Energy = MyState ? MyState->GetEnergy() : 0;
	if (Text_MyEnergy) { Text_MyEnergy->SetText(FText::AsNumber(Energy)); }
	if (Energy != LastEnergy)
	{
		const int32 Delta = (LastEnergy == INDEX_NONE) ? 0 : Energy - LastEnergy;
		LastEnergy = Energy;
		BP_OnMyEnergyChanged(Energy, Delta);
	}

	// 우리 팀 에너지 합계 (서버가 팀원에게만 보냄)
	if (Text_TeamEnergy) { Text_TeamEnergy->SetText(FText::AsNumber(MyState ? MyState->GetTeamEnergy() : 0)); }
}
