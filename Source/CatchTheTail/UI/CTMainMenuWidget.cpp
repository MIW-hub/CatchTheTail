// Copyright Moon Party Games. All Rights Reserved.
#include "CTMainMenuWidget.h"
#include "../Core/CTGameInstance.h"
#include "CTSettingsWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
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
	if (Button_JoinByCode)  { Button_JoinByCode->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleJoinByCodeClicked); }
	if (Button_QuitConfirm) { Button_QuitConfirm->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleQuitConfirmClicked); }

	// 뒤로가기/취소는 메인으로
	UButton* BackButtons[] = { Button_LobbyBack, Button_SettingsBack, Button_HowToBack, Button_QuitCancel };
	for (UButton* BackButton : BackButtons)
	{
		if (BackButton) { BackButton->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleBackClicked); }
	}

	CreateSettingsWidget();
	ShowPanel(ECTMenuPanel::Main);
	ApplyOnlineAvailability();

	// 저장된 이름 표시
	if (Text_PlayerName)
	{
		if (const UCTGameInstance* NameGameInstance = GetGameInstance<UCTGameInstance>())
		{
			Text_PlayerName->SetText(FText::FromString(NameGameInstance->GetPlayerName()));
		}
	}

	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->OnMenuMessage.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleMessage);

		// 연결 끊김 안내
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

// 설정 화면

void UCTMainMenuWidget::CreateSettingsWidget()
{
	if (SettingsWidget || !SettingsWidgetClass) { return; }

	UPanelWidget* Container = Cast<UPanelWidget>(Panel_Settings);
	if (!Container)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 설정 화면을 넣을 Panel_Settings가 없거나 패널 위젯이 아닙니다."));
		return;
	}

	SettingsWidget = CreateWidget<UCTSettingsWidget>(this, SettingsWidgetClass);
	if (!SettingsWidget) { return; }

	Container->AddChild(SettingsWidget);

	// 설정 화면의 닫기 버튼 = 메인 메뉴로 돌아가기
	SettingsWidget->OnCloseRequested.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleBackClicked);
}

// 패널 전환

void UCTMainMenuWidget::SetPanelVisible(UWidget* Panel, bool bVisible)
{
	if (Panel)
	{
		Panel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCTMainMenuWidget::ShowPanel(ECTMenuPanel Panel)
{
	// 없는 패널은 무시 (메인 제외)
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

	// 팝업이라 메인 패널 유지
	SetPanelVisible(Panel_Main, Panel == ECTMenuPanel::Main || Panel == ECTMenuPanel::QuitConfirm);
	SetPanelVisible(Panel_Lobby, Panel == ECTMenuPanel::Lobby);
	SetPanelVisible(Panel_Settings, Panel == ECTMenuPanel::Settings);
	SetPanelVisible(Panel_HowTo, Panel == ECTMenuPanel::HowTo);
	SetPanelVisible(Panel_QuitConfirm, Panel == ECTMenuPanel::QuitConfirm);

	BP_OnPanelChanged(Panel);
}

void UCTMainMenuWidget::ApplyOnlineAvailability()
{
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();

	const bool bAvailable = GameInstance && GameInstance->IsOnlineSessionAvailable();
	const FText Reason = bAvailable ? FText::GetEmpty() : NSLOCTEXT("CT", "Menu_OnlineUnavailable", "스팀 연결 후 사용할 수 있습니다.");

	UWidget* Blocked[] = { Button_Start, Button_JoinByCode, EditableTextBox_RoomCode };
	for (UWidget* Widget : Blocked)
	{
		if (!Widget) { continue; }
		Widget->SetIsEnabled(bAvailable);
		Widget->SetToolTipText(Reason);
	}
}

void UCTMainMenuWidget::HandleJoinByCodeClicked()
{
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->JoinByRoomCode(EditableTextBox_RoomCode ? EditableTextBox_RoomCode->GetText().ToString() : FString());
	}
}

// 버튼

void UCTMainMenuWidget::HandleStartClicked()
{
	// 빠른 참가: 열린 방에 무작위 입장 (온라인 기능이 막혀 있으면 안내만)
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->QuickJoin();
	}
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
