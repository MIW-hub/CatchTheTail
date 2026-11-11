// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTEmoteSelectWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UUniformGridPanel;
class UHorizontalBox;
class ACTCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTEmoteCellClickedSignature, int32, Index);
DECLARE_MULTICAST_DELEGATE(FCTEmoteMenuClosedSignature);

/**
 * 이모트 한 칸. 모양은 WBP_CTEmoteCellWidget 에서 꾸밈. 코드는 그림 채우기, 눌림, 선택 표시만 함.
 * WBP 에 이름이 맞는 위젯을 두면 연결됨: Image_Icon(이모지 그림), Button_Cell(눌림), Widget_Selected(고른 칸일 때만 보임), Text_Key(키 번호).
 */
UCLASS()
class CATCHTHETAIL_API UCTEmoteCellWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	// 칸 번호와 키 번호 글자를 정함
	void Init(int32 InIndex, const FText& KeyText);

	// 안에 넣을 그림. UV로 아틀라스의 한 칸만 잘라 씀
	void SetIcon(UTexture2D* Texture, FVector2D UVMin, FVector2D UVMax);

	// 고른 칸 표시
	void SetSelected(bool bSelected);

	FCTEmoteCellClickedSignature OnCellClicked;

protected:
	virtual void NativeOnInitialized() override;

	// 선택 상태가 바뀔 때 (WBP 에서 연출용으로 씀)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Emote")
	void BP_OnSelectedChanged(bool bSelected);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Cell;

	// 고른 칸일 때만 보이는 표시 (테두리 그림 등)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_Selected;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Key;

private:
	UFUNCTION()
	void HandleClicked();

	int32 Index = 0;
};

/**
 * 이모트 고르는 창 (R 키). 모양은 WBP_CTEmoteSelectWidget 에서 꾸밈.
 * WBP 에 Row_EmoteSlots(숫자 키 자리가 들어갈 가로 상자), Grid_EmoteLibrary(전체 목록이 들어갈 격자),
 * Button_Close(닫기)를 두면 코드가 칸을 만들어 채움. 칸 모양은 EmoteCellClass(WBP_CTEmoteCellWidget).
 */
UCLASS()
class CATCHTHETAIL_API UCTEmoteSelectWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	UCTEmoteSelectWidget(const FObjectInitializer& ObjectInitializer);

	// 캐릭터의 이모트 목록으로 창을 채움 (CreateWidget 직후 한 번)
	void Setup(ACTCharacter* InCharacter);

	// 닫기 요청 (R, ESC, 닫기 버튼)
	FCTEmoteMenuClosedSignature OnCloseRequested;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 한 칸의 모양 (WBP_CTEmoteCellWidget)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Emote")
	TSubclassOf<UCTEmoteCellWidget> EmoteCellClass;

	// 한 줄에 놓는 목록 칸 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Emote", meta = (ClampMin = "1"))
	int32 LibraryColumns = 9;

	// 숫자 키 자리가 들어갈 가로 상자
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> Row_EmoteSlots;

	// 전체 이모트 목록이 들어갈 격자
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> Grid_EmoteLibrary;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

private:
	UFUNCTION()
	void HandleSlotClicked(int32 Index);

	UFUNCTION()
	void HandleLibraryClicked(int32 Index);

	UFUNCTION()
	void HandleCloseClicked();

	void RefreshSlots();
	void RefreshSelection();

	UPROPERTY(Transient)
	TObjectPtr<ACTCharacter> Character;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCTEmoteCellWidget>> SlotCells;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCTEmoteCellWidget>> LibraryCells;

	// 지금 고른 숫자 키 자리 (0부터)
	int32 SelectedSlot = 0;
};
