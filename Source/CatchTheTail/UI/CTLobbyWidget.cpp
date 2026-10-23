// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyWidget.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTGameState.h"
#include "../Player/CTLobbyPlayerController.h"
#include "../Player/CTPlayerState.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UCTLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Ready) { Button_Ready->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleReadyClicked); }
	if (Button_Start) { Button_Start->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleStartClicked); }
	if (Button_ToggleRoomCode) { Button_ToggleRoomCode->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleToggleRoomCodeClicked); }
	if (Button_CopyRoomCode)   { Button_CopyRoomCode->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleCopyRoomCodeClicked); }
	if (Text_CopyResult)       { Text_CopyResult->SetVisibility(ESlateVisibility::Collapsed); }
	if (Button_Leave) { Button_Leave->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleLeaveClicked); }

	// 채팅
	if (EditableTextBox_Chat)
	{
		EditableTextBox_Chat->OnTextCommitted.AddUniqueDynamic(this, &UCTLobbyWidget::HandleChatCommitted);
		EditableTextBox_Chat->OnTextChanged.AddUniqueDynamic(this, &UCTLobbyWidget::HandleChatTextChanged);
	}
	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->OnChatReceived.AddUniqueDynamic(this, &UCTLobbyWidget::HandleChatReceived);
		Controller->OnChatFocusRequested.AddUniqueDynamic(this, &UCTLobbyWidget::HandleChatFocusRequested);
	}
	RefreshChatLog();

	// 접속/준비/카운트다운 변화를 짧은 주기로 따라감
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UCTLobbyWidget::RefreshLobby, 0.2f, true);
	}
	RefreshLobby();
}

void UCTLobbyWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
		World->GetTimerManager().ClearTimer(CopyResultTimerHandle);
	}

	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->OnChatReceived.RemoveDynamic(this, &UCTLobbyWidget::HandleChatReceived);
		Controller->OnChatFocusRequested.RemoveDynamic(this, &UCTLobbyWidget::HandleChatFocusRequested);
	}

	Super::NativeDestruct();
}

// 방 코드

void UCTLobbyWidget::RefreshRoomCode()
{
	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	const FString Code = CTGameState ? CTGameState->GetRoomCode() : FString();
	const bool bHasCode = !Code.IsEmpty();

	if (Text_RoomCode)
	{
		// 가려진 동안은 글자 수만큼 점으로 표시
		const FString Masked = FString::ChrN(Code.Len(), TEXT('\x2022'));
		Text_RoomCode->SetText(bHasCode ? FText::FromString(bRoomCodeVisible ? Code : Masked) : NSLOCTEXT("CT", "Lobby_NoRoomCode", "-"));
	}
	if (Text_ToggleRoomCodeLabel)
	{
		Text_ToggleRoomCodeLabel->SetText(bRoomCodeVisible
			? NSLOCTEXT("CT", "Lobby_HideRoomCode", "방 코드 숨기기")
			: NSLOCTEXT("CT", "Lobby_ShowRoomCode", "방 코드 보기"));
	}
	if (Button_ToggleRoomCode) { Button_ToggleRoomCode->SetIsEnabled(bHasCode); }
	if (Button_CopyRoomCode)   { Button_CopyRoomCode->SetIsEnabled(bHasCode); }
}

void UCTLobbyWidget::HandleToggleRoomCodeClicked()
{
	bRoomCodeVisible = !bRoomCodeVisible;
	RefreshRoomCode();
}

void UCTLobbyWidget::HandleCopyRoomCodeClicked()
{
	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	const FString Code = CTGameState ? CTGameState->GetRoomCode() : FString();
	if (Code.IsEmpty()) { return; }

	FPlatformApplicationMisc::ClipboardCopy(*Code);

	// 복사됐다는 안내를 잠깐 보여 줌
	if (Text_CopyResult)
	{
		Text_CopyResult->SetText(NSLOCTEXT("CT", "Lobby_CodeCopied", "복사되었습니다"));
		Text_CopyResult->SetVisibility(ESlateVisibility::HitTestInvisible);
		GetWorld()->GetTimerManager().SetTimer(CopyResultTimerHandle, this, &UCTLobbyWidget::ClearCopyResult, 1.5f, false);
	}
}

