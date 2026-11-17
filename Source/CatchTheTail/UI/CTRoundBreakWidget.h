// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "../Core/CTGameState.h"
#include "CTRoundBreakWidget.generated.h"

class UImage;
class UTextBlock;
class UPanelWidget;
class UCTRoundBreakRowWidget;

// 보물찾기 라운드가 끝난 뒤 7초 동안 뜨는 결과 팝업. 모양(디자인)은 WBP_CTRoundBreakWidget 에서 정함.
// 아래 이름으로 위젯을 만들어 두면 자동으로 채워집니다 (없는 이름은 건너뜀). "My" 는 내 팀, "Enemy" 는 상대 팀.
//   Text_RoundTitle                          "라운드 1 결과"
//   Text_MyTeamLabel / Text_EnemyTeamLabel   "우리 팀" / "상대 팀"
//   Image_MyTeamColor / Image_EnemyTeamColor 팀 색
//   Text_MyTeamEnergy / Text_EnemyTeamEnergy 이번 라운드 팀 에너지 합계 (0부터 올라가며 표시)
//   Text_MyTeamWins / Text_EnemyTeamWins     승점 (결과가 나오면 +1 올라감)
//   Text_MyWinPlus / Text_EnemyWinPlus       "+1" (이긴 팀에만 떠올랐다 사라짐)
//   Text_MyResult / Text_EnemyResult         "승리" / "패배" / "무승부"
//   Box_MyPlayers / Box_EnemyPlayers         개인 점수 줄이 들어갈 세로 상자 (RowWidgetClass 로 채움)
UCLASS(Abstract)
class CATCHTHETAIL_API UCTRoundBreakWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	// 내용을 채우고 연출을 처음부터 시작. MyTeam 은 내 팀 번호
	void Show(const FCTRoundBreakInfo& Info, int32 MyTeam);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 에너지 합계가 올라가기 시작할 때까지(초), 올라가는 데 걸리는 시간, 승패가 나오는 시각
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	float CountStartSeconds = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	float CountDurationSeconds = 1.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	float RevealSeconds = 2.8f;

	// "+1" 이 떠오르며 사라지는 시간과 올라가는 높이(px)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	float PlusRiseSeconds = 1.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	float PlusRiseHeight = 60.f;

	// 개인 점수 줄 모양 (WBP_CTRoundBreakRowWidget)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTRoundBreakRowWidget> RowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor FallbackTeamColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.f);

	// 연출용 블루프린트 이벤트: 팝업이 뜰 때 / 승패가 나왔을 때(어느 쪽이 이겼는지, 무승부면 둘 다 true)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnShow();

	UFUNCTION(BlueprintImplementableEvent, Category = "CT|UI")
	void BP_OnReveal(bool bMyTeamWon, bool bEnemyTeamWon);

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_RoundTitle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_MyTeamLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_EnemyTeamLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_MyTeamColor;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_EnemyTeamColor;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_MyTeamEnergy;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_EnemyTeamEnergy;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_MyTeamWins;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_EnemyTeamWins;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_MyWinPlus;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_EnemyWinPlus;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_MyResult;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_EnemyResult;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_MyPlayers;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> Box_EnemyPlayers;

private:
	void FillPlayers(UPanelWidget* Box, const FCTRoundBreakInfo& Info, int32 Team, const FLinearColor& Color, int32 MyTeam);
	void UpdatePlus(UTextBlock* Plus, bool bWon, float T);

	FCTRoundBreakInfo ShownInfo;
	int32 ShownMyTeam = 0;
	float Elapsed = 0.f;
	bool bRevealed = false;
	bool bMyWon = false;
	bool bEnemyWon = false;
};
