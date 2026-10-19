// Copyright Moon Party Games. All Rights Reserved.
#include "CTHUDWidget.h"
#include "CTScoreEntryWidget.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"

namespace
{
	void SetShown(UWidget* Widget, bool bShown)
	{
		if (Widget)
		{
			Widget->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UCTHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetShown(Widget_Countdown, false);
	SetShown(Widget_Ghost, false);

	TryBindGameState();
	TryBindMyPlayerState();
	RefreshMyState();
}

void UCTHUDWidget::NativeDestruct()
{
	UnbindAll();
	Super::NativeDestruct();
}

void UCTHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TryBindGameState();
	TryBindMyPlayerState();

	UpdateTimeDisplay();
	UpdateActionGauges();

	ScoreboardSyncAccumulator += InDeltaTime;
	if (ScoreboardSyncAccumulator >= ScoreboardSyncInterval)
	{
		ScoreboardSyncAccumulator = 0.f;
		SyncScoreboardPlayers();
	}

	if (bScoreboardDirty)
	{
		RefreshScoreboard();
	}
}

// ============================================================
// 연결
// ============================================================

void UCTHUDWidget::TryBindGameState()
{
	if (BoundGameState.IsValid()) { return; }

	ACTGameState* GameState = GetCTGameState();
	if (!GameState) { return; }

	BoundGameState = GameState;
	GameState->OnPhaseChanged.AddUniqueDynamic(this, &UCTHUDWidget::HandlePhaseChanged);
	GameState->OnRoundChanged.AddUniqueDynamic(this, &UCTHUDWidget::HandleRoundChanged);

	// 연결하기 전에 이미 바뀐 값도 화면에 반영합니다.
	HandlePhaseChanged(GameState->GetPhase());
	bScoreboardDirty = true;
}

void UCTHUDWidget::TryBindMyPlayerState()
{
	ACTPlayerState* MyPlayerState = GetOwningCTPlayerState();
	if (MyPlayerState == BoundMyPlayerState.Get()) { return; }

	if (ACTPlayerState* OldPlayerState = BoundMyPlayerState.Get())
	{
		OldPlayerState->OnPlayerStateChanged.RemoveDynamic(this, &UCTHUDWidget::HandleMyPlayerStateChanged);
	}

	BoundMyPlayerState = MyPlayerState;
	LastTailCount = INDEX_NONE;

	if (MyPlayerState)
	{
		MyPlayerState->OnPlayerStateChanged.AddUniqueDynamic(this, &UCTHUDWidget::HandleMyPlayerStateChanged);
	}

	RefreshMyState();
	bScoreboardDirty = true;
}

void UCTHUDWidget::UnbindAll()
{
	if (ACTGameState* GameState = BoundGameState.Get())
	{
		GameState->OnPhaseChanged.RemoveDynamic(this, &UCTHUDWidget::HandlePhaseChanged);
		GameState->OnRoundChanged.RemoveDynamic(this, &UCTHUDWidget::HandleRoundChanged);
	}
	BoundGameState.Reset();

	if (ACTPlayerState* MyPlayerState = BoundMyPlayerState.Get())
	{
		MyPlayerState->OnPlayerStateChanged.RemoveDynamic(this, &UCTHUDWidget::HandleMyPlayerStateChanged);
	}
	BoundMyPlayerState.Reset();

	for (const TWeakObjectPtr<ACTPlayerState>& PlayerState : BoundScorePlayerStates)
	{
		if (PlayerState.IsValid())
		{
			PlayerState->OnPlayerStateChanged.RemoveDynamic(this, &UCTHUDWidget::HandleAnyPlayerStateChanged);
		}
	}
	BoundScorePlayerStates.Reset();
}

void UCTHUDWidget::HandlePhaseChanged(ECTGamePhase NewPhase)
{
	if (Text_Phase)
	{
		switch (NewPhase)
		{
		case ECTGamePhase::WaitingForPlayers:
			Text_Phase->SetText(NSLOCTEXT("CT", "UI_Phase_Waiting", "플레이어를 기다리는 중"));
			break;
		case ECTGamePhase::Playing:
			Text_Phase->SetText(FText::GetEmpty());
			break;
		case ECTGamePhase::Finished:
			Text_Phase->SetText(NSLOCTEXT("CT", "UI_Phase_Finished", "경기 종료"));
			break;
		case ECTGamePhase::Countdown:
			Text_Phase->SetText(NSLOCTEXT("CT", "UI_Phase_Countdown", "곧 경기가 시작됩니다"));
			break;
		}
	}

	LastCountdownNumber = -1;
	bScoreboardDirty = true;
	BP_OnPhaseChanged(NewPhase);
}

void UCTHUDWidget::HandleRoundChanged(int32 NewRoundNumber)
{
	bScoreboardDirty = true;
	BP_OnRoundChanged(NewRoundNumber);
}

void UCTHUDWidget::HandleMyPlayerStateChanged(ACTPlayerState* ChangedPlayerState)
{
	RefreshMyState();
	bScoreboardDirty = true;
}

void UCTHUDWidget::HandleAnyPlayerStateChanged(ACTPlayerState* ChangedPlayerState)
{
	bScoreboardDirty = true;
}

// ============================================================
// 갱신
// ============================================================

void UCTHUDWidget::UpdateTimeDisplay()
{
	const ACTGameState* GameState = BoundGameState.Get();
	const bool bPlaying = GameState && GameState->GetPhase() == ECTGamePhase::Playing;

	if (Text_GameTime)
	{
		Text_GameTime->SetText(bPlaying ? FormatTime(GameState->GetGameRemainingSeconds()) : FText::FromString(TEXT("-:--")));
	}
	if (Text_RoundTime)
	{
		Text_RoundTime->SetText(bPlaying ? FormatTime(GameState->GetRoundRemainingSeconds()) : FText::FromString(TEXT("-:--")));
	}
	if (Text_RoundNumber)
	{
		Text_RoundNumber->SetText(bPlaying
			? FText::Format(NSLOCTEXT("CT", "UI_Round", "라운드 {0}"), FText::AsNumber(GameState->GetRoundNumber()))
			: FText::GetEmpty());
	}

	// 카운트다운: 숫자가 바뀔 때만 갱신 + 이벤트
	// 경기 중 라운드 전환 카운트다운과, 경기 시작 전 카운트다운 모두 같은 위젯에 보여 줍니다.
	const bool bStartCountdown = GameState && GameState->IsStartCountdown();
	const int32 CountdownNumber = (bPlaying || bStartCountdown) ? GameState->GetCountdownNumber() : 0;
	if (CountdownNumber != LastCountdownNumber)
	{
		LastCountdownNumber = CountdownNumber;

		SetShown(Widget_Countdown, CountdownNumber > 0);
		if (Text_Countdown)
		{
			Text_Countdown->SetText(CountdownNumber > 0 ? FText::AsNumber(CountdownNumber) : FText::GetEmpty());
		}
		if (CountdownNumber > 0)
		{
			BP_OnCountdownTick(CountdownNumber);
			if (bStartCountdown)
			{
				BP_OnGameStartCountdownTick(CountdownNumber);
			}
		}
	}
}

void UCTHUDWidget::UpdateActionGauges()
{
	const ACTCharacter* MyCharacter = GetOwningCTCharacter();

	if (ProgressBar_Run)
	{
		ProgressBar_Run->SetPercent(MyCharacter ? MyCharacter->GetRunGaugePercent() : 0.f);
	}
	if (ProgressBar_Push)
	{
		ProgressBar_Push->SetPercent(MyCharacter ? MyCharacter->GetPushReadyPercent() : 0.f);
	}
	if (Text_RunState)
	{
		FText RunText = FText::GetEmpty();
		if (MyCharacter)
		{
			FNumberFormattingOptions OneDecimal;
			OneDecimal.MinimumFractionalDigits = 1;
			OneDecimal.MaximumFractionalDigits = 1;
			const FText Remaining = FText::AsNumber(MyCharacter->GetRunStateRemainingSeconds(), &OneDecimal);

			if (MyCharacter->IsGhost())
			{
				RunText = NSLOCTEXT("CT", "UI_Run_Ghost", "달리기 불가");
			}
			else
			{
				switch (MyCharacter->GetRunState())
				{
				case ECTRunState::Idle:
					RunText = NSLOCTEXT("CT", "UI_Run_Ready", "달리기 가능");
					break;
				case ECTRunState::Running:
					RunText = FText::Format(NSLOCTEXT("CT", "UI_Run_Running", "달리는 중 {0}"), Remaining);
					break;
				case ECTRunState::Cooldown:
					RunText = FText::Format(NSLOCTEXT("CT", "UI_Run_Cooldown", "쿨타임 {0}"), Remaining);
					break;
				}
			}
		}
		Text_RunState->SetText(RunText);
	}
}

void UCTHUDWidget::RefreshMyState()
{
	ACTPlayerState* MyPlayerState = BoundMyPlayerState.Get();

	const ECTColor MyColor = MyPlayerState ? MyPlayerState->GetAssignedColor() : ECTColor::None;
	const ECTColor TargetColor = MyPlayerState ? MyPlayerState->GetTargetColor() : ECTColor::None;
	const int32 TailCount = MyPlayerState ? MyPlayerState->GetTailCount() : 0;
	const bool bIsGhost = MyPlayerState && MyPlayerState->IsGhost();

	ApplyColorToWidgets(MyColor, Text_MyColor, Image_MyColor, NSLOCTEXT("CT", "UI_NoMyColor", "-"));
	ApplyColorToWidgets(TargetColor, Text_TargetColor, Image_TargetColor, NSLOCTEXT("CT", "UI_NoTarget", "목표 없음"));

	if (Text_TailCount)
	{
		Text_TailCount->SetText(FText::AsNumber(TailCount));
	}
	SetShown(Widget_Ghost, bIsGhost);

	if (!MyPlayerState) { return; }

	// 꼬리 수 변화 이벤트 (처음 연결할 때는 변화로 치지 않음)
	if (LastTailCount != INDEX_NONE && LastTailCount != TailCount)
	{
		BP_OnMyTailCountChanged(TailCount, TailCount - LastTailCount);
	}
	LastTailCount = TailCount;

	if (bIsGhost != bLastIsGhost)
	{
		bLastIsGhost = bIsGhost;
		BP_OnMyGhostChanged(bIsGhost);
	}

	BP_OnMyStateChanged(MyPlayerState);
}

void UCTHUDWidget::SyncScoreboardPlayers()
{
	// 이름 변경처럼 알림이 없는 값도 반영되도록 주기적으로 다시 그립니다.
	bScoreboardDirty = true;

	if (!VerticalBox_Scoreboard || !ScoreEntryClass) { return; }

	const TArray<ACTPlayerState*> Players = GetSortedPlayers(BoundGameState.Get());

	// 나간 플레이어의 줄 제거
	for (int32 Index = ScoreEntries.Num() - 1; Index >= 0; --Index)
	{
		UCTScoreEntryWidget* Entry = ScoreEntries[Index];
		if (!Entry || !Players.Contains(Entry->GetEntryPlayerState()))
		{
			if (Entry)
			{
				Entry->RemoveFromParent();
			}
			ScoreEntries.RemoveAt(Index);
		}
	}

	// 새로 들어온 플레이어의 줄 추가
	for (ACTPlayerState* PlayerState : Players)
	{
		const bool bHasEntry = ScoreEntries.ContainsByPredicate(
			[PlayerState](const UCTScoreEntryWidget* Entry) { return Entry && Entry->GetEntryPlayerState() == PlayerState; });
		if (bHasEntry) { continue; }

		UCTScoreEntryWidget* NewEntry = CreateWidget<UCTScoreEntryWidget>(this, ScoreEntryClass);
		if (!NewEntry) { continue; }

		NewEntry->InheritColorIcons(this);
		NewEntry->SetEntryPlayerState(PlayerState);
		ScoreEntries.Add(NewEntry);

		PlayerState->OnPlayerStateChanged.AddUniqueDynamic(this, &UCTHUDWidget::HandleAnyPlayerStateChanged);
		BoundScorePlayerStates.AddUnique(PlayerState);
	}

	// 사라진 PlayerState는 목록에서 정리
	BoundScorePlayerStates.RemoveAll([](const TWeakObjectPtr<ACTPlayerState>& PlayerState) { return !PlayerState.IsValid(); });
}

void UCTHUDWidget::RefreshScoreboard()
{
	bScoreboardDirty = false;

	if (!VerticalBox_Scoreboard) { return; }

	// 꼬리 수 순으로 정렬
	TArray<UCTScoreEntryWidget*> SortedEntries;
	for (UCTScoreEntryWidget* Entry : ScoreEntries)
	{
		if (Entry && Entry->GetEntryPlayerState())
		{
			SortedEntries.Add(Entry);
		}
	}
	SortedEntries.Sort([](const UCTScoreEntryWidget& A, const UCTScoreEntryWidget& B)
	{
		const ACTPlayerState* PlayerA = A.GetEntryPlayerState();
		const ACTPlayerState* PlayerB = B.GetEntryPlayerState();
		if (PlayerA->GetTailCount() != PlayerB->GetTailCount())
		{
			return PlayerA->GetTailCount() > PlayerB->GetTailCount();
		}
		return PlayerA->GetPlayerName() < PlayerB->GetPlayerName();
	});

	// 순서가 바뀌었을 때만 다시 배치 (매번 다시 넣으면 위젯 애니메이션이 끊김)
	bool bOrderChanged = VerticalBox_Scoreboard->GetChildrenCount() != SortedEntries.Num();
	for (int32 Index = 0; !bOrderChanged && Index < SortedEntries.Num(); ++Index)
	{
		bOrderChanged = VerticalBox_Scoreboard->GetChildAt(Index) != SortedEntries[Index];
	}
	if (bOrderChanged)
	{
		VerticalBox_Scoreboard->ClearChildren();
		for (UCTScoreEntryWidget* Entry : SortedEntries)
		{
			VerticalBox_Scoreboard->AddChildToVerticalBox(Entry);
		}
	}

	// 순위 계산 (동점이면 같은 순위) 후 갱신
	const ACTPlayerState* MyPlayerState = BoundMyPlayerState.Get();
	int32 Rank = 0;
	int32 PreviousTailCount = INDEX_NONE;
	for (int32 Index = 0; Index < SortedEntries.Num(); ++Index)
	{
		const ACTPlayerState* PlayerState = SortedEntries[Index]->GetEntryPlayerState();
		if (PlayerState->GetTailCount() != PreviousTailCount)
		{
			Rank = Index + 1;
			PreviousTailCount = PlayerState->GetTailCount();
		}
		SortedEntries[Index]->RefreshEntry(Rank, PlayerState == MyPlayerState);
	}
}
