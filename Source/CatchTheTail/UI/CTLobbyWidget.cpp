// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyWidget.h"
#include "CTButtonHoverEffect.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTGameState.h"
#include "../Player/CTLobbyPlayerController.h"
#include "../Player/CTPlayerState.h"
#include "CTMapEntryWidget.h"
#include "../Core/CTMapSettings.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UCTLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UCTButtonHoverEffect::Attach(this);

	// 버튼 소리 (올림 / 누름)
	UCTAudioSubsystem::ApplyUISounds(this);

	if (Button_Ready) { Button_Ready->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleReadyClicked); }
	if (Button_Start) { Button_Start->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleStartClicked); }
	if (Button_ToggleRoomCode) { Button_ToggleRoomCode->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleToggleRoomCodeClicked); }
	if (Button_CopyRoomCode)   { Button_CopyRoomCode->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleCopyRoomCodeClicked); }
	if (Text_CopyResult)       { Text_CopyResult->SetVisibility(ESlateVisibility::Collapsed); }
	if (Button_Leave) { Button_Leave->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleLeaveClicked); }

	// 맵 선택
	if (Button_SelectMap)       { Button_SelectMap->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleSelectMapClicked); }
	if (Button_MapSelectClose)  { Button_MapSelectClose->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleMapSelectCloseClicked); }
	BindMatchSettingWidgets();
	SetMapSelectOpen(false);

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
	ReturnFocusToGame();
	bRoomCodeVisible = !bRoomCodeVisible;
	RefreshRoomCode();
}

void UCTLobbyWidget::HandleCopyRoomCodeClicked()
{
	ReturnFocusToGame();
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

			const FText Mark = CTPlayerState->IsHost()
				? NSLOCTEXT("CT", "Lobby_StateHost", "방장")
				: (CTPlayerState->IsReady() ? NSLOCTEXT("CT", "Lobby_StateReady", "준비") : NSLOCTEXT("CT", "Lobby_StateWaiting", "대기"));
			List += FString::Printf(TEXT("%s   [%s]\n"), *CTPlayerState->GetPlayerName(), *Mark.ToString());
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

	RefreshMap(bIsHost, bStarting);
	RefreshMatchSettings(bIsHost, bStarting);

	BP_OnLobbyRefreshed(bIsHost, bCanStart, StartRemaining);
}

// 버튼을 누른 뒤 포커스를 게임 화면으로 돌려, 이후 Space(점프)가 버튼을 다시 누르지 않게 함
void UCTLobbyWidget::ReturnFocusToGame()
{
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

// 맵 선택

void UCTLobbyWidget::RefreshMap(bool bIsHost, bool bStarting)
{
	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState) { return; }

	// 선택 버튼: 방장만, 시작이 정해지면 잠금
	if (Button_SelectMap)
	{
		Button_SelectMap->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_SelectMap->SetIsEnabled(!bStarting);
	}
	if (bMapSelectOpen && (!bIsHost || bStarting))
	{
		SetMapSelectOpen(false);
	}

	// 선택된 맵이 바뀔 때만 미리보기를 다시 그림
	const int32 SelectedIndex = CTGameState->GetSelectedMapIndex();
	if (SelectedIndex == LastShownMapIndex) { return; }
	LastShownMapIndex = SelectedIndex;

	FCTMapInfo Info;
	const bool bHasMap = UCTMapSettings::GetMapInfo(SelectedIndex, Info);

	if (Text_MapName)
	{
		Text_MapName->SetText(bHasMap ? Info.DisplayName : NSLOCTEXT("CT", "Lobby_NoMap", "맵 없음"));
	}
	if (Text_MapDescription)
	{
		Text_MapDescription->SetText(bHasMap ? Info.Description : FText::GetEmpty());
	}
	if (Image_MapPreview)
	{
		if (bHasMap && !Info.Thumbnail.IsNull())
		{
			Image_MapPreview->SetVisibility(ESlateVisibility::HitTestInvisible);
			Image_MapPreview->SetBrushFromSoftTexture(Info.Thumbnail, /*bMatchSize=*/false);
		}
		else
		{
			Image_MapPreview->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 선택 창이 열려 있으면 강조 표시도 갱신
	for (UCTMapEntryWidget* Entry : MapEntries)
	{
		if (Entry) { Entry->SetSelected(Entry->GetMapIndex() == SelectedIndex); }
	}
}

void UCTLobbyWidget::BindMatchSettingWidgets()
{
	const int32 Count = static_cast<int32>(ECTMatchSetting::Count);
	SettingArrows.Reset();
	SettingLabelTexts.Init(nullptr, Count);
	SettingValueTexts.Init(nullptr, Count);
	SettingPrevButtons.Init(nullptr, Count);
	SettingNextButtons.Init(nullptr, Count);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const ECTMatchSetting Setting = static_cast<ECTMatchSetting>(Index);
		const FString Key = FCTMatchSettings::GetKey(Setting).ToString();

		SettingLabelTexts[Index] = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("Label_Setting_%s"), *Key)));
		SettingValueTexts[Index] = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("Text_Setting_%s"), *Key)));
		SettingPrevButtons[Index] = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("Button_Setting_%s_Prev"), *Key)));
		SettingNextButtons[Index] = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("Button_Setting_%s_Next"), *Key)));

		auto Bind = [this, Setting](UButton* Button, int32 Direction)
		{
			if (!Button) { return; }
			UCTMatchSettingArrow* Arrow = NewObject<UCTMatchSettingArrow>(this);
			Arrow->Setting = Setting;
			Arrow->Direction = Direction;
			Arrow->OnArrowClicked.BindUObject(this, &UCTLobbyWidget::HandleSettingArrow);
			Button->OnClicked.AddDynamic(Arrow, &UCTMatchSettingArrow::HandleClicked);
			SettingArrows.Add(Arrow);
		};
		Bind(SettingPrevButtons[Index], -1);
		Bind(SettingNextButtons[Index], 1);
	}
}

