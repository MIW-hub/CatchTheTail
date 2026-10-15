// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CTAnimNotifyState_GrabWindow.generated.h"

/**
 * 잡기 몽타주에서 "이 구간에 닿으면 잡힌다"를 표시하는 노티파이 스테이트.
 *
 * 쓰는 방법
 *  1. 잡기 몽타주(GrabMontage)를 엽니다.
 *  2. Notifies 트랙에서 우클릭 → Add Notify State → CT Grab Window
 *  3. 손을 뻗어 꼬리에 닿을 수 있는 프레임 구간에 맞춰 늘립니다.
 *
 * 동작
 *  - 구간이 시작하면 서버가 잡기 판정을 켜고, 구간이 끝나면(몽타주가 중간에 끊겨도) 끕니다.
 *  - 판정과 잡기 성공은 서버에서만 처리됩니다. 클라이언트에서는 아무 일도 하지 않습니다.
 *  - 몽타주에 이 노티파이가 없으면 캐릭터의 GrabWindowDelaySeconds / GrabActiveSeconds 타이머로 대신합니다.
 */
UCLASS(meta = (DisplayName = "CT Grab Window"))
class CATCHTHETAIL_API UCTAnimNotifyState_GrabWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
