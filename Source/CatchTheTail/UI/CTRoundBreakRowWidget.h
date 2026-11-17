// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTRoundBreakRowWidget.generated.h"

class UTextBlock;
class UImage;

// 라운드 사이 결과 팝업의 플레이어 한 줄. 모양은 WBP_CTRoundBreakRowWidget 에서 정함.
//   Text_Name    플레이어 이름
//   Text_Score   이번 라운드 개인 점수(에너지)
//   Image_Color  팀 색 (선택, 색이 입혀짐)
UCLASS(Abstract)
class CATCHTHETAIL_API UCTRoundBreakRowWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	// 내용 채우기. bIsMe 이면 BP 에서 강조할 수 있게 BP_OnSetEntry 로 알림
	void SetEntry(const FString& Name, int32 Score, const FLinearColor& TeamColor, bool bIsMe);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnSetEntry(bool bIsMe);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Score;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Color;
};
