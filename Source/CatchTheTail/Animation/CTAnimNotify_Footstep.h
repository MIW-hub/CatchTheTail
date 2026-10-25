// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CTAnimNotify_Footstep.generated.h"

/**
 * 발소리 노티파이. 걷기/달리기 애니메이션에서 발이 땅에 닿는 프레임에 넣으면, 그 순간에 발소리가 납니다.
 * 소리는 프로젝트 설정 CT Audio 의 Footstep 사운드를 쓰고, 위치 소리라서 가까울수록 크게 들립니다.
 */
UCLASS(meta = (DisplayName = "CT Footstep"))
class CATCHTHETAIL_API UCTAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	// 소리가 나는 발 뼈 이름. 비우면 캐릭터 위치에서 납니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FName FootBoneName;

	// 소리 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0"))
	float Volume = 0.55f;

	// 소리가 매번 똑같지 않게 높낮이를 이 범위에서 무작위로 바꿈
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0.1"))
	float PitchMin = 0.94f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0.1"))
	float PitchMax = 1.06f;
};
