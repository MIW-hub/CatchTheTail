// Copyright Moon Party Games. All Rights Reserved.
#include "CTMainMenuWidget.h"
#include "../Core/CTMatchHistory.h"
#include "CTButtonHoverEffect.h"
#include "CTVersionLabel.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTGameInstance.h"
#include "CTSettingsWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UCTMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 메인 메뉴로 나왔다면 이전 방은 끝난 것: 방 전적 비움
	UCTMatchHistory::ClearRecords();
	UCTButtonHoverEffect::Attach(this);

	// 버튼 소리 (올림 / 누름)
	UCTAudioSubsystem::ApplyUISounds(this);

	// 오른쪽 아래에 버전 표시
	CTVersionLabel::Show(GetWorld(), VersionLabel);

	if (Button_Start)       { Button_Start->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleStartClicked); }
	if (Button_FindLobby)   { Button_FindLobby->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleFindLobbyClicked); }
	if (Button_Settings)    { Button_Settings->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleSettingsClicked); }
	if (Button_HowTo)       { Button_HowTo->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleHowToClicked); }
	if (Button_Credits)     { Button_Credits->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleCreditsClicked); }
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
#if !UE_BUILD_SHIPPING
	// 개발용: 명령줄 -CTShowCredits 로 시작하자마자 크레딧을 보여 줌 (화면 확인용)
	if (FParse::Param(FCommandLine::Get(), TEXT("CTShowCredits"))) { ShowPanel(ECTMenuPanel::Credits); }
#endif
	ApplyOnlineAvailability();

	// 저장된 이름 표시
	RefreshPlayerName();

	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->OnMenuMessage.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleMessage);
		GameInstance->OnPlayerNameChanged.AddUniqueDynamic(this, &UCTMainMenuWidget::HandlePlayerNameChanged);

		// 연결 끊김 안내
		const FText Pending = GameInstance->GetPendingMessage();
		if (!Pending.IsEmpty())
		{
			ShowMessage(Pending);
			GameInstance->ClearPendingMessage();
		}
	}
}

void UCTMainMenuWidget::HandlePlayerNameChanged()
{
	RefreshPlayerName();
}

void UCTMainMenuWidget::RefreshPlayerName()
{
	if (!Text_PlayerName) { return; }

	if (const UCTGameInstance* NameGameInstance = GetGameInstance<UCTGameInstance>())
	{
		Text_PlayerName->SetText(FText::FromString(NameGameInstance->GetPlayerName()));
	}
}

