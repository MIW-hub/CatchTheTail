// Copyright Moon Party Games. All Rights Reserved.
#include "CTMatchSettings.h"
#include "HAL/IConsoleManager.h"

namespace
{
	// 보물찾기를 로비에서 시작할 수 있게 함. 개발 빌드(Development)는 기본 1이라 바로 시험할 수 있고, 출시 빌드(Shipping)는 이 값과 상관없이 항상 막힘 (IsModePlayable).
	// 개발 빌드에서 끄려면 -ExecCmds="ct.EnableTreasureHunt 0"
	TAutoConsoleVariable<int32> CVarEnableTreasureHunt(TEXT("ct.EnableTreasureHunt"), 1, TEXT("1이면 로비에서 보물찾기 모드를 시작할 수 있음 (개발 중)"), ECVF_Default);
}

bool FCTMatchSettings::IsModePlayable() const
{
	if (GetMode() == ECTGameModeType::TailChase) { return true; }
#if UE_BUILD_SHIPPING
	return false;
#else
	return CVarEnableTreasureHunt.GetValueOnAnyThread() != 0;
#endif
}

int32 FCTMatchSettings::Get(ECTMatchSetting Setting) const
{
	switch (Setting)
	{
	case ECTMatchSetting::GameMinutes:  return GameMinutes;
	case ECTMatchSetting::RoundSeconds: return RoundSeconds;
	case ECTMatchSetting::MaxPlayers:   return MaxPlayers;
	case ECTMatchSetting::InitialTails: return InitialTails;
	case ECTMatchSetting::PhoneUses:    return PhoneUses;
	case ECTMatchSetting::Bush:         return Bush;
	case ECTMatchSetting::AiPlayers:    return AiPlayers;
	case ECTMatchSetting::Mode:         return Mode;
	default: return 0;
	}
}

void FCTMatchSettings::Set(ECTMatchSetting Setting, int32 NewValue)
{
	switch (Setting)
	{
	case ECTMatchSetting::GameMinutes:  GameMinutes = NewValue; break;
	case ECTMatchSetting::RoundSeconds: RoundSeconds = NewValue; break;
	case ECTMatchSetting::MaxPlayers:   MaxPlayers = NewValue; break;
	case ECTMatchSetting::InitialTails: InitialTails = NewValue; break;
	case ECTMatchSetting::PhoneUses:    PhoneUses = NewValue; break;
	case ECTMatchSetting::Bush:         Bush = NewValue; break;
	case ECTMatchSetting::AiPlayers:    AiPlayers = NewValue; break;
	case ECTMatchSetting::Mode:         Mode = NewValue; break;
	default: break;
	}
}

const TArray<int32>& FCTMatchSettings::GetOptions(ECTMatchSetting Setting)
{
	static const TArray<int32> GameMinutesOptions = { 5, 6, 7, 8, 9, 10 };
	static const TArray<int32> RoundSecondsOptions = { 30, 45, 60, 90, 120 };
	static const TArray<int32> MaxPlayersOptions = { 3, 4, 5, 6, 7, 8 };
	static const TArray<int32> InitialTailsOptions = { 1, 2, 3 };
	static const TArray<int32> PhoneUsesOptions = { 0, 15, 30, 45, 60 };
	static const TArray<int32> BushOptions = { 0, 1 };
	static const TArray<int32> AiPlayersOptions = { 0, 1, 2, 3, 4, 5, 6, 7 };
	static const TArray<int32> ModeOptions = { 0, 1 };
	static const TArray<int32> None;

	switch (Setting)
	{
	case ECTMatchSetting::GameMinutes:  return GameMinutesOptions;
	case ECTMatchSetting::RoundSeconds: return RoundSecondsOptions;
	case ECTMatchSetting::MaxPlayers:   return MaxPlayersOptions;
	case ECTMatchSetting::InitialTails: return InitialTailsOptions;
	case ECTMatchSetting::PhoneUses:    return PhoneUsesOptions;
	case ECTMatchSetting::Bush:         return BushOptions;
	case ECTMatchSetting::AiPlayers:    return AiPlayersOptions;
	case ECTMatchSetting::Mode:         return ModeOptions;
	default: return None;
	}
}

void FCTMatchSettings::Sanitize()
{
	for (int32 Index = 0; Index < static_cast<int32>(ECTMatchSetting::Count); ++Index)
	{
		const ECTMatchSetting Setting = static_cast<ECTMatchSetting>(Index);
		const TArray<int32>& Options = GetOptions(Setting);
		if (Options.IsEmpty()) { continue; }

		const int32 Current = Get(Setting);
		int32 Best = Options[0];
		for (const int32 Option : Options)
		{
			if (FMath::Abs(Option - Current) < FMath::Abs(Best - Current)) { Best = Option; }
		}
		Set(Setting, Best);
	}
}

