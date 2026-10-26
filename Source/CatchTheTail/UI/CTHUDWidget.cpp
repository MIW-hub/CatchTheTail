// Copyright Moon Party Games. All Rights Reserved.
#include "CTHUDWidget.h"
#include "CTScoreEntryWidget.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	UpdateSkillSlots();

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
	UpdateSkillSlots();

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

// 연결

void UCTHUDWidget::TryBindGameState()
{
	if (BoundGameState.IsValid()) { return; }

	ACTGameState* GameState = GetCTGameState();
	if (!GameState) { return; }

	BoundGameState = GameState;
	GameState->OnPhaseChanged.AddUniqueDynamic(this, &UCTHUDWidget::HandlePhaseChanged);
	GameState->OnRoundChanged.AddUniqueDynamic(this, &UCTHUDWidget::HandleRoundChanged);

	// 연결 전 값도 반영
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
	RefreshHunter(/*bForce=*/false);
}

// 갱신

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

	// 카운트다운 숫자 변할 때만 갱신
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

UMaterialInstanceDynamic* UCTHUDWidget::GetOverlayMaterial(UImage* Overlay)
{
	if (!Overlay) { return nullptr; }

	if (const TObjectPtr<UMaterialInstanceDynamic>* Found = OverlayMaterials.Find(Overlay))
	{
		return Found->Get();
	}

	// 브러시가 머티리얼일 때만 만들어짐 (일반 이미지면 nullptr)
	UMaterialInstanceDynamic* Material = Overlay->GetDynamicMaterial();
	OverlayMaterials.Add(Overlay, Material);
	return Material;
}

void UCTHUDWidget::UpdateSkillSlot(UImage* Overlay, UTextBlock* Text, float Remaining, float Total, bool bRunning, bool bAvailable)
{
	const bool bCoolingDown = !bRunning && bAvailable && Remaining > KINDA_SMALL_NUMBER && Total > 0.f;

	if (Overlay)
	{
		// 쿨타임 중에는 남은 비율만큼, 사용 불가는 전부 덮음
		const bool bShowOverlay = bCoolingDown || (!bRunning && !bAvailable);
		const float Progress = bCoolingDown ? FMath::Clamp(Remaining / Total, 0.f, 1.f) : 1.f;

		if (UMaterialInstanceDynamic* Material = GetOverlayMaterial(Overlay))
		{
			Material->SetScalarParameterValue(CooldownProgressParameter, Progress);
		}
		SetShown(Overlay, bShowOverlay);
	}

	if (Text)
	{
		if (bRunning || bCoolingDown)
		{
			// 10초 미만은 소수점 한 자리
			FNumberFormattingOptions Format;
			Format.MinimumFractionalDigits = Remaining < 10.f ? 1 : 0;
			Format.MaximumFractionalDigits = Remaining < 10.f ? 1 : 0;
			Text->SetText(FText::AsNumber(Remaining, &Format));
			Text->SetColorAndOpacity(FSlateColor(bRunning ? RunActiveTextColor : CooldownTextColor));
			SetShown(Text, true);
		}
		else
		{
			Text->SetText(FText::GetEmpty());
			SetShown(Text, false);
		}
	}
}

void UCTHUDWidget::UpdateSkillSlots()
{
	const ACTCharacter* MyCharacter = GetOwningCTCharacter();

	const bool bCanAct = MyCharacter && MyCharacter->CanUseActions();
	const bool bCanRun = bCanAct && MyCharacter->CanMoveNow();

	// 달리기: 달리는 중에는 남은 달리기 시간, 쿨타임이면 남은 쿨타임
	const ECTRunState RunState = MyCharacter ? MyCharacter->GetRunState() : ECTRunState::Idle;
	const float RunRemaining = MyCharacter ? MyCharacter->GetRunStateRemainingSeconds() : 0.f;
	UpdateSkillSlot(Image_CoolTime, Text_RunCooldown,
		RunState == ECTRunState::Idle ? 0.f : RunRemaining,
		MyCharacter ? MyCharacter->GetRunCooldownSeconds() : 0.f,
		bCanRun && RunState == ECTRunState::Running,
		bCanRun);

	UpdateSkillSlot(Image_CoolTime_1, Text_GrabCooldown,
		MyCharacter ? MyCharacter->GetGrabCooldownRemainingSeconds() : 0.f,
		MyCharacter ? MyCharacter->GetGrabCooldownSeconds() : 0.f,
		false, bCanAct);

	UpdateSkillSlot(Image_CoolTime_2, Text_PushCooldown,
		MyCharacter ? MyCharacter->GetPushCooldownRemainingSeconds() : 0.f,
		MyCharacter ? MyCharacter->GetPushCooldownSeconds() : 0.f,
		false, bCanAct);
}

