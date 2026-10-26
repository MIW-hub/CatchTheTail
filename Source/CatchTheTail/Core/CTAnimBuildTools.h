// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CTAnimBuildTools.generated.h"

class UAnimMontage;
class UAnimSequence;

/**
 * 몽타주를 코드로 구성하는 에디터 도구. 에디터 Python에서 호출합니다. (게임 실행 중에는 아무 일도 하지 않음)
 */
UCLASS()
class CATCHTHETAIL_API UCTAnimBuildTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 몽타주의 첫 슬롯 트랙을 주어진 구간들로 다시 채웁니다. 구간은 순서대로 이어 붙고, 구간마다 같은 이름의 섹션이 시작 위치에 생깁니다.
	 * 재생 속도가 음수면 그 구간을 거꾸로 재생합니다 (시작/끝 시간은 원본 애니메이션 기준, 시작 < 끝).
	 * LoopSection 이 비어 있지 않으면 그 섹션이 자기 자신으로 반복되게 합니다.
	 * 성공하면 몽타주 길이(초), 실패하면 -1. 저장은 호출한 쪽에서 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|Anim")
	static float BuildMontageSegments(UAnimMontage* Montage, const TArray<UAnimSequence*>& Anims, const TArray<float>& StartTimes, const TArray<float>& EndTimes, const TArray<float>& PlayRates, const TArray<FName>& SectionNames, FName LoopSection, float BlendInTime, float BlendOutTime);
};
