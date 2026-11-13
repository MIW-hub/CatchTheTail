// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTTeamSwapRequestWidget.generated.h"

class ACTPlayerState;
class UButton;
class UCTCircularProgress;
class UTextBlock;

// 로비 팀 변경 요청 패널 한 장 (요청마다 하나씩 쌓임). 모양은 WBP_CTTeamSwapRequestWidget 에서 정함.
// 이름이 같은 위젯이 있으면 자동으로 값이 채워짐 (모두 선택 사항):
//   Text_RequesterName  요청한 플레이어 이름
//   Button_Accept       팀 변경 신청(수락) 버튼. 요청자가 가려는 팀의 사람에게만 눌림 (그 외에는 비활성)
//   Text_AcceptLabel    버튼 글자
//   Progress_Circle     5초 원형 진행 표시 (UCTCircularProgress)
UCLASS(Abstract)
class CATCHTHETAIL_API UCTTeamSwapRequestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 패널이 가리키는 요청자 지정
	void SetRequester(ACTPlayerState* NewRequester);
	ACTPlayerState* GetRequester() const { return Requester; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RequesterName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Accept;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AcceptLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCTCircularProgress> Progress_Circle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText AcceptText = NSLOCTEXT("CT", "Lobby_TeamSwapAccept", "팀 변경 신청");

private:
	UFUNCTION()
	void HandleAcceptClicked();

	UPROPERTY(Transient)
	TObjectPtr<ACTPlayerState> Requester;
};
