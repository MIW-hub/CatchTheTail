// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CTAnimTools.generated.h"

class UAnimBlueprint;
class UAnimSequence;

// 에디터 전용 도구 (파이썬으로 부름): 애니메이션 블루프린트 그래프에 상체 슬롯을 넣고, 잡기 애니메이션으로 들고 있기 / 던지기 몽타주를 만듦.
UCLASS()
class CATCHTHETAIL_API UCTAnimTools : public UObject
{
	GENERATED_BODY()

public:
	// 애니메이션 그래프의 노드와 핀 연결, 스켈레톤 뼈 이름을 글자로 돌려줌 (확인용)
	UFUNCTION(BlueprintCallable, Category = "CT|Anim")
	static FString DescribeAnimGraph(UAnimBlueprint* AnimBlueprint);

	// 걷기 포즈와 DefaultSlot 사이에 "걷기 포즈 + 상체만 UpperBody 슬롯 몽타주" 층을 넣음. 이미 있으면 건너뜀
	UFUNCTION(BlueprintCallable, Category = "CT|Anim")
	static bool AddUpperBodyLayer(UAnimBlueprint* AnimBlueprint, FName SlotName, FName BoneName, int32 BlendDepth);

	// 잡기 애니메이션(Pull_Heavy_Object)으로 AM_Hold_Object_Montage, AM_Throw_Object_Montage 를 만듦 (UpperBody 슬롯, 새 슬롯 그룹)
	UFUNCTION(BlueprintCallable, Category = "CT|Anim")
	static FString CreateHoldThrowMontages(UAnimSequence* GrabSequence, FName SlotName, FName SlotGroupName);
};
