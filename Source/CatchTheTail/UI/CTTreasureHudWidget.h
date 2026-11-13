// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTTreasureHudWidget.generated.h"

class UImage;
class UTextBlock;

// 보물찾기 모드 전용 HUD. 모양(디자인)은 WBP_CTTreasureHudWidget 에서 정함.
// 아래 이름으로 위젯을 만들어 두면 자동으로 값이 채워집니다 (없는 이름은 건너뜀).
//
// 화면 위쪽: 라운드 승점, 라운드 번호, 남은 시간
//   Text_RoundNumber       "1 / 3"
//   Text_RoundTimer        "1:00"
//   Text_MyTeamWins        우리 팀 라운드 승점
//   Text_EnemyTeamWins     상대 팀 라운드 승점
//   Image_MyTeamColor      우리 팀 색 (색이 입혀짐)
//   Image_EnemyTeamColor   상대 팀 색
// 에너지 (상대 팀 에너지는 알 수 없으므로 표시하지 않음)
//   Text_MyEnergy          내 에너지
//   Text_TeamEnergy        우리 팀 에너지 (아직 서버가 보내 주지 않아 "-" 로 표시)
UCLASS(Abstract)
class CATCHTHETAIL_API UCTTreasureHudWidget : public UCTUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 내 에너지가 바뀌었을 때 (연출용). Delta 는 증감
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnMyEnergyChanged(int32 NewEnergy, int32 Delta);

	// 라운드 승점이 바뀌었을 때
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnRoundWinsChanged(int32 MyTeamWins, int32 EnemyTeamWins);

	// 라운드 번호가 바뀌었을 때
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnRoundChanged(int32 NewRoundNumber);

	// 전체 라운드 수 (표시용, "1 / 3" 의 3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI", meta = (ClampMin = "1"))
	int32 TotalRounds = 3;

	// 팀 색 칩에 입히는 색을 못 찾았을 때의 색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor FallbackTeamColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.f);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RoundNumber;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RoundTimer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MyTeamWins;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_EnemyTeamWins;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_MyTeamColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_EnemyTeamColor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MyEnergy;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TeamEnergy;

private:
	// 이전 값 (바뀔 때만 이벤트를 부르기 위함)
	int32 LastEnergy = INDEX_NONE;
	int32 LastMyWins = INDEX_NONE;
	int32 LastEnemyWins = INDEX_NONE;
	int32 LastRound = INDEX_NONE;
};
