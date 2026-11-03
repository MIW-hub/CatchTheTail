// Copyright Moon Party Games. All Rights Reserved.
#include "CTLoadingWidget.h"
#include "../Core/CTGameState.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

namespace
{
	// 게임 설명 모음. 열 때마다 이 중 하나를 무작위로 보여줌
	TArray<FText> MakeTips()
	{
		return {
			NSLOCTEXT("CT", "Loading_Tip_Target", "목표 색의 플레이어에게서 꼬리를 빼앗으세요."),
			NSLOCTEXT("CT", "Loading_Tip_ColorChange", "라운드마다 목표 색이 바뀌니 화면의 색을 꼭 확인하세요."),
			NSLOCTEXT("CT", "Loading_Tip_Run", "Shift를 누르면 달릴 수 있지만 오래 달리면 지쳐요."),
			NSLOCTEXT("CT", "Loading_Tip_Push", "목표 색이 아닌 상대는 마우스 우클릭으로 밀어낼 수 있어요."),
			NSLOCTEXT("CT", "Loading_Tip_Ghost", "꼬리를 모두 잃으면 유령이 되어 이동만 할 수 있어요."),
			NSLOCTEXT("CT", "Loading_Tip_Win", "경기가 끝났을 때 꼬리가 가장 많은 플레이어가 승리해요."),
			NSLOCTEXT("CT", "Loading_Tip_Bush", "부쉬 안에 들어가면 다른 플레이어의 눈을 피할 수 있어요."),
			NSLOCTEXT("CT", "Loading_Tip_Gauge", "달리기와 밀치기 게이지를 확인하며 움직이세요."),
		};
	}
}

void UCTLoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ElapsedSeconds = 0.f;
	PickRandomTip();
	RefreshPlayerCount();

	if (ProgressBar_Loading) { ProgressBar_Loading->SetPercent(0.f); }
}

void UCTLoadingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ElapsedSeconds += InDeltaTime;

	// 점이 0~3개로 돌아가며 늘어남
	if (Text_Loading)
	{
		const int32 DotCount = static_cast<int32>(ElapsedSeconds / DotIntervalSeconds) % 4;
		const FString Dots = FString::ChrN(DotCount, TEXT('.'));
		Text_Loading->SetText(FText::FromString(NSLOCTEXT("CT", "Loading_Label", "로딩 중").ToString() + Dots));
	}

	// 처음엔 빠르게, 끝에 가까울수록 느리게 차오름
	if (ProgressBar_Loading)
	{
		const float Percent = ProgressMax * (1.f - FMath::Exp(-ElapsedSeconds * ProgressSpeed));
		ProgressBar_Loading->SetPercent(Percent);
	}

	RefreshPlayerCount();
}

void UCTLoadingWidget::RefreshPlayerCount()
{
	if (!Text_PlayerCount) { return; }

	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState) { return; }

	Text_PlayerCount->SetText(FText::Format(
		NSLOCTEXT("CT", "Loading_Players", "참가 인원 {0}/{1}"),
		CTGameState->PlayerArray.Num(), CTGameState->GetMatchSettings().MaxPlayers));
}

void UCTLoadingWidget::PickRandomTip()
{
	if (!Text_Tip) { return; }

	const TArray<FText> Tips = MakeTips();
	Text_Tip->SetText(Tips[FMath::RandRange(0, Tips.Num() - 1)]);
}
