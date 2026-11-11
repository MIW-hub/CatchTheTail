// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Core/CTMatchHistory.h"
#include "CTMatchHistoryWidget.generated.h"

class UButton;
class UImage;
class UScrollBox;
class UTextBlock;

// 전적 팝업의 한 판 줄. 모양은 WBP_CTMatchHistoryEntryWidget 에서 정함
UCLASS(Abstract)
class CATCHTHETAIL_API UCTMatchHistoryEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// GameNumber: 이 방에서 몇 번째 판인지
	void SetRecord(const FCTMatchRecord& Record, int32 GameNumber);

protected:
	// 판 번호 (1, 2, 3...)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Index;

	// 맵 이름
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MapName;

	// 1~3등 칸 (이름, 꼬리 수, 내 칸 강조용 배경)
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Name1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Name2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Name3;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Tail1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Tail2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Tail3;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Cell1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Cell2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Image_Cell3;

	// 내 칸 배경색
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor MyCellColor = FLinearColor(1.f, 0.78f, 0.1f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor OtherCellColor = FLinearColor::White;

};

// 로비에서 여는 전적 팝업 (지난 판들의 결과)
UCLASS(Abstract)
class CATCHTHETAIL_API UCTMatchHistoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCTMatchHistoryWidget(const FObjectInitializer& ObjectInitializer);

	// 저장된 전적으로 목록을 다시 채움
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void RefreshHistory();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTMatchHistoryEntryWidget> EntryClass;

	// 위 요약 ("총 N판 / 1등 M번")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText TotalText = NSLOCTEXT("CT", "UI_History_Total", "총 {0}판 / 1등 {1}번");

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> Scroll_History;

	// 전적이 없을 때 보이는 안내
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Empty;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Total;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

private:
	UFUNCTION()
	void HandleCloseClicked();
};
