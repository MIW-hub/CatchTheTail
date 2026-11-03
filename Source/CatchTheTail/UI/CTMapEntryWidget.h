// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Core/CTMapSettings.h"
#include "CTMapEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UWidget;

// 항목을 눌렀을 때 (맵 번호)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTMapEntryClickedSignature, int32, MapIndex);

// 맵 선택 창 항목 한 칸
UCLASS(Abstract)
class CATCHTHETAIL_API UCTMapEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 항목 채우기
	void SetEntry(int32 InMapIndex, const FCTMapInfo& Info, bool bSelected);

	// 선택 표시만 바꿈
	void SetSelected(bool bSelected);

	int32 GetMapIndex() const { return MapIndex; }

	UPROPERTY(BlueprintAssignable, Category = "CT|Lobby|Map")
	FCTMapEntryClickedSignature OnEntryClicked;

protected:
	virtual void NativeConstruct() override;

	// 선택 상태가 바뀔 때 (선택 연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Lobby|Map")
	void BP_OnSelectionChanged(bool bSelected);

	// 항목 전체를 덮는 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Entry;

	// 맵 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Thumbnail;

	// 맵 이름
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MapName;

	// 맵 설명
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MapDescription;

	// 선택 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Widget_Selected;

private:
	UFUNCTION()
	void HandleClicked();

	int32 MapIndex = INDEX_NONE;
};
