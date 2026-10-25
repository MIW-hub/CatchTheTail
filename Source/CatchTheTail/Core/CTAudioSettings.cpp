// Copyright Moon Party Games. All Rights Reserved.
#include "CTAudioSettings.h"
#include "Sound/SoundBase.h"

namespace
{
	TSoftObjectPtr<USoundBase> SoundPath(const TCHAR* Name)
	{
		return TSoftObjectPtr<USoundBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/sound/%s.%s"), Name, Name)));
	}
}

UCTAudioSettings::UCTAudioSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("CT Audio");

	// 기본값: Content/sound 의 파일 이름 기준
	MenuMusic     = SoundPath(TEXT("BGM_Sky_Breeze_132BPM_Loop"));
	GameMusic     = SoundPath(TEXT("BGM_Bouncy_Battery_128BPM_Loop"));

	UIHover       = SoundPath(TEXT("SFX_UI_Hover"));
	UIConfirm     = SoundPath(TEXT("SFX_UI_Confirm"));
	UIBack        = SoundPath(TEXT("SFX_UI_Back"));
	CountdownTick = SoundPath(TEXT("SFX_Countdown_Tick"));
	RoundStart    = SoundPath(TEXT("SFX_Round_Start"));
	TailGained    = SoundPath(TEXT("SFX_Battery_Collect"));
	TailLost      = SoundPath(TEXT("SFX_Tail_Lost"));
	Push          = SoundPath(TEXT("SFX_Push_Boing"));
	GhostRespawn  = SoundPath(TEXT("SFX_Ghost_Respawn"));
	Victory       = SoundPath(TEXT("SFX_Victory"));
	Footstep      = SoundPath(TEXT("SFX_Footstep"));
	Jump          = SoundPath(TEXT("SFX_Jump"));
	Land          = SoundPath(TEXT("SFX_Land"));
}

USoundBase* UCTAudioSettings::LoadSfx(ECTSfx Sfx) const
{
	switch (Sfx)
	{
	case ECTSfx::UIHover:       return UIHover.LoadSynchronous();
	case ECTSfx::UIConfirm:     return UIConfirm.LoadSynchronous();
	case ECTSfx::UIBack:        return UIBack.LoadSynchronous();
	case ECTSfx::CountdownTick: return CountdownTick.LoadSynchronous();
	case ECTSfx::RoundStart:    return RoundStart.LoadSynchronous();
	case ECTSfx::TailGained:    return TailGained.LoadSynchronous();
	case ECTSfx::TailLost:      return TailLost.LoadSynchronous();
	case ECTSfx::Push:          return Push.LoadSynchronous();
	case ECTSfx::GhostRespawn:  return GhostRespawn.LoadSynchronous();
	case ECTSfx::Victory:       return Victory.LoadSynchronous();
	case ECTSfx::Footstep:      return Footstep.LoadSynchronous();
	case ECTSfx::Jump:          return Jump.LoadSynchronous();
	case ECTSfx::Land:          return Land.LoadSynchronous();
	}
	return nullptr;
}

USoundBase* UCTAudioSettings::LoadMusic(ECTMusic Music) const
{
	switch (Music)
	{
	case ECTMusic::Menu: return MenuMusic.LoadSynchronous();
	case ECTMusic::Game: return GameMusic.LoadSynchronous();
	default:             return nullptr;
	}
}
