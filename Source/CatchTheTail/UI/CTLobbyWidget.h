// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "CTLobbyWidget.generated.h"

class UButton;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;

/**
 * 대기 로비 화면. 이름이 맞는 위젯만 만들어 두면 자동으로 연결됩니다. (모두 선택 사항)
 */
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

	// 갱신될 때마다 호출. 추가 연출은 BP에서
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Lobby")
	void BP_OnLobbyRefreshed(bool bIsHost, bool bCanStart, float StartRemainingSeconds);

	// 플레이어 목록. 줄마다 "이름   [상태]"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PlayerList;

	// "인원 3/3 · 준비 2명" 또는 "게임 시작 3"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	// 방 코드 (스팀 연결 후 표시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_RoomCode;

	// 방 코드 보기/숨기기 버튼. 평소에는 코드를 가려 둡니다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ToggleRoomCode;

	// 위 버튼 안 글자: 방 코드 보기 / 숨기기
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ToggleRoomCodeLabel;

	// 방 코드 복사 버튼 (가려져 있어도 복사됨)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_CopyRoomCode;

	// 복사했다는 안내 문구 (잠깐 보였다 사라짐)
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

	// 채팅 로그가 들어 있는 스크롤 박스 (새 글이 오면 맨 아래로 이동)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ScrollBox_Chat;

	// 채팅 로그. 줄마다 "이름: 내용"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ChatLog;

	// 채팅 입력칸. Enter로 보내기
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EditableTextBox_Chat;

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

	// 방 코드를 화면에 보여 주는 중인가 (기본 가림)
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

	FTimerHandle RefreshTimerHandle;
};
