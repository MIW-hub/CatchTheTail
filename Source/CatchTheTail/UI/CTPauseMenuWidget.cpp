// Copyright Moon Party Games. All Rights Reserved.
#include "CTPauseMenuWidget.h"
#include "CTButtonHoverEffect.h"
#include "CTSettingsWidget.h"
#include "../Core/CTAudioSubsystem.h"
#include "../Core/CTGameInstance.h"
#include "../Core/CTGameState.h"
#include "../Player/CTPlayerController.h"
#include "../Player/CTPlayerState.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

UCTPauseMenuWidget::UCTPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본 설정 화면 (WBP_CTSettingsWidget). 블루프린트에서 바꿀 수 있음
	static ConstructorHelpers::FClassFinder<UCTSettingsWidget> SettingsFinder(TEXT("/Game/UI/WBP_CTSettingsWidget"));
	if (SettingsFinder.Succeeded())
	{
		SettingsWidgetClass = SettingsFinder.Class;
	}
}

void UCTPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UCTButtonHoverEffect::Attach(this);
	UCTAudioSubsystem::ApplyUISounds(this);

	if (Button_Resume)     { Button_Resume->OnClicked.AddUniqueDynamic(this, &UCTPauseMenuWidget::HandleResumeClicked); }
	if (Button_Settings)   { Button_Settings->OnClicked.AddUniqueDynamic(this, &UCTPauseMenuWidget::HandleSettingsClicked); }
	if (Button_Forfeit)    { Button_Forfeit->OnClicked.AddUniqueDynamic(this, &UCTPauseMenuWidget::HandleForfeitClicked); }
	if (Button_Quit)       { Button_Quit->OnClicked.AddUniqueDynamic(this, &UCTPauseMenuWidget::HandleQuitClicked); }
	if (Button_ConfirmYes) { Button_ConfirmYes->OnClicked.AddUniqueDynamic(this, &UCTPauseMenuWidget::HandleConfirmYesClicked); }
	if (Button_ConfirmNo)  { Button_ConfirmNo->OnClicked.AddUniqueDynamic(this, &UCTPauseMenuWidget::HandleConfirmNoClicked); }

	CreateSettingsWidget();
	ResetToMain();
}

void UCTPauseMenuWidget::NativeDestruct()
{
	if (Button_Resume)     { Button_Resume->OnClicked.RemoveDynamic(this, &UCTPauseMenuWidget::HandleResumeClicked); }
	if (Button_Settings)   { Button_Settings->OnClicked.RemoveDynamic(this, &UCTPauseMenuWidget::HandleSettingsClicked); }
	if (Button_Forfeit)    { Button_Forfeit->OnClicked.RemoveDynamic(this, &UCTPauseMenuWidget::HandleForfeitClicked); }
	if (Button_Quit)       { Button_Quit->OnClicked.RemoveDynamic(this, &UCTPauseMenuWidget::HandleQuitClicked); }
	if (Button_ConfirmYes) { Button_ConfirmYes->OnClicked.RemoveDynamic(this, &UCTPauseMenuWidget::HandleConfirmYesClicked); }
	if (Button_ConfirmNo)  { Button_ConfirmNo->OnClicked.RemoveDynamic(this, &UCTPauseMenuWidget::HandleConfirmNoClicked); }

	Super::NativeDestruct();
}

void UCTPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 0.25f;
		RefreshForfeit();
	}
}

// 설정 화면

void UCTPauseMenuWidget::CreateSettingsWidget()
{
	if (SettingsWidget || !SettingsWidgetClass) { return; }

	UPanelWidget* Container = Cast<UPanelWidget>(Panel_Settings);
	if (!Container)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 일시정지 메뉴에 설정 화면을 넣을 Panel_Settings가 없거나 패널 위젯이 아닙니다."));
		return;
	}

	SettingsWidget = CreateWidget<UCTSettingsWidget>(this, SettingsWidgetClass);
	if (!SettingsWidget) { return; }

	UPanelSlot* PanelSlot = Container->AddChild(SettingsWidget);

	// 설정 화면은 자체 배경과 가운데 창 구조라서 패널을 가득 채움
	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(PanelSlot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PanelSlot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));
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

	// 경기 중에는 닉네임 변경을 막음
	SettingsWidget->SetNameChangeHidden(true);

	// 설정 화면의 닫기 = 메뉴 첫 화면으로
	SettingsWidget->OnCloseRequested.AddUniqueDynamic(this, &UCTPauseMenuWidget::HandleSettingsClosed);
}

// 화면 전환

void UCTPauseMenuWidget::ShowPanel(EPanel Panel)
{
	CurrentPanel = Panel;

	if (Panel_Main)     { Panel_Main->SetVisibility(Panel == EPanel::Main ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed); }
	if (Panel_Settings) { Panel_Settings->SetVisibility(Panel == EPanel::Settings ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed); }
}

void UCTPauseMenuWidget::ResetToMain()
{
	CloseConfirm();
	ShowPanel(EPanel::Main);
	RefreshForfeit();
	RefreshTimer = 0.25f;
}

bool UCTPauseMenuWidget::HandleBack()
{
	// 확인 창이 떠 있으면 취소
	if (PendingAction != EPendingAction::None)
	{
		CloseConfirm();
		return true;
	}

	// 설정에서는 첫 화면으로
	if (CurrentPanel == EPanel::Settings)
	{
		ShowPanel(EPanel::Main);
		return true;
	}

	return false;
}