void UCTMainMenuWidget::NativeDestruct()
{
	CTVersionLabel::Hide(GetWorld(), VersionLabel);

	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->OnMenuMessage.RemoveDynamic(this, &UCTMainMenuWidget::HandleMessage);
		GameInstance->OnPlayerNameChanged.RemoveDynamic(this, &UCTMainMenuWidget::HandlePlayerNameChanged);
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

	UPanelSlot* PanelSlot = Container->AddChild(SettingsWidget);

	// 설정 화면은 자체 배경과 "가운데 창" 구조라서, 패널을 가득 채우게 둠 (창은 화면에 맞춰 줄어들며 가운데에 놓임)
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PanelSlot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(PanelSlot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(PanelSlot))
	{
		VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
		VerticalSlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(PanelSlot))
	{
		HorizontalSlot->SetHorizontalAlignment(HAlign_Fill);
		HorizontalSlot->SetVerticalAlignment(VAlign_Fill);
	}

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
	case ECTMenuPanel::Credits:     Target = Panel_Credits; break;
	case ECTMenuPanel::QuitConfirm: Target = Panel_QuitConfirm; break;
	}
	if (!Target && Panel != ECTMenuPanel::Main) { return; }

	CurrentPanel = Panel;
	if (Panel == ECTMenuPanel::Credits)
	{
		// 검은 화면이 천천히 나타나도록 처음 상태로
		CreditsTime = 0.f;
		if (Panel_Credits) { Panel_Credits->SetRenderOpacity(0.f); }
		SetKeyboardFocus();
	}

	// 팝업이라 메인 패널 유지
	SetPanelVisible(Panel_Main, Panel == ECTMenuPanel::Main || Panel == ECTMenuPanel::QuitConfirm);
	SetPanelVisible(Panel_Lobby, Panel == ECTMenuPanel::Lobby);
	SetPanelVisible(Panel_Settings, Panel == ECTMenuPanel::Settings);
	SetPanelVisible(Panel_HowTo, Panel == ECTMenuPanel::HowTo);
	SetPanelVisible(Panel_Credits, Panel == ECTMenuPanel::Credits);
	SetPanelVisible(Panel_QuitConfirm, Panel == ECTMenuPanel::QuitConfirm);

	// 크레딧은 검은 화면만 보이게: 오른쪽 위 이름표를 숨김
	{
		const bool bShowName = Panel != ECTMenuPanel::Credits;
		if (Text_PlayerName) { Text_PlayerName->SetVisibility(bShowName ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
		if (WidgetTree)
		{
			for (const TCHAR* Name : { TEXT("Image_NamePill"), TEXT("Image_NameIcon") })
			{
				if (UWidget* NameWidget = WidgetTree->FindWidget(Name)) { NameWidget->SetVisibility(bShowName ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
			}
		}
	}

	BP_OnPanelChanged(Panel);
}

void UCTMainMenuWidget::ApplyOnlineAvailability()
{
	// 빠른 참가, 방 코드 입력은 항상 열어 둠. 온라인을 쓸 수 없으면 누른 뒤 안내 문구가 나옴
	UWidget* Unlocked[] = { Button_Start, Button_JoinByCode, EditableTextBox_RoomCode };
	for (UWidget* Widget : Unlocked)
	{
		if (!Widget) { continue; }
		Widget->SetIsEnabled(true);
		Widget->SetToolTipText(FText::GetEmpty());
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

void UCTMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (CurrentPanel == ECTMenuPanel::Credits)
	{
		CreditsTime += InDeltaTime;
		UpdateCredits(MyGeometry);
	}
}

void UCTMainMenuWidget::UpdateCredits(const FGeometry& MyGeometry)
{
	constexpr float FadeInSeconds = 0.6f;   // 검은 화면이 나타나는 시간
	constexpr float ScrollDelay = 0.6f;     // 나타난 뒤 글이 시작하기까지
	constexpr float ScrollSpeed = 85.f;     // 글이 올라가는 속도
	constexpr float HoldSeconds = 4.f;      // 끝에서 멈춰 있는 시간

	if (Panel_Credits) { Panel_Credits->SetRenderOpacity(FMath::Clamp(CreditsTime / FadeInSeconds, 0.f, 1.f)); }

	UCanvasPanelSlot* ScrollSlot = Panel_CreditsScroll ? Cast<UCanvasPanelSlot>(Panel_CreditsScroll->Slot) : nullptr;
	if (!ScrollSlot) { return; }
	const float ViewHeight = MyGeometry.GetLocalSize().Y;
	const float Total = static_cast<float>(ScrollSlot->GetSize().Y);
	const float StartY = ViewHeight;
	const float EndY = ViewHeight * 0.5f - Total * 0.5f - 60.f; // 마지막 줄들이 화면 가운데에 멈춤
	const float Travel = StartY - EndY;
	const float Moved = FMath::Max(CreditsTime - FadeInSeconds - ScrollDelay, 0.f) * ScrollSpeed;
	ScrollSlot->SetPosition(FVector2D(0.0, StartY - FMath::Min(Moved, Travel)));

	if (Moved >= Travel && CreditsTime - FadeInSeconds - ScrollDelay > Travel / ScrollSpeed + HoldSeconds)
	{
		HandleBackClicked();
	}
}

FReply UCTMainMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 크레딧 중 어디든 누르면 닫음
	if (CurrentPanel == ECTMenuPanel::Credits)
	{
		HandleBackClicked();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCTMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (CurrentPanel == ECTMenuPanel::Credits)
	{
		HandleBackClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCTMainMenuWidget::HandleCreditsClicked()
{
	ShowPanel(ECTMenuPanel::Credits);
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
