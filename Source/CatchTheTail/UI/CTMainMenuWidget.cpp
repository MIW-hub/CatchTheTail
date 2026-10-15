// Copyright Moon Party Games. All Rights Reserved.
// [보류 중] 켜는 방법은 CTGameInstance.h 맨 위 설명을 보세요.
#if 0
#include "CTMainMenuWidget.h"
#include "../Core/CTGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void UCTMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Host) { Button_Host->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleHostClicked); }
	if (Button_Join) { Button_Join->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleJoinClicked); }
	if (Button_Quit) { Button_Quit->OnClicked.AddUniqueDynamic(this, &UCTMainMenuWidget::HandleQuitClicked); }

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

void UCTMainMenuWidget::HandleQuitClicked()
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
#endif // 보류: 메인 메뉴 / 방 만들기
