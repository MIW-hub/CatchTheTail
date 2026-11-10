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

/** 이모트 한 칸 (이모지 그림 + 눌림). 단축키 자리와 목록 둘 다 씀 */
UCLASS()
class CATCHTHETAIL_API UCTEmoteCellWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	// 칸 크기, 번호, 위 왼쪽에 적을 글자(키 번호)를 정함. 한 번만 호출
	void Build(int32 InIndex, float Size, const FText& CornerText);

	// 안에 넣을 그림. UV로 아틀라스의 한 칸만 잘라 씀
	void SetIcon(UTexture2D* Texture, FVector2D UVMin, FVector2D UVMax);

	// 고른 칸 표시 (분홍 테두리)
	void SetSelected(bool bSelected);

	FCTEmoteCellClickedSignature OnCellClicked;

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image_Back;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button_Cell;

	int32 Index = 0;
};

/**
 * 이모트 고르는 창 (R 키). 위에는 숫자 키 자리, 아래에는 전체 이모트 목록.
 * 자리를 누르고 아래 이모트를 누르면 그 자리에 등록됨. 코드로 만들어서 WBP가 필요 없음.
 */
UCLASS()
class CATCHTHETAIL_API UCTEmoteSelectWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	// 캐릭터의 이모트 목록으로 창을 채움 (CreateWidget 직후 한 번)
	void Setup(ACTCharacter* InCharacter);

	// 닫기 요청 (R, ESC, 닫기 버튼)
	FCTEmoteMenuClosedSignature OnCloseRequested;

protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

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

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text_Selected;

	// 지금 고른 숫자 키 자리 (0부터)
	int32 SelectedSlot = 0;
};
