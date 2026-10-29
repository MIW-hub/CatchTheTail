// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "../Core/CTMatchSettings.h"
#include "CTLobbyWidget.generated.h"

class UButton;
class UCTMapEntryWidget;
class UEditableTextBox;
class UImage;
class UPanelWidget;
class UScrollBox;
class UTextBlock;
class UWidget;

// 경기 설정 < > 버튼 하나
UCLASS()
class CATCHTHETAIL_API UCTMatchSettingArrow : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE_TwoParams(FOnArrowClicked, ECTMatchSetting, int32);
	FOnArrowClicked OnArrowClicked;

	ECTMatchSetting Setting = ECTMatchSetting::GameMinutes;
	int32 Direction = 1;

	UFUNCTION()
	void HandleClicked() { OnArrowClicked.ExecuteIfBound(Setting, Direction); }
};

// 대기 로비 화면
UCLASS(Abstract)
class CATCHTHETAIL_API UCTLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 목록/버튼 즉시 갱신
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby")
	void RefreshLobby();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 갱신 후 (연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Lobby")
	void BP_OnLobbyRefreshed(bool bIsHost, bool bCanStart, float StartRemainingSeconds);

	// 플레이어 목록
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PlayerList;

	// 인원/카운트다운 문구
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	// 방 코드 (스팀 연결 후 표시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RoomCode;

	// 방 코드 보기/숨기기 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ToggleRoomCode;

	// 위 버튼 안 글자: 방 코드 보기 / 숨기기
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ToggleRoomCodeLabel;

	// 방 코드 복사 버튼 (가려져 있어도 복사됨)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_CopyRoomCode;

	// 복사 안내 문구
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CopyResult;

	// 준비 버튼 (방장에게는 숨김)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Ready;

	// 버튼 안 글자: 준비 / 준비 취소
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ReadyLabel;

	// 시작 버튼 (방장에게만 보임, 조건 충족 시 활성)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Start;

	// 나가기 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Leave;

	// 채팅 스크롤 박스
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Chat;

	// 채팅 로그
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ChatLog;

	// 채팅 입력칸
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Chat;

	// 선택된 맵 이미지 (모두에게 보임)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_MapPreview;

	// 선택된 맵 이름
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MapName;

	// 선택된 맵 설명
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MapDescription;

	// 경기 설정 요약
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MatchSettings;

	// 맵 선택 창을 여는 버튼 (방장에게만 보임)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SelectMap;

	// 맵 선택 창
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Panel_MapSelect;

	// 맵 항목이 들어갈 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Panel_MapList;

	// 맵 선택 창 닫기 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_MapSelectClose;

	// 맵 항목 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Map")
	TSubclassOf<UCTMapEntryWidget> MapEntryClass;

	// 로그에 남길 최대 줄 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Chat", meta = (ClampMin = "1"))
	int32 MaxChatLines = 50;

private:
	UFUNCTION()
	void HandleToggleRoomCodeClicked();

	UFUNCTION()
	void HandleCopyRoomCodeClicked();

	void ClearCopyResult();
	void RefreshRoomCode();

	// 방 코드 표시 중
	bool bRoomCodeVisible = false;

	FTimerHandle CopyResultTimerHandle;

	UFUNCTION()
	void HandleChatReceived(const FString& SenderName, const FString& Message);

	UFUNCTION()
	void HandleChatFocusRequested();

	UFUNCTION()
	void HandleChatCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleChatTextChanged(const FText& Text);

	void RefreshChatLog();

	TArray<FString> ChatLines;

	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleLeaveClicked();

	// 포커스를 게임 화면으로 복귀
	void ReturnFocusToGame();

	// 맵 선택
	void RefreshMap(bool bIsHost, bool bStarting);
	void SetMapSelectOpen(bool bOpen);
	void RebuildMapEntries();

	UFUNCTION()
	void HandleSelectMapClicked();

	UFUNCTION()
	void HandleMapSelectCloseClicked();

	UFUNCTION()
	void HandleMapEntryClicked(int32 MapIndex);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCTMapEntryWidget>> MapEntries;

	bool bMapSelectOpen = false;

	// 경기 설정 항목별 값 글자와 < > 버튼
	void BindMatchSettingWidgets();
	void RefreshMatchSettings(bool bIsHost, bool bStarting);
	void HandleSettingArrow(ECTMatchSetting Setting, int32 Direction);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCTMatchSettingArrow>> SettingArrows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SettingLabelTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SettingValueTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> SettingPrevButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> SettingNextButtons;

	// 마지막 미리보기 맵 번호
	int32 LastShownMapIndex = -2;

	FTimerHandle RefreshTimerHandle;
};