// 기권

void UCTPauseMenuWidget::RefreshForfeit()
{
	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
	const ACTPlayerState* MyState = GetOwningPlayerState<ACTPlayerState>();

	bool bCanForfeit = false;
	FText Info;

	if (!CTGameState || CTGameState->GetPhase() != ECTGamePhase::Playing)
	{
		Info = NSLOCTEXT("CT", "Pause_ForfeitNotPlaying", "경기 중에만 기권할 수 있어요.");
	}
	else if (MyState && MyState->IsForfeited())
	{
		Info = NSLOCTEXT("CT", "Pause_ForfeitDone", "이미 기권했어요.");
	}
	else if (!CTGameState->CanForfeitNow())
	{
		const int32 Remaining = FMath::CeilToInt(CTGameState->GetForfeitUnlockRemainingSeconds());
		Info = FText::Format(NSLOCTEXT("CT", "Pause_ForfeitLocked", "경기 시간이 절반 지나면 기권할 수 있어요. ({0}:{1} 후)"),
			FText::AsNumber(Remaining / 60), FText::FromString(FString::Printf(TEXT("%02d"), Remaining % 60)));
	}
	else
	{
		bCanForfeit = true;
		Info = NSLOCTEXT("CT", "Pause_ForfeitReady", "기권하면 이번 경기 순위에서 빠져요.");
	}

	if (Button_Forfeit) { Button_Forfeit->SetIsEnabled(bCanForfeit); }
	if (Text_ForfeitInfo) { Text_ForfeitInfo->SetText(Info); }
}

// 확인 창

void UCTPauseMenuWidget::OpenConfirm(EPendingAction Action)
{
	PendingAction = Action;

	FText Title;
	FText Body;
	if (Action == EPendingAction::Forfeit)
	{
		Title = NSLOCTEXT("CT", "Pause_ForfeitTitle", "기권하시겠어요?");
		Body = NSLOCTEXT("CT", "Pause_ForfeitBody", "기권하면 꼬리를 모두 잃고 이번 경기의 순위에서 제외돼요. 남은 시간에는 유령 상태로 지켜보기만 할 수 있고, 되돌릴 수 없어요.");
	}
	else
	{
		const ACTPlayerState* MyState = GetOwningPlayerState<ACTPlayerState>();
		const UWorld* World = GetWorld();
		const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;
		const bool bInMatch = CTGameState && CTGameState->GetPhase() == ECTGamePhase::Playing;

		Title = NSLOCTEXT("CT", "Pause_QuitTitle", "게임을 종료하시겠어요?");
		if (!bInMatch)
		{
			Body = NSLOCTEXT("CT", "Pause_QuitBodyIdle", "방을 나가 메인 메뉴로 돌아가요.");
		}
		else if (MyState && MyState->IsHost())
		{
			Body = NSLOCTEXT("CT", "Pause_QuitBodyHost", "방장이 나가면 방이 닫혀 모든 플레이어의 경기가 끝나요. 진행 중인 경기 기록은 남지 않고, 다시 들어올 수 없어요.");
		}
		else
		{
			Body = NSLOCTEXT("CT", "Pause_QuitBodyGuest", "경기 도중에 나가면 기권으로 처리돼 순위에서 빠지고, 진행 중인 경기에는 다시 들어올 수 없어요.");
		}
	}

	if (Text_ConfirmTitle) { Text_ConfirmTitle->SetText(Title); }
	if (Text_ConfirmBody)  { Text_ConfirmBody->SetText(Body); }
	if (Panel_Confirm)     { Panel_Confirm->SetVisibility(ESlateVisibility::SelfHitTestInvisible); }
}

void UCTPauseMenuWidget::CloseConfirm()
{
	PendingAction = EPendingAction::None;
	if (Panel_Confirm) { Panel_Confirm->SetVisibility(ESlateVisibility::Collapsed); }
}

// 버튼

void UCTPauseMenuWidget::HandleResumeClicked()
{
	OnCloseRequested.Broadcast();
}

void UCTPauseMenuWidget::HandleSettingsClicked()
{
	ShowPanel(EPanel::Settings);
}

void UCTPauseMenuWidget::HandleSettingsClosed()
{
	ShowPanel(EPanel::Main);
}

void UCTPauseMenuWidget::HandleForfeitClicked()
{
	OpenConfirm(EPendingAction::Forfeit);
}

void UCTPauseMenuWidget::HandleQuitClicked()
{
	OpenConfirm(EPendingAction::Quit);
}

void UCTPauseMenuWidget::HandleConfirmNoClicked()
{
	CloseConfirm();
}

void UCTPauseMenuWidget::HandleConfirmYesClicked()
{
	const EPendingAction Action = PendingAction;
	CloseConfirm();

	if (Action == EPendingAction::Forfeit)
	{
		if (ACTPlayerController* Controller = GetOwningPlayer<ACTPlayerController>())
		{
			Controller->RequestForfeit();
		}
		OnCloseRequested.Broadcast();
	}
	else if (Action == EPendingAction::Quit)
	{
		if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
		{
			GameInstance->LeaveToMainMenu(FText::GetEmpty());
		}
	}
}
