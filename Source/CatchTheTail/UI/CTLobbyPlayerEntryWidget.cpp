// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyPlayerEntryWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCTLobbyPlayerEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// 디자인에서 정한 줄 배경 색을 기억해 둠 (팀전이 아닐 때 되돌리려고)
	if (Image_PlayerBack) { DefaultBackColor = Image_PlayerBack->GetColorAndOpacity(); }
}

void UCTLobbyPlayerEntryWidget::SetEntry(const FString& PlayerName, ECTLobbyPlayerKind Kind, bool bIsMe, const TOptional<FLinearColor>& TeamColor)
{
	FText StateText;
	FLinearColor StateColor;
	switch (Kind)
	{
	case ECTLobbyPlayerKind::Host:    StateText = HostText;    StateColor = HostColor;    break;
	case ECTLobbyPlayerKind::Ready:   StateText = ReadyText;   StateColor = ReadyColor;   break;
	case ECTLobbyPlayerKind::AI:      StateText = AIText;      StateColor = AIColor;      break;
	default:                          StateText = WaitingText; StateColor = WaitingColor; break;
	}

	if (Text_Name)  { Text_Name->SetText(FText::FromString(PlayerName)); }
	if (Text_State) { Text_State->SetText(StateText); }
	if (Image_StatePill) { Image_StatePill->SetColorAndOpacity(StateColor); }
	if (Image_Host) { Image_Host->SetVisibility(Kind == ECTLobbyPlayerKind::Host ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (Image_Me)   { Image_Me->SetVisibility(bIsMe ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
	if (Image_PlayerBack)
	{
		Image_PlayerBack->SetColorAndOpacity(TeamColor.IsSet() ? FMath::Lerp(FLinearColor::White, TeamColor.GetValue(), TeamTintStrength) : DefaultBackColor);
	}
	if (Image_TeamColor)
	{
		Image_TeamColor->SetVisibility(TeamColor.IsSet() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (TeamColor.IsSet()) { Image_TeamColor->SetColorAndOpacity(TeamColor.GetValue()); }
	}
}
