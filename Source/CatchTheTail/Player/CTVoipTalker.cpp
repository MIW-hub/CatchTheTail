// Copyright Moon Party Games. All Rights Reserved.
#include "CTVoipTalker.h"
#include "Components/AudioComponent.h"
#include "HAL/IConsoleManager.h"

// 받는 쪽에서 상대 목소리를 키우는 배수 (1.0 = 그대로). 엔진 기본 음성은 스팀 음성보다 작게 들려서 키움.
// 실행할 때 -ini:Engine:[SystemSettings]:ct.VoicePlaybackGain=8 처럼 바꿔 가며 알맞은 값을 찾을 수 있음
static TAutoConsoleVariable<float> CVarVoicePlaybackGain(TEXT("ct.VoicePlaybackGain"), 4.0f, TEXT("받는 목소리 크기 배수 (1.0 = 그대로)"), ECVF_Default);

static float VoiceGain(bool bMuted)
{
	return bMuted ? 0.f : FMath::Max(0.f, CVarVoicePlaybackGain.GetValueOnGameThread());
}

void UCTVoipTalker::SetMuted(bool bNewMuted)
{
	bMuted = bNewMuted;
	if (UAudioComponent* Audio = VoiceAudio.Get())
	{
		Audio->SetVolumeMultiplier(VoiceGain(bMuted));
	}
}

void UCTVoipTalker::OnTalkingBegin(UAudioComponent* AudioComponent)
{
	VoiceAudio = AudioComponent;
	if (AudioComponent)
	{
		AudioComponent->SetVolumeMultiplier(VoiceGain(bMuted));
	}
	Super::OnTalkingBegin(AudioComponent);
}

void UCTVoipTalker::OnTalkingEnd()
{
	VoiceAudio.Reset();
	Super::OnTalkingEnd();
}
