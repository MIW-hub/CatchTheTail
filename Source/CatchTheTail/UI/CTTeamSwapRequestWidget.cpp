// Copyright Moon Party Games. All Rights Reserved.
#include "CTTeamSwapRequestWidget.h"
#include "CTCircularProgress.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTGameState.h"
#include "../Player/CTLobbyPlayerController.h"
#include "../Player/CTPlayerState.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void UCTTeamSwapRequestWidget::SetRequester(ACTPlayerState* NewRequester)
{
	Requester = NewRequester;
	if (Text_RequesterName && Requester)
	{
		Text_RequesterName->SetText(FText::FromString(Requester->GetPlayerName()));
	}
}

void UCTTeamSwapRequestWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UCTAudioSubsystem::ApplyUISounds(this);

	if (Button_Accept) { Button_Accept->OnClicked.AddUniqueDynamic(this, &UCTTeamSwapRequestWidget::HandleAcceptClicked); }
	if (Text_AcceptLabel) { Text_AcceptLabel->SetText(AcceptText); }
}

void UCTTeamSwapRequestWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UWorld* World = GetWorld();
	const ACTGameState* GameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	if (!GameState || !Requester) { return; }

	// 남은 시간 (요청은 5초 동안 유지)
	for (const FCTTeamSwapRequest& Request : GameState->GetTeamSwapRequests())
	{
		if (Request.Requester != Requester) { continue; }
		const float Remaining = Request.ExpireServerTime - GameState->GetServerWorldTimeSeconds();
		if (Progress_Circle) { Progress_Circle->SetPercent(Remaining / 5.f); }
		break;
	}

	// 수락은 요청자가 가려는 팀(= 요청자와 반대 팀)에 있는, 준비하지 않은 사람만
	if (Button_Accept)
	{
		const ACTPlayerState* MyState = GetOwningPlayerState<ACTPlayerState>();
		const bool bCanAccept = MyState && MyState != Requester && !MyState->IsReady()
			&& MyState->GetTeamId() == 1 - Requester->GetTeamId();
		Button_Accept->SetIsEnabled(bCanAccept);
	}
}

void UCTTeamSwapRequestWidget::HandleAcceptClicked()
{
	if (ACTLobbyPlayerController* Controller = GetOwningPlayer<ACTLobbyPlayerController>())
	{
		Controller->RequestAcceptTeamSwap(Requester);
	}
}
