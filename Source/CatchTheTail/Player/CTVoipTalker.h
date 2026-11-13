// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Net/VoiceConfig.h"
#include "CTVoipTalker.generated.h"

class UAudioComponent;

// 한 플레이어의 목소리 처리. 기본 VOIPTalker 에 "무음" 기능을 더함 (팀 음성을 상대 팀이 못 듣게 할 때 씀)
UCLASS()
class CATCHTHETAIL_API UCTVoipTalker : public UVOIPTalker
{
	GENERATED_BODY()

public:
	// 이 목소리를 내 쪽에서 소리 0으로 만들거나 되돌림 (말하는 도중에도 바로 적용)
	void SetMuted(bool bNewMuted);

	virtual void OnTalkingBegin(UAudioComponent* AudioComponent) override;
	virtual void OnTalkingEnd() override;

private:
	bool bMuted = false;
	TWeakObjectPtr<UAudioComponent> VoiceAudio;
};
