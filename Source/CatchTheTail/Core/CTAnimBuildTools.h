// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CTAnimBuildTools.generated.h"

class UAnimMontage;
class UAnimSequence;

// 몽타주 구성 에디터 도구
UCLASS()
class CATCHTHETAIL_API UCTAnimBuildTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 몽타주 구간 재구성. 성공 시 길이(초), 실패 시 -1
	UFUNCTION(BlueprintCallable, Category = "CT|Anim")
	static float BuildMontageSegments(UAnimMontage* Montage, const TArray<UAnimSequence*>& Anims, const TArray<float>& StartTimes, const TArray<float>& EndTimes, const TArray<float>& PlayRates, const TArray<FName>& SectionNames, FName LoopSection, float BlendInTime, float BlendOutTime);
};
