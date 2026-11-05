// Copyright Moon Party Games. All Rights Reserved.
#include "CTHUDWidget.h"
#include "CTScoreEntryWidget.h"
#include "CTDirectionArrow.h"
#include "../Core/CTSettingsSubsystem.h"
#include "Engine/GameInstance.h"
#include "../Core/CTGameState.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Font.h"
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
	SetShown(Text_Notice, false);
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

	if (NoticeRemaining > 0.f)
	{
		NoticeRemaining -= InDeltaTime;
		if (NoticeRemaining <= 0.f) { SetShown(Text_Notice, false); }
	}

	UpdateDirectionArrow(InDeltaTime);
	UpdateMicIndicator(InDeltaTime);

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

void UCTHUDWidget::UpdateMicIndicator(float DeltaSeconds)
{
	if (!Image_MicIcon && !Image_MicPanel && !Text_MicKey) { return; }

	const UCTSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
	const bool bMicEnabled = !Settings || Settings->IsMicEnabled();

	const APlayerController* PlayerController = GetOwningPlayer();
	const ACTCharacter* MyCharacter = PlayerController ? Cast<ACTCharacter>(PlayerController->GetPawn()) : nullptr;
	const bool bTalking = bMicEnabled && MyCharacter && MyCharacter->IsVoiceTransmitting();

	// 패널과 아이콘, 막대 색: 평소 흰 패널 + 마젠타 / 말하는 중 마젠타 패널 + 흰색 / 꺼짐 회색
	const FLinearColor PanelColor = !bMicEnabled ? MicOffPanelColor : (bTalking ? MicTalkingPanelColor : MicPanelColor);
	const FLinearColor ForeColor = !bMicEnabled ? MicOffIconColor : (bTalking ? MicTalkingIconColor : MicIconColor);
	if (Image_MicPanel) { Image_MicPanel->SetColorAndOpacity(PanelColor); }
	if (Image_MicIcon) { Image_MicIcon->SetColorAndOpacity(ForeColor); }

	// 파장 막대: 말하는 중에는 파도타기처럼 오르내리고, 아니면 계단 모양으로 멈춤 (꺼짐은 낮고 평평)
	MicWaveTime += DeltaSeconds * MicWaveSpeed;
	MicWaveBlend = FMath::FInterpTo(MicWaveBlend, bTalking ? 1.f : 0.f, DeltaSeconds, 14.f);

	const TObjectPtr<USizeBox> WaveBoxes[3] = { Size_MicWave1, Size_MicWave2, Size_MicWave3 };
	const TObjectPtr<UImage> WaveImages[3] = { Image_MicWave1, Image_MicWave2, Image_MicWave3 };
	static const float RestRatios[3] = { 0.35f, 0.65f, 1.f };
	static const float Phases[3] = { 0.f, 2.1f, 4.2f };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		float Ratio = RestRatios[Index];
		if (!bMicEnabled)
		{
			Ratio = 0.2f;
		}
		else if (MicWaveBlend > KINDA_SMALL_NUMBER)
		{
			// 두 개의 사인을 섞어 규칙적이지 않게 출렁임
			const float Wave = 0.5f + 0.32f * FMath::Sin(MicWaveTime + Phases[Index]) + 0.18f * FMath::Sin(MicWaveTime * 1.9f + Phases[Index] * 1.7f);
			Ratio = FMath::Lerp(Ratio, FMath::Clamp(Wave, 0.12f, 1.f), MicWaveBlend);
		}
		if (WaveBoxes[Index]) { WaveBoxes[Index]->SetHeightOverride(FMath::Lerp(MicWaveMinHeight, MicWaveMaxHeight, Ratio)); }
		if (WaveImages[Index]) { WaveImages[Index]->SetColorAndOpacity(ForeColor); }
	}

	// 눌러서 말하기 키 (키를 바꾸면 따라 바뀜)
	if (Text_MicKey && Settings)
	{
		const FString KeyName = Settings->GetActionKey(TEXT("Voice")).GetDisplayName().ToString();
		if (KeyName != LastMicKeyName)
		{
			LastMicKeyName = KeyName;
			Text_MicKey->SetText(FText::FromString(KeyName));
		}
	}
}

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
			// 정수 초 (올림)
			Text->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
			Text->SetColorAndOpacity(FSlateColor(bRunning ? RunActiveTextColor : CooldownTextColor));
			Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text->SetText(FText::GetEmpty());
			Text->SetVisibility(ESlateVisibility::Hidden); // 칸 폭은 유지
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
	// 나를 노리는 사람은 알 수 없음. 전화로 거리만 알아낼 수 있어서 화면에는 표시하지 않음
	if (bHunterInitialized && !bForce) { return; }
	bHunterInitialized = true;

	if (Text_HunterColor) { Text_HunterColor->SetVisibility(ESlateVisibility::Collapsed); }
	if (Image_Hunter) { Image_Hunter->SetVisibility(ESlateVisibility::Collapsed); }
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
	if (Text_TargetColor) { Text_TargetColor->SetColorAndOpacity(FSlateColor(TargetNameTextColor)); }
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

		// 기권자는 되살아날 수 없으므로 안내하지 않음
		if (bIsGhost && !MyPlayerState->IsForfeited())
		{
			ShowNotice(NSLOCTEXT("CT", "Ghost_RelocateHint", "F키: 무작위 위치로 한 번 이동 (유령은 다음 라운드에 부활)"), 8.f);
		}
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

	// 순서 바뀔 때만 재배치, 위쪽 몇 명만
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