void UCTLobbyWidget::ClearCopyResult()
{
	if (Text_CopyResult)
	{
		Text_CopyResult->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// 채팅

void UCTLobbyWidget::HandleChatReceived(const FString& SenderName, const FString& Message)
{
	ChatLines.Add(FString::Printf(TEXT("%s: %s"), *SenderName, *Message));
	while (ChatLines.Num() > MaxChatLines)
	{
		ChatLines.RemoveAt(0);
	}
	RefreshChatLog();
}

void UCTLobbyWidget::RefreshChatLog()
{
	if (Text_ChatLog)
	{
		Text_ChatLog->SetText(FText::FromString(FString::Join(ChatLines, TEXT("\n"))));
	}
	if (ScrollBox_Chat)
	{
		ScrollBox_Chat->ScrollToEnd();
	}
}

void UCTLobbyWidget::HandleChatFocusRequested()
{
	if (EditableTextBox_Chat)
	{
		EditableTextBox_Chat->SetKeyboardFocus();
	}
}

void UCTLobbyWidget::HandleChatTextChanged(const FText& Text)
{
	// 글자 수 제한
	ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>();
	const int32 MaxLength = Controller ? Controller->GetMaxChatLength() : 100;
	if (EditableTextBox_Chat && Text.ToString().Len() > MaxLength)
	{
		EditableTextBox_Chat->SetText(FText::FromString(Text.ToString().Left(MaxLength)));
	}
}

void UCTLobbyWidget::HandleChatCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	// Enter일 때만 전송. 다른 곳 클릭/Esc는 무시
	if (CommitMethod != ETextCommit::OnEnter) { return; }

	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->SendChat(Text.ToString());
	}

	if (EditableTextBox_Chat)
	{
		EditableTextBox_Chat->SetText(FText::GetEmpty());
	}

	// 입력칸에서 빠져나와 다시 캐릭터를 움직일 수 있게 함
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

void UCTLobbyWidget::RefreshLobby()
{
	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState) { return; }

	const ACTPlayerState* MyState = GetOwningPlayerState<ACTPlayerState>();
	const bool bIsHost = MyState && MyState->IsHost();
	const bool bCanStart = CTGameState->CanStartLobby();
	const float StartRemaining = CTGameState->GetLobbyStartRemainingSeconds();
	const bool bStarting = StartRemaining > 0.f;

	// 플레이어 목록
	if (Text_PlayerList)
	{
		FString List;
		for (const APlayerState* PlayerState : CTGameState->PlayerArray)
		{
			const ACTPlayerState* CTPlayerState = Cast<ACTPlayerState>(PlayerState);
			if (!CTPlayerState) { continue; }

			const TCHAR* Mark = CTPlayerState->IsHost() ? TEXT("방장") : (CTPlayerState->IsReady() ? TEXT("준비") : TEXT("대기"));
			List += FString::Printf(TEXT("%s   [%s]\n"), *CTPlayerState->GetPlayerName(), Mark);
		}
		Text_PlayerList->SetText(FText::FromString(List));
	}

	// 상태 문구
	if (Text_Status)
	{
		FText Status;
		if (bStarting)
		{
			Status = FText::Format(NSLOCTEXT("CT", "Lobby_Starting", "게임 시작 {0}"), FMath::CeilToInt(StartRemaining));
		}
		else
		{
			Status = FText::Format(
				NSLOCTEXT("CT", "Lobby_Status", "인원 {0}/{1} · 준비 {2}명"),
				CTGameState->PlayerArray.Num(), CTGameState->GetLobbyMinPlayers(), CTGameState->GetLobbyReadyCount());
		}
		Text_Status->SetText(Status);
	}

	// 준비 버튼: 방장에게는 필요 없음
	if (Button_Ready)
	{
		Button_Ready->SetVisibility(bIsHost ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		Button_Ready->SetIsEnabled(!bStarting);
	}
	if (Text_ReadyLabel)
	{
		Text_ReadyLabel->SetText((MyState && MyState->IsReady())
			? NSLOCTEXT("CT", "Lobby_CancelReady", "준비 취소")
			: NSLOCTEXT("CT", "Lobby_Ready", "준비"));
	}

	// 시작 버튼: 방장만, 조건을 만족할 때만 활성
	if (Button_Start)
	{
		Button_Start->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_Start->SetIsEnabled(bIsHost && bCanStart && !bStarting);
	}

	RefreshRoomCode();

	BP_OnLobbyRefreshed(bIsHost, bCanStart, StartRemaining);
}

void UCTLobbyWidget::HandleReadyClicked()
{
	const ACTPlayerState* MyState = GetOwningPlayerState<ACTPlayerState>();
	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->RequestSetReady(!(MyState && MyState->IsReady()));
	}
}

void UCTLobbyWidget::HandleStartClicked()
{
	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->RequestStartGame();
	}
}

void UCTLobbyWidget::HandleLeaveClicked()
{
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->LeaveToMainMenu(FText::GetEmpty());
	}
}
