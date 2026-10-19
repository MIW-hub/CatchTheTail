// Copyright Moon Party Games. All Rights Reserved.
#include "CTMainMenuWidget.h"
#include "../Core/CTGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"

void UCTMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Start)       { Button_Start->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleStartClicked); }
	if (Button_FindLobby)   { Button_FindLobby->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleFindLobbyClicked); }
	if (Button_Settings)    { Button_Settings->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleSettingsClicked); }
	if (Button_HowTo)       { Button_HowTo->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleHowToClicked); }
	if (Button_Quit)        { Button_Quit->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleQuitClicked); }
	if (Button_Host)        { Button_Host->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleHostClicked); }
	if (Button_Join)        { Button_Join->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleJoinClicked); }
	if (Button_QuitConfirm) { Button_QuitConfirm->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleQuitConfirmClicked); }

	// 뒤로가기/취소 버튼은 모두 메인 패널로 돌아갑니다.
	UButton* BackButtons[] = { Button_LobbyBack, Button_SettingsBack, Button_HowToBack, Button_QuitCancel };
	for (UButton* BackButton : BackButtons)
	{
		if (BackButton) { BackButton->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleBackClicked); }
	}

	ShowPanel(ECTMenuPanel::Main);

	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->OnMenuMessage.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleMessage);

		// 게임 중 연결이 끊겨서 메뉴로 돌아온 경우 안내 문구를 보여 줍니다.
		const FText Pending = GameInstance->GetPendingMessage();
		if (!Pending.IsEmpty())
		{
			ShowMessage(Pending);
			GameInstance->ClearPendingMessage();
		}
	}
}

void UCTMainMenuWidget::NativeDestruct()
{
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->OnMenuMessage.RemoveDynamic(this, &UCTMainMenuWidget::HandleMessage);
	}

	Super::NativeDestruct();
}

// ============================================================
// 패널 전환
// ============================================================

void UCTMainMenuWidget::SetPanelVisible(UWidget* Panel, bool bVisible)
{
	if (Panel)
	{
		Panel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCTMainMenuWidget::ShowPanel(ECTMenuPanel Panel)
{
	// 만들지 않은 패널로는 이동하지 않습니다. (메인은 항상 허용)
	UWidget* Target = nullptr;
	switch (Panel)
	{
	case ECTMenuPanel::Main:        Target = Panel_Main; break;
	case ECTMenuPanel::Lobby:       Target = Panel_Lobby; break;
	case ECTMenuPanel::Settings:    Target = Panel_Settings; break;
	case ECTMenuPanel::HowTo:       Target = Panel_HowTo; break;
	case ECTMenuPanel::QuitConfirm: Target = Panel_QuitConfirm; break;
	}
	if (!Target && Panel != ECTMenuPanel::Main) { return; }

	CurrentPanel = Panel;

	// 종료 확인은 메인 화면 위에 겹쳐 뜨는 팝업이라 메인 패널을 끄지 않습니다.
	SetPanelVisible(Panel_Main, Panel == ECTMenuPanel::Main || Panel == ECTMenuPanel::QuitConfirm);
	SetPanelVisible(Panel_Lobby, Panel == ECTMenuPanel::Lobby);
	SetPanelVisible(Panel_Settings, Panel == ECTMenuPanel::Settings);
	SetPanelVisible(Panel_HowTo, Panel == ECTMenuPanel::HowTo);
	SetPanelVisible(Panel_QuitConfirm, Panel == ECTMenuPanel::QuitConfirm);

	BP_OnPanelChanged(Panel);
}

// ============================================================
// 버튼
// ============================================================

void UCTMainMenuWidget::HandleStartClicked()
{
	// 게임 시작 = 방을 만들고 바로 들어갑니다. (로비/Ready는 아직 없음)
	HandleHostClicked();
}

void UCTMainMenuWidget::HandleFindLobbyClicked()
{
	ShowPanel(ECTMenuPanel::Lobby);
}

void UCTMainMenuWidget::HandleSettingsClicked()
{
	ShowPanel(ECTMenuPanel::Settings);
}

void UCTMainMenuWidget::HandleHowToClicked()
{
	ShowPanel(ECTMenuPanel::HowTo);
}

void UCTMainMenuWidget::HandleQuitClicked()
{
	if (Panel_QuitConfirm)
	{
		ShowPanel(ECTMenuPanel::QuitConfirm);
	}
	else
	{
		HandleQuitConfirmClicked();
	}
}

void UCTMainMenuWidget::HandleBackClicked()
{
	ShowPanel(ECTMenuPanel::Main);
}

void UCTMainMenuWidget::HandleHostClicked()
{
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->HostGame();
	}
}

void UCTMainMenuWidget::HandleJoinClicked()
{
	UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	if (!GameInstance) { return; }

	const FString Address = EditableTextBox_Address ? EditableTextBox_Address->GetText().ToString() : FString();
	GameInstance->JoinGame(Address);
}

void UCTMainMenuWidget::HandleQuitConfirmClicked()
{
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->QuitToDesktop();
	}
}

void UCTMainMenuWidget::HandleMessage(FText Message)
{
	ShowMessage(Message);
}

void UCTMainMenuWidget::ShowMessage(const FText& Message)
{
	if (Text_Status)
	{
		Text_Status->SetText(Message);
	}
	BP_OnMessageShown(Message);
}