void UCTHUDWidget::RefreshHunter(bool bForce)
{
	const ACTPlayerState* MyPlayerState = BoundMyPlayerState.Get();
	const ECTColor MyColor = MyPlayerState ? MyPlayerState->GetAssignedColor() : ECTColor::None;

	// 나를 목표로 가진 사람의 색
	ECTColor HunterColor = ECTColor::None;
	if (MyColor != ECTColor::None)
	{
		for (const ACTPlayerState* PlayerState : GetSortedPlayers(BoundGameState.Get()))
		{
			if (PlayerState != MyPlayerState && PlayerState->GetTargetColor() == MyColor)
			{
				HunterColor = PlayerState->GetAssignedColor();
				break;
			}
		}
	}

	if (!bForce && bHunterInitialized && HunterColor == LastHunterColor) { return; }

	bHunterInitialized = true;
	LastHunterColor = HunterColor;
	ApplyColorToWidgets(HunterColor, Text_HunterColor, Image_Hunter, NSLOCTEXT("CT", "UI_NoHunter", "-"));
}

void UCTHUDWidget::RefreshMyState()
{
	ACTPlayerState* MyPlayerState = BoundMyPlayerState.Get();

	const ECTColor MyColor = MyPlayerState ? MyPlayerState->GetAssignedColor() : ECTColor::None;
	const ECTColor TargetColor = MyPlayerState ? MyPlayerState->GetTargetColor() : ECTColor::None;
	const int32 TailCount = MyPlayerState ? MyPlayerState->GetTailCount() : 0;
	const bool bIsGhost = MyPlayerState && MyPlayerState->IsGhost();

	ApplyColorToWidgets(MyColor, Text_MyColor, Image_My, NSLOCTEXT("CT", "UI_NoMyColor", "-"));
	ApplyColorToWidgets(TargetColor, Text_TargetColor, Image_TargetColor, NSLOCTEXT("CT", "UI_NoTarget", "목표 없음"));
	RefreshHunter(/*bForce=*/true);

	if (Text_TailCount)
	{
		Text_TailCount->SetText(FText::AsNumber(TailCount));
	}
	SetShown(Widget_Ghost, bIsGhost);

	if (!MyPlayerState) { return; }

	// 꼬리 수 변화 이벤트 (첫 연결 제외)
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
	// 알림 없는 값(이름 등) 주기 갱신
	bScoreboardDirty = true;
	RefreshHunter(/*bForce=*/false);

	if (!VerticalBox_Scoreboard || !ScoreEntryClass) { return; }

	const TArray<ACTPlayerState*> Players = GetSortedPlayers(BoundGameState.Get());

	// 나간 플레이어 줄 제거
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

	// 새 플레이어 줄 추가
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

	// 사라진 PlayerState 정리
	BoundScorePlayerStates.RemoveAll([](const TWeakObjectPtr<ACTPlayerState>& PlayerState) { return !PlayerState.IsValid(); });
}

void UCTHUDWidget::RefreshScoreboard()
{
	bScoreboardDirty = false;

	if (!VerticalBox_Scoreboard) { return; }

	// 꼬리 수 순 정렬
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

	// 순서 바뀔 때만 재배치 (애니메이션 끊김)
	// 화면에는 위쪽 몇 명만 (전체 순위는 결과 화면)
	const int32 DisplayCount = MaxScoreboardEntries > 0 ? FMath::Min(MaxScoreboardEntries, SortedEntries.Num()) : SortedEntries.Num();

	bool bOrderChanged = VerticalBox_Scoreboard->GetChildrenCount() != DisplayCount;
	for (int32 Index = 0; !bOrderChanged && Index < DisplayCount; ++Index)
	{
		bOrderChanged = VerticalBox_Scoreboard->GetChildAt(Index) != SortedEntries[Index];
	}
	if (bOrderChanged)
	{
		VerticalBox_Scoreboard->ClearChildren();
		for (int32 Index = 0; Index < DisplayCount; ++Index)
		{
			VerticalBox_Scoreboard->AddChildToVerticalBox(SortedEntries[Index]);
		}
	}

	// 동점은 같은 순위
	const ACTPlayerState* MyPlayerState = BoundMyPlayerState.Get();
	int32 Rank = 0;
	// 모두 동점이면 왕관 없음
	const bool bAllTied = SortedEntries.Num() > 1
		&& SortedEntries[0]->GetEntryPlayerState()->GetTailCount() == SortedEntries.Last()->GetEntryPlayerState()->GetTailCount();
	int32 PreviousTailCount = INDEX_NONE;
	for (int32 Index = 0; Index < SortedEntries.Num(); ++Index)
	{
		const ACTPlayerState* PlayerState = SortedEntries[Index]->GetEntryPlayerState();
		if (PlayerState->GetTailCount() != PreviousTailCount)
		{
			Rank = Index + 1;
			PreviousTailCount = PlayerState->GetTailCount();
		}
		SortedEntries[Index]->RefreshEntry(Rank, PlayerState == MyPlayerState, !bAllTied);
	}
}
