// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CTAnimNotify_Footstep.generated.h"

// 발소리 노티파이
UCLASS(meta = (DisplayName = "CT Footstep"))
class CATCHTHETAIL_API UCTAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	// 발 뼈 이름. 비우면 캐릭터 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FName FootBoneName;

	// 소리 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0"))
	float Volume = 0.55f;

	// 피치 무작위 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0.1"))
	float PitchMin = 0.94f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "0.1"))
	float PitchMax = 1.06f;
};
