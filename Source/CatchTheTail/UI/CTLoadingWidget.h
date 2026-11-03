// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTLoadingWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 게임 시작 후 게임 맵으로 넘어가는 동안 보여주는 로딩 화면.
 * 화면 전체를 덮어서 그 사이에 다른 버튼이 눌리지 않게 합니다.
 * 참가 인원, 로딩 중 문구(점이 움직임), 진행 막대, 게임 설명 한 줄(열 때마다 무작위)을 보여줍니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTLoadingWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 참가 인원 (예: 참가 인원 3/8)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PlayerCount;

	// "로딩 중..." 문구
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Loading;

	// 게임 설명 한 줄
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Tip;

	// 진행 막대. 실제 진행률을 알 수 없어서 천천히 차오르는 연출
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_Loading;

	// 점이 한 칸 늘어나는 간격(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Loading", meta = (ClampMin = "0.1"))
	float DotIntervalSeconds = 0.4f;

	// 진행 막대가 차오르는 속도. 클수록 빨리 끝에 가까워짐
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Loading", meta = (ClampMin = "0.05"))
	float ProgressSpeed = 0.5f;

	// 진행 막대가 멈추는 위치 (맵이 열릴 때까지 100%는 채우지 않음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Loading", meta = (ClampMin = "0.1", ClampMax = "1"))
	float ProgressMax = 0.95f;

private:
	void RefreshPlayerCount();
	void PickRandomTip();

	float ElapsedSeconds = 0.f;
};
