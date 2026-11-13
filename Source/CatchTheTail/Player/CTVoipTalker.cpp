// Copyright Moon Party Games. All Rights Reserved.
#include "CTVoipTalker.h"
#include "Components/AudioComponent.h"

void UCTVoipTalker::SetMuted(bool bNewMuted)
{
	bMuted = bNewMuted;
	if (UAudioComponent* Audio = VoiceAudio.Get())
	{
		Audio->SetVolumeMultiplier(bMuted ? 0.f : 1.f);
	}
}

void UCTVoipTalker::OnTalkingBegin(UAudioComponent* AudioComponent)
{
	VoiceAudio = AudioComponent;
	if (AudioComponent)
	{
		AudioComponent->SetVolumeMultiplier(bMuted ? 0.f : 1.f);
	}
	Super::OnTalkingBegin(AudioComponent);
}

void UCTVoipTalker::OnTalkingEnd()
{
	VoiceAudio.Reset();
	Super::OnTalkingEnd();
}
