// Copyright Moon Party Games. All Rights Reserved.
#include "CTResultWidget.h"
#include "CTButtonHoverEffect.h"
#include "../Core/CTAudioSubsystem.h"
#include "CTScoreEntryWidget.h"
#include "../Core/CTGameState.h"
#include "../Core/CTGameInstance.h"
#include "../Player/CTPlayerState.h"
#include "../Player/CTPlayerController.h"
#include "../Core/CTMatchHistory.h"
#include "../Core/CTMapSettings.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UObject/ConstructorHelpers.h"

UCTResultWidget::UCTResultWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 블루프린트에서 지정이 풀려도 순위가 비지 않도록 기본 줄 위젯을 미리 지정 (블루프린트 값이 있으면 그 값이 우선)
	static ConstructorHelpers::FClassFinder<UCTScoreEntryWidget> ScoreEntryFinder(TEXT("/Game/UI/WBP_CTScoreEntryWidget"));
	if (ScoreEntryFinder.Succeeded())
	{
		ScoreEntryClass = ScoreEntryFinder.Class;
	}
}

void UCTResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UCTButtonHoverEffect::Attach(this);

	// 버튼 소리 (올림 / 누름)
	UCTAudioSubsystem::ApplyUISounds(this);

	if (Button_ReturnToLobby) { Button_ReturnToLobby->OnClicked.AddUniqueDynamic(this, &UCTResultWidget::HandleReturnToLobbyClicked); }
	if (Button_LeaveToMenu)   { Button_LeaveToMenu->OnClicked.AddUniqueDynamic(this, &UCTResultWidget::HandleLeaveToMenuClicked); }
}

void UCTResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 로비로 돌아가기: 방장에게만 버튼, 모두에게 남은 시간 안내
	const ACTGameState* GameState = GetCTGameState();
	const ACTPlayerState* MyPlayerState = GetOwningCTPlayerState();

	if (Button_ReturnToLobby)
	{
		Button_ReturnToLobby->SetVisibility((MyPlayerState && MyPlayerState->IsHost()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (Text_ReturnHint)
	{
		const float Remaining = GameState ? GameState->GetReturnToLobbyRemainingSeconds() : 0.f;
		if (Remaining > 0.f)
		{
			Text_ReturnHint->SetText(FText::Format(NSLOCTEXT("CT", "UI_Result_ReturnHint", "{0}초 후 로비로 돌아갑니다"), FMath::CeilToInt(Remaining)));
			Text_ReturnHint->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_ReturnHint->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCTResultWidget::HandleReturnToLobbyClicked()
{
	if (ACTPlayerController* Controller = GetOwningPlayer<ACTPlayerController>())
	{
		Controller->RequestReturnToLobby();
	}
}

void UCTResultWidget::HandleLeaveToMenuClicked()
{
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->LeaveToMainMenu(FText::GetEmpty());
	}
}

void UCTResultWidget::ShowResult()
{
	const ACTGameState* GameState = GetCTGameState();
	if (!GameState) { return; }

	// 모드별로 표 모양을 나눌 수 있게 모드를 먼저 알림
	BP_OnModeKnown(GameState->GetModeType());

	// 보물찾기: 라운드 승점을 알림 (연출용)
	if (GameState->GetModeType() == ECTGameModeType::TreasureHunt)
	{
		const ACTPlayerState* Me = GetOwningCTPlayerState();
		const int32 MyTeam = Me ? Me->GetTeamId() : 0;
		BP_OnTreasureResult(GameState->GetTeamRoundWins(MyTeam), GameState->GetTeamRoundWins(1 - MyTeam), GameState->IsTie());
	}

	const ACTPlayerState* MyPlayerState = GetOwningCTPlayerState();
	const bool bIsTie = GameState->IsTie();
	const bool bLocalPlayerWon = GameState->IsWinner(MyPlayerState);

	// 내 등수 (꼬리 수가 같으면 같은 등수). 순위표 목록과 같은 규칙
	int32 MyRank = INDEX_NONE;
	{
		const TArray<ACTPlayerState*> Sorted = GetSortedPlayers(GameState);
		int32 Rank = 0;
		int32 PreviousTailCount = INDEX_NONE;
		for (int32 Index = 0; Index < Sorted.Num(); ++Index)
		{
			if (GetPlayerScore(GameState, Sorted[Index]) != PreviousTailCount)
			{
				Rank = Index + 1;
				PreviousTailCount = GetPlayerScore(GameState, Sorted[Index]);
			}
			if (Sorted[Index] == MyPlayerState) { MyRank = Rank; break; }
		}
	}

	if (Text_ResultTitle)
	{
		// 항상 "N등" (연장전 문구는 쓰지 않음) (순위에 없는 관전자는 기존 승리/패배 문구)
		FText Title;
		if (GameState->GetModeType() == ECTGameModeType::TreasureHunt && MyPlayerState)
		{
			// 보물찾기는 팀 승패가 제목: 승리 / 패배 / 무승부 (개인 순위는 아래 표)
			Title = bIsTie ? DrawText : (bLocalPlayerWon ? WinText : LoseText);
		}
		else if (MyRank != INDEX_NONE) { Title = FText::Format(RankText, MyRank); }
		else { Title = bLocalPlayerWon ? WinText : LoseText; }
		Text_ResultTitle->SetText(Title);
	}

	// 1등이면 등수 옆에 왕관
	if (Image_RankCrown)
	{
		Image_RankCrown->SetVisibility((MyRank == 1) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 위에는 내 순위만 보여줌 (우승자 이름 줄은 숨김)
	if (Text_Winners)
	{
		Text_Winners->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!VerticalBox_Ranking || !ScoreEntryClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 결과 순위를 그릴 수 없음: VerticalBox_Ranking=%s, ScoreEntryClass=%s"),
			VerticalBox_Ranking ? TEXT("있음") : TEXT("없음"), ScoreEntryClass ? TEXT("있음") : TEXT("없음"));
	}

	if (VerticalBox_Ranking && ScoreEntryClass)
	{
		VerticalBox_Ranking->ClearChildren();

		const TArray<ACTPlayerState*> Players = GetSortedPlayers(GameState);
		UE_LOG(LogTemp, Log, TEXT("[CT] 결과 순위 %d명 표시"), Players.Num());
		int32 Rank = 0;
		int32 PreviousTailCount = INDEX_NONE;
		for (int32 Index = 0; Index < Players.Num(); ++Index)
		{
			ACTPlayerState* PlayerState = Players[Index];
			if (GetPlayerScore(GameState, PlayerState) != PreviousTailCount)
			{
				Rank = Index + 1;
				PreviousTailCount = GetPlayerScore(GameState, PlayerState);
			}

			UCTScoreEntryWidget* Entry = CreateWidget<UCTScoreEntryWidget>(this, ScoreEntryClass);
			if (!Entry) { continue; }

			Entry->InheritColorIcons(this);
			Entry->SetEntryPlayerState(PlayerState);
			VerticalBox_Ranking->AddChildToVerticalBox(Entry);
			Entry->RefreshEntry(Rank, PlayerState == MyPlayerState);
		}
	}

	// 전적 저장 (이 판 한 번만. 관전자는 제외)
	if (!bRecordedToHistory && MyRank != INDEX_NONE && MyPlayerState)
	{
		bRecordedToHistory = true;

		FCTMatchRecord Record;
		Record.Time = FDateTime::Now();
		Record.MyRank = MyRank;
		Record.MyTailCount = GetPlayerScore(GameState, MyPlayerState);
		Record.PlayerCount = GetSortedPlayers(GameState).Num();
		{
			FCTMapInfo MapInfo;
			if (UCTMapSettings::GetMapInfo(GameState->GetSelectedMapIndex(), MapInfo)) { Record.MapName = MapInfo.DisplayName.ToString(); }
		}

		const TArray<ACTPlayerState*> Sorted = GetSortedPlayers(GameState);
		int32 Rank = 0;
		int32 PreviousTailCount = INDEX_NONE;
		for (int32 Index = 0; Index < Sorted.Num(); ++Index)
		{
			if (GetPlayerScore(GameState, Sorted[Index]) != PreviousTailCount)
			{
				Rank = Index + 1;
				PreviousTailCount = GetPlayerScore(GameState, Sorted[Index]);
			}
			// 1~3등만 기록 (동점이면 같은 등수 사람이 모두 들어감)
			if (Rank > 3) { break; }
			FCTMatchRecordEntry& Entry = Record.Entries.AddDefaulted_GetRef();
			Entry.PlayerName = Sorted[Index]->GetPlayerName();
			Entry.Rank = Rank;
			Entry.TailCount = GetPlayerScore(GameState, Sorted[Index]);
			Entry.bIsMe = (Sorted[Index] == MyPlayerState);
		}
		UCTMatchHistory::AddRecord(Record);
	}

	BP_OnResultShown(bLocalPlayerWon, bIsTie);
}