UTextBlock* UCTHUDWidget::EnsureNoticeText()
{
	if (Text_Notice) { return Text_Notice; }

	// WBP에 없으면 화면 위 가운데에 직접 만듦 (루트가 캔버스일 때)
	UCanvasPanel* Canvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
	if (!Canvas) { return nullptr; }

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Notice"));
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetShadowOffset(FVector2D(2.f, 2.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));

	FSlateFontInfo Font = Text->GetFont();
	if (UFont* NotoFont = LoadObject<UFont>(nullptr, TEXT("/Game/Font/NotoSansKR-Bold_Font.NotoSansKR-Bold_Font")))
	{
		Font.FontObject = NotoFont;
		Font.TypefaceFontName = NAME_None;
	}
	Font.Size = 30;
	Text->SetFont(Font);

	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Text))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.18f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
	}

	Text_Notice = Text;
	return Text_Notice;
}

UCTDirectionArrow* UCTHUDWidget::EnsureDirectionArrow()
{
	if (DirectionArrow) { return DirectionArrow; }

	// 안내 문구와 같이 루트가 캔버스일 때 화면 위쪽 가운데(문구 아래)에 만듦
	UCanvasPanel* Canvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
	if (!Canvas) { return nullptr; }

	DirectionArrow = WidgetTree->ConstructWidget<UCTDirectionArrow>(UCTDirectionArrow::StaticClass(), TEXT("DirectionArrow"));
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(DirectionArrow))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.3f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetSize(FVector2D(130.f, 130.f));
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
	}
	SetShown(DirectionArrow, false);
	return DirectionArrow;
}

void UCTHUDWidget::ShowDirectionArrow(const FVector& TargetLocation, float Seconds)
{
	UCTDirectionArrow* Arrow = EnsureDirectionArrow();
	if (!Arrow) { return; }

	ArrowTargetLocation = TargetLocation;
	ArrowRemaining = FMath::Max(Seconds, 0.1f);
	SetShown(Arrow, true);
	UpdateDirectionArrow(0.f);
}

void UCTHUDWidget::UpdateDirectionArrow(float DeltaSeconds)
{
	if (!DirectionArrow || ArrowRemaining <= 0.f) { return; }

	ArrowRemaining -= DeltaSeconds;
	if (ArrowRemaining <= 0.f)
	{
		SetShown(DirectionArrow, false);
		return;
	}

	// 카메라가 바라보는 방향에서 목표까지 얼마나 돌아야 하는가
	const APlayerController* Controller = GetOwningPlayer();
	const APawn* Pawn = GetOwningPlayerPawn();
	if (!Controller || !Controller->PlayerCameraManager || !Pawn) { return; }

	const FVector ToTarget = ArrowTargetLocation - Pawn->GetActorLocation();
	const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, ToTarget.X));
	const float CameraYaw = Controller->PlayerCameraManager->GetCameraRotation().Yaw;
	DirectionArrow->SetAngleDegrees(FRotator::NormalizeAxis(TargetYaw - CameraYaw));
}

void UCTHUDWidget::ShowNotice(const FText& Message, float Seconds)
{
	UTextBlock* Text = EnsureNoticeText();
	if (!Text) { return; }

	Text->SetText(Message);
	SetShown(Text, true);
	NoticeRemaining = FMath::Max(Seconds, 0.1f);
}