bool FCTMatchSettings::Step(ECTMatchSetting Setting, int32 Direction)
{
	const TArray<int32>& Options = GetOptions(Setting);
	if (Options.IsEmpty() || Direction == 0) { return false; }

	// 현재 값 이상인 첫 선택지 번호
	const int32 Current = Get(Setting);
	int32 Index = 0;
	for (int32 OptionIndex = 0; OptionIndex < Options.Num(); ++OptionIndex)
	{
		if (Options[OptionIndex] <= Current) { Index = OptionIndex; }
	}

	const int32 NewIndex = FMath::Clamp(Index + (Direction > 0 ? 1 : -1), 0, Options.Num() - 1);
	if (NewIndex == Index && Options[Index] == Current) { return false; }

	Set(Setting, Options[NewIndex]);
	return true;
}

bool FCTMatchSettings::CanStep(ECTMatchSetting Setting, int32 Direction) const
{
	FCTMatchSettings Copy = *this;
	return Copy.Step(Setting, Direction);
}

bool FCTMatchSettings::IsAllowed(int32 NumPlayers) const
{
	return MaxPlayers >= NumPlayers && RoundSeconds <= GameMinutes * 60;
}

FName FCTMatchSettings::GetKey(ECTMatchSetting Setting)
{
	switch (Setting)
	{
	case ECTMatchSetting::GameMinutes:  return TEXT("GameMinutes");
	case ECTMatchSetting::RoundSeconds: return TEXT("RoundSeconds");
	case ECTMatchSetting::MaxPlayers:   return TEXT("MaxPlayers");
	case ECTMatchSetting::InitialTails: return TEXT("InitialTails");
	case ECTMatchSetting::PhoneUses:    return TEXT("PhoneUses");
	case ECTMatchSetting::Bush:         return TEXT("Bush");
	case ECTMatchSetting::AiPlayers:    return TEXT("AiPlayers");
	case ECTMatchSetting::Mode:         return TEXT("Mode");
	default: return NAME_None;
	}
}

FText FCTMatchSettings::GetLabel(ECTMatchSetting Setting)
{
	switch (Setting)
	{
	case ECTMatchSetting::GameMinutes:  return NSLOCTEXT("CT", "Match_GameMinutes", "경기 시간");
	case ECTMatchSetting::RoundSeconds: return NSLOCTEXT("CT", "Match_RoundSeconds", "라운드 시간");
	case ECTMatchSetting::MaxPlayers:   return NSLOCTEXT("CT", "Match_MaxPlayers", "최대 인원");
	case ECTMatchSetting::InitialTails: return NSLOCTEXT("CT", "Match_InitialTails", "시작 꼬리 수");
	case ECTMatchSetting::PhoneUses:    return NSLOCTEXT("CT", "Match_PhoneUses", "전화 쿨타임");
	case ECTMatchSetting::Bush:         return NSLOCTEXT("CT", "Match_Bush", "부쉬");
	case ECTMatchSetting::AiPlayers:    return NSLOCTEXT("CT", "Match_AiPlayers", "AI 인원");
	case ECTMatchSetting::Mode:         return NSLOCTEXT("CT", "Match_Mode", "게임 모드");
	default: return FText::GetEmpty();
	}
}

FText FCTMatchSettings::FormatValue(ECTMatchSetting Setting, int32 Value)
{
	switch (Setting)
	{
	case ECTMatchSetting::GameMinutes:  return FText::Format(NSLOCTEXT("CT", "Match_ValueMinutes", "{0}분"), Value);
	case ECTMatchSetting::RoundSeconds: return FText::Format(NSLOCTEXT("CT", "Match_ValueSeconds", "{0}초"), Value);
	case ECTMatchSetting::MaxPlayers:   return FText::Format(NSLOCTEXT("CT", "Match_ValuePlayers", "{0}명"), Value);
	case ECTMatchSetting::InitialTails: return FText::Format(NSLOCTEXT("CT", "Match_ValueTails", "{0}개"), Value);
	case ECTMatchSetting::PhoneUses:
		return Value <= 0 ? NSLOCTEXT("CT", "Match_ValueNone", "사용 안 함")
			: FText::Format(NSLOCTEXT("CT", "Match_ValueSeconds", "{0}초"), Value);
	case ECTMatchSetting::AiPlayers:
		return Value <= 0 ? NSLOCTEXT("CT", "Match_ValueNone", "사용 안 함")
			: FText::Format(NSLOCTEXT("CT", "Match_ValuePlayers", "{0}명"), Value);
	case ECTMatchSetting::Bush:
		return Value != 0 ? NSLOCTEXT("CT", "Match_ValueOn", "켜기") : NSLOCTEXT("CT", "Match_ValueOff", "끄기");
	case ECTMatchSetting::Mode:
		return Value == static_cast<int32>(ECTGameModeType::TreasureHunt)
			? NSLOCTEXT("CT", "Match_ValueTreasure", "보물찾기 (준비 중)")
			: NSLOCTEXT("CT", "Match_ValueTailChase", "꼬리잡기");
	default: return FText::GetEmpty();
	}
}

FText FCTMatchSettings::MakeSummary() const
{
	return FText::Format(NSLOCTEXT("CT", "Match_Summary", "{0}분 · 라운드 {1}초 · 최대 {2}명"), GameMinutes, RoundSeconds, MaxPlayers);
}
