// Copyright Moon Party Games. All Rights Reserved.
// [보류 중] 켜는 방법은 CTGameInstance.h 맨 위 설명을 보세요.
#if 0
#include "CTGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UCTGameInstance::Init()
{
	Super::Init();

	// 접속이 끊기거나 이동에 실패했을 때 알림을 받습니다.
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UCTGameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UCTGameInstance::HandleTravelFailure);
	}
}

void UCTGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

bool UCTGameInstance::IsInMainMenuMap() const
{
	const UWorld* World = GetWorld();
	return World && UGameplayStatics::GetCurrentLevelName(World, /*bRemovePrefixString=*/true) == MainMenuMapName.ToString();
}

// ============================================================
// 방 만들기 / 참가 / 나가기
// ============================================================

void UCTGameInstance::HostGame()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	// 게임 맵을 listen 서버로 엽니다. (방장 = 서버 + 플레이어)
	UGameplayStatics::OpenLevel(World, GameMapName, /*bAbsolute=*/true, TEXT("listen"));
}

void UCTGameInstance::JoinGame(const FString& Address)
{
	FString FinalAddress = Address.TrimStartAndEnd();
	if (FinalAddress.IsEmpty())
	{
		const FText Message = NSLOCTEXT("CT", "Menu_EmptyAddress", "방장의 주소를 입력하세요.");
		PendingMessage = Message;
		OnMenuMessage.Broadcast(Message);
		return;
	}

	// 포트를 안 적었으면 기본 포트를 붙입니다.
	if (!FinalAddress.Contains(TEXT(":")))
	{
		FinalAddress += FString::Printf(TEXT(":%d"), DefaultPort);
	}

	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController) { return; }

	PlayerController->ClientTravel(FinalAddress, ETravelType::TRAVEL_Absolute);
}

void UCTGameInstance::LeaveToMainMenu(FText Message)
{
	PendingMessage = Message;

	UWorld* World = GetWorld();
	if (!World) { return; }

	// 방장이면 방이 닫히고, 클라이언트면 방장과의 연결이 끊깁니다.
	UGameplayStatics::OpenLevel(World, MainMenuMapName, /*bAbsolute=*/true);
}

void UCTGameInstance::QuitToDesktop()
{
	UKismetSystemLibrary::QuitGame(this, GetFirstLocalPlayerController(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
}

// ============================================================
// 오류 처리
// ============================================================

void UCTGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 이미 메뉴에 있으면 별도 처리가 필요 없습니다. (참가 실패 문구는 아래 HandleTravelFailure)
	if (IsInMainMenuMap()) { return; }

	LeaveToMainMenu(NSLOCTEXT("CT", "Menu_Disconnected", "방장과의 연결이 끊어졌습니다."));
}

void UCTGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	const FText Message = NSLOCTEXT("CT", "Menu_JoinFailed", "방에 접속하지 못했습니다. 주소와 방장의 방 상태를 확인하세요.");
	PendingMessage = Message;
	OnMenuMessage.Broadcast(Message);

	if (!IsInMainMenuMap())
	{
		LeaveToMainMenu(Message);
	}
}
#endif // 보류: 메인 메뉴 / 방 만들기