void UCTLobbyWidget::RefreshMatchSettings(bool bIsHost, bool bStarting)
{
	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState) { return; }

	const FCTMatchSettings& Settings = CTGameState->GetMatchSettings();
	const int32 NumPlayers = CTGameState->PlayerArray.Num();

	if (Text_MatchSettings)
	{
		Text_MatchSettings->SetText(Settings.MakeSummary());
	}

	for (int32 Index = 0; Index < SettingValueTexts.Num(); ++Index)
	{
		const ECTMatchSetting Setting = static_cast<ECTMatchSetting>(Index);

		if (SettingLabelTexts[Index]) { SettingLabelTexts[Index]->SetText(FCTMatchSettings::GetLabel(Setting)); }
		if (SettingValueTexts[Index]) { SettingValueTexts[Index]->SetText(FCTMatchSettings::FormatValue(Setting, Settings.Get(Setting))); }

		// 방장만 바꿀 수 있고, 더 이동할 수 없거나 허용되지 않는 값이면 막음
		auto CanMove = [&](int32 Direction)
		{
			FCTMatchSettings Next = Settings;
			return bIsHost && !bStarting && Next.Step(Setting, Direction) && Next.IsAllowed(NumPlayers);
		};
		if (SettingPrevButtons[Index]) { SettingPrevButtons[Index]->SetIsEnabled(CanMove(-1)); }
		if (SettingNextButtons[Index]) { SettingNextButtons[Index]->SetIsEnabled(CanMove(1)); }
	}
}

void UCTLobbyWidget::HandleSettingArrow(ECTMatchSetting Setting, int32 Direction)
{
	ReturnFocusToGame();
	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->RequestChangeMatchSetting(Setting, Direction);
	}
}

void UCTLobbyWidget::SetMapSelectOpen(bool bOpen)
{
	bMapSelectOpen = bOpen;

	if (Panel_MapSelect)
	{
		Panel_MapSelect->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (bOpen)
	{
		RebuildMapEntries();
	}
}

void UCTLobbyWidget::RebuildMapEntries()
{
	if (!Panel_MapList || !MapEntryClass) { return; }

	Panel_MapList->ClearChildren();
	MapEntries.Reset();

	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	const int32 SelectedIndex = CTGameState ? CTGameState->GetSelectedMapIndex() : INDEX_NONE;

	const int32 TotalCount = GetDefault<UCTMapSettings>()->Maps.Num();
	for (int32 Index = 0; Index < TotalCount; ++Index)
	{
		FCTMapInfo Info;
		if (!UCTMapSettings::GetMapInfo(Index, Info)) { continue; }

		UCTMapEntryWidget* Entry = CreateWidget<UCTMapEntryWidget>(this, MapEntryClass);
		if (!Entry) { continue; }

		Entry->SetEntry(Index, Info, Index == SelectedIndex);
		Entry->OnEntryClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleMapEntryClicked);
		Panel_MapList->AddChild(Entry);
		MapEntries.Add(Entry);
	}
}

void UCTLobbyWidget::HandleSelectMapClicked()
{
	ReturnFocusToGame();
	SetMapSelectOpen(!bMapSelectOpen);
}

void UCTLobbyWidget::HandleMapSelectCloseClicked()
{
	ReturnFocusToGame();
	SetMapSelectOpen(false);
}

void UCTLobbyWidget::HandleMapEntryClicked(int32 MapIndex)
{
	ReturnFocusToGame();
	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->RequestSelectMap(MapIndex);
	}

	// 고르면 창을 닫음
	SetMapSelectOpen(false);
}

void UCTLobbyWidget::HandleReadyClicked()
{
	ReturnFocusToGame();
	const ACTPlayerState* MyState = GetOwningPlayerState<ACTPlayerState>();
	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->RequestSetReady(!(MyState && MyState->IsReady()));
	}
}

void UCTLobbyWidget::HandleStartClicked()
{
	ReturnFocusToGame();
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
