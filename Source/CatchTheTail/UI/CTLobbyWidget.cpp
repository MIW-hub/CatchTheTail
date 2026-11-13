// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyWidget.h"
#include "CTButtonHoverEffect.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTGameState.h"
#include "../Player/CTLobbyPlayerController.h"
#include "../Player/CTPlayerState.h"
#include "CTMapEntryWidget.h"
#include "CTLobbyPlayerEntryWidget.h"
#include "CTTeamSwapRequestWidget.h"
#include "Components/VerticalBox.h"
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
#include "UObject/ConstructorHelpers.h"

UCTLobbyWidget::UCTLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UUserWidget> HistoryFinder(TEXT("/Game/UI/WBP_CTMatchHistoryWidget"));
	if (HistoryFinder.Succeeded())
	{
		HistoryClass = HistoryFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UCTTeamSwapRequestWidget> SwapRequestFinder(TEXT("/Game/UI/WBP_CTTeamSwapRequestWidget"));
	if (SwapRequestFinder.Succeeded())
	{
		SwapRequestClass = SwapRequestFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UCTLobbyPlayerEntryWidget> PlayerEntryFinder(TEXT("/Game/UI/WBP_CTLobbyPlayerEntryWidget"));
	if (PlayerEntryFinder.Succeeded())
	{
		PlayerEntryClass = PlayerEntryFinder.Class;
	}
}

void UCTLobbyWidget::HandleHistoryClicked()
{
	if (!HistoryClass) { return; }

	if (UUserWidget* Popup = CreateWidget<UUserWidget>(GetOwningPlayer(), HistoryClass))
	{
		Popup->AddToViewport(200);
		Popup->SetKeyboardFocus();
	}
}

void UCTLobbyWidget::HandleChangeTeamClicked()
{
	ReturnFocusToGame();
	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->RequestChangeTeam();
	}
}

void UCTLobbyWidget::RefreshSwapRequests(const ACTGameState& GameState)
{
	if (!Box_SwapRequests || !SwapRequestClass) { return; }

	TArray<ACTPlayerState*> Requesters;
	for (const FCTTeamSwapRequest& Request : GameState.GetTeamSwapRequests())
	{
		if (Request.Requester) { Requesters.Add(Request.Requester); }
	}

	bool bSame = Requesters.Num() == SwapRequestWidgets.Num();
	for (int32 Index = 0; bSame && Index < Requesters.Num(); ++Index)
	{
		bSame = SwapRequestWidgets[Index] && SwapRequestWidgets[Index]->GetRequester() == Requesters[Index];
	}
	if (bSame) { return; }

	// 요청이 생기거나 사라지면 오래된 순서로 다시 쌓음
	Box_SwapRequests->ClearChildren();
	SwapRequestWidgets.Reset();
	for (ACTPlayerState* Requester : Requesters)
	{
		UCTTeamSwapRequestWidget* Panel = CreateWidget<UCTTeamSwapRequestWidget>(this, SwapRequestClass);
		if (!Panel) { continue; }
		Panel->SetRequester(Requester);
		Box_SwapRequests->AddChild(Panel);
		SwapRequestWidgets.Add(Panel);
	}
}

void UCTLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UCTButtonHoverEffect::Attach(this);
	// 맵 카드의 맵 설정 버튼은 크기 변화 없이 색만 진해짐
	UCTButtonHoverEffect::ApplyDarkenStyle(Button_SelectMap);

	// 버튼 소리 (올림 / 누름)
	UCTAudioSubsystem::ApplyUISounds(this);

	if (Button_Ready) { Button_Ready->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleReadyClicked); }
	if (Button_Start) { Button_Start->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleStartClicked); }
	if (Button_ToggleRoomCode) { Button_ToggleRoomCode->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleToggleRoomCodeClicked); }
	if (Button_CopyRoomCode)   { Button_CopyRoomCode->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleCopyRoomCodeClicked); }
	if (Text_CopyResult)       { Text_CopyResult->SetVisibility(ESlateVisibility::Collapsed); }
	if (Button_Leave) { Button_Leave->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleLeaveClicked); }
	if (Button_History) { Button_History->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleHistoryClicked); }
	if (Button_ChangeTeam) { Button_ChangeTeam->OnClicked.AddUniqueDynamic(this, &UCTLobbyWidget::HandleChangeTeamClicked); }

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

	// 플레이어 목록 (사람은 접속 순서, 그 뒤에 게임에서 채워질 AI. AI는 항상 준비 상태)
	{
		struct FRow { FString Name; ECTLobbyPlayerKind Kind; bool bIsMe; int32 Team; };
		int32 HumanTeamCount[2] = { 0, 0 };
		TArray<FRow> Rows;
		for (const APlayerState* PlayerState : CTGameState->PlayerArray)
		{
			const ACTPlayerState* CTPlayerState = Cast<ACTPlayerState>(PlayerState);
			// 남아 있던 이전 게임의 AI 상태는 건너뜀 (아래에서 경기 설정 인원만큼 따로 표시)
			if (!CTPlayerState || CTPlayerState->IsABot()) { continue; }

			const ECTLobbyPlayerKind Kind = CTPlayerState->IsHost() ? ECTLobbyPlayerKind::Host
				: (CTPlayerState->IsReady() ? ECTLobbyPlayerKind::Ready : ECTLobbyPlayerKind::Waiting);
			const int32 Team = CTPlayerState->GetTeamId() == 1 ? 1 : 0;
			++HumanTeamCount[Team];
			Rows.Add({ CTPlayerState->GetPlayerName(), Kind, CTPlayerState == MyState, Team });
		}
		int32 TeamCounts[2] = { HumanTeamCount[0], HumanTeamCount[1] };
		const int32 AICount = CTGameState->GetLobbyAIPlayerCount();
		for (int32 Index = 0; Index < AICount; ++Index)
		{
			// AI는 시작할 때 사람이 적은 팀부터 채워짐 (보물찾기에서 들어갈 팀을 미리 보여 줌)
			const int32 AITeam = (TeamCounts[1] < TeamCounts[0]) ? 1 : 0;
			++TeamCounts[AITeam];
			Rows.Add({ FString::Printf(TEXT("AI %d"), Index + 1), ECTLobbyPlayerKind::AI, false, AITeam });
		}

		// 팀전 모드(보물찾기): 참가 목록이 두 팀 칸으로 나뉨
		const bool bTeamMode = CTGameState->GetMatchSettings().GetMode() == ECTGameModeType::TreasureHunt;
		const bool bTeamPanels = bTeamMode && Scroll_TeamA && Scroll_TeamB && PlayerEntryClass;
		if (Panel_TeamLists) { Panel_TeamLists->SetVisibility(bTeamPanels ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed); }
		if (Scroll_LobbyPlayers) { Scroll_LobbyPlayers->SetVisibility(bTeamPanels ? ESlateVisibility::Collapsed : ESlateVisibility::Visible); }
		if (bTeamPanels)
		{
			auto FillTeam = [&](UScrollBox* Scroll, TArray<TObjectPtr<UCTLobbyPlayerEntryWidget>>& Entries, int32 Team)
			{
				TArray<const FRow*> TeamRows;
				for (const FRow& Row : Rows) { if (Row.Team == Team) { TeamRows.Add(&Row); } }
				if (Entries.Num() != TeamRows.Num())
				{
					Scroll->ClearChildren();
					Entries.Reset();
					for (int32 Index = 0; Index < TeamRows.Num(); ++Index)
					{
						UCTLobbyPlayerEntryWidget* Entry = CreateWidget<UCTLobbyPlayerEntryWidget>(this, PlayerEntryClass);
						if (!Entry) { continue; }
						Scroll->AddChild(Entry);
						Entries.Add(Entry);
					}
				}
				for (int32 Index = 0; Index < Entries.Num() && Index < TeamRows.Num(); ++Index)
				{
					if (Entries[Index]) { Entries[Index]->SetEntry(TeamRows[Index]->Name, TeamRows[Index]->Kind, TeamRows[Index]->bIsMe); }
				}
			};
			FillTeam(Scroll_TeamA, TeamAEntries, 0);
			FillTeam(Scroll_TeamB, TeamBEntries, 1);

			// 머리글: 팀 색 칩과 이름 (색은 로비에서 미리 정해져 경기까지 유지됨)
			auto SetHeader = [&](UImage* Chip, UTextBlock* Name, int32 Team)
			{
				const ECTColor Color = CTGameState->GetTeamColor(Team);
				if (Chip) { Chip->SetColorAndOpacity(Color == ECTColor::None ? FLinearColor::White : ACTPlayerState::GetLinearColorOf(Color)); }
				if (Name)
				{
					Name->SetText(FText::Format(NSLOCTEXT("CT", "Lobby_TeamName", "{0}팀"),
						Color == ECTColor::None ? FText::AsNumber(Team + 1) : ACTPlayerState::GetColorDisplayName(Color)));
				}
			};
			SetHeader(Image_TeamAColor, Text_TeamAName, 0);
			SetHeader(Image_TeamBColor, Text_TeamBName, 1);
			if (Text_PlayerList) { Text_PlayerList->SetVisibility(ESlateVisibility::Collapsed); }
		}

		// 팀 변경 버튼과 요청 패널
		if (Button_ChangeTeam)
		{
			const int32 MyTeam = (MyState && MyState->GetTeamId() == 1) ? 1 : 0;
			bool bHasMyRequest = false;
			for (const FCTTeamSwapRequest& Request : CTGameState->GetTeamSwapRequests())
			{
				if (Request.Requester == MyState) { bHasMyRequest = true; }
			}
			Button_ChangeTeam->SetVisibility(bTeamPanels ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			Button_ChangeTeam->SetIsEnabled(MyState && !MyState->IsReady() && !bStarting && !bHasMyRequest);
			if (Text_ChangeTeamLabel)
			{
				// 반대 팀이 가득 찼으면 바로 이동할 수 없고 신청이 됨
				Text_ChangeTeamLabel->SetText(HumanTeamCount[1 - MyTeam] >= 4
					? NSLOCTEXT("CT", "Lobby_ChangeTeamRequest", "팀 변경 신청")
					: NSLOCTEXT("CT", "Lobby_ChangeTeam", "팀 변경"));
			}
		}
		RefreshSwapRequests(*CTGameState);

		if (!bTeamPanels && Scroll_LobbyPlayers && PlayerEntryClass)
		{
			// 줄 수가 달라질 때만 위젯을 새로 만들고, 아니면 내용만 갱신 (깜빡임 방지)
			if (PlayerEntries.Num() != Rows.Num())
			{
				Scroll_LobbyPlayers->ClearChildren();
				PlayerEntries.Reset();
				for (int32 Index = 0; Index < Rows.Num(); ++Index)
				{
					UCTLobbyPlayerEntryWidget* Entry = CreateWidget<UCTLobbyPlayerEntryWidget>(this, PlayerEntryClass);
					if (!Entry) { continue; }
					Scroll_LobbyPlayers->AddChild(Entry);
					PlayerEntries.Add(Entry);
				}
			}
			for (int32 Index = 0; Index < PlayerEntries.Num() && Index < Rows.Num(); ++Index)
			{
				if (PlayerEntries[Index]) { PlayerEntries[Index]->SetEntry(Rows[Index].Name, Rows[Index].Kind, Rows[Index].bIsMe); }
			}
			if (Text_PlayerList) { Text_PlayerList->SetVisibility(ESlateVisibility::Collapsed); }
		}
		else if (!bTeamPanels && Text_PlayerList)
		{
			// 줄 위젯이 없을 때의 예전 글자 목록
			FString List;
			for (const FRow& Row : Rows)
			{
				List += FString::Printf(TEXT("%s\n"), *Row.Name);
			}
			Text_PlayerList->SetText(FText::FromString(List));
		}

		// 참가 인원 "현재 / 최대" (사람 + AI)
		if (Text_PlayerCount)
		{
			const int32 MaxPlayers = FMath::Min(CTGameState->GetMatchSettings().MaxPlayers, CTColorCount);
			Text_PlayerCount->SetText(FText::Format(PlayerCountText, Rows.Num(), MaxPlayers));
		}
	}

	// 상태 문구: 인원/준비 안내와 패널 그림은 없앰. 시작 카운트다운일 때만 글자로 보임
	if (UWidget* StatusPill = GetWidgetFromName(TEXT("Image_StatusPill")))
	{
		StatusPill->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_Status)
	{
		if (bStarting)
		{
			Text_Status->SetText(FText::Format(NSLOCTEXT("CT", "Lobby_Starting", "게임 시작 {0}"), FMath::CeilToInt(StartRemaining)));
			Text_Status->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_Status->SetText(FText::GetEmpty());
			Text_Status->SetVisibility(ESlateVisibility::Collapsed);
		}
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
