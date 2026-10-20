// Copyright Moon Party Games. All Rights Reserved.
#include "CTGameInstance.h"
#include "CTProfileSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UCTGameInstance::Init()
{
	Super::Init();

	// 이 컴퓨터에 저장된 프로필(이름)을 불러옵니다.
	LoadProfile();

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
// 플레이어 이름 (저장 / 검사)
// ============================================================

namespace
{
	const TCHAR* const CTProfileSlotName = TEXT("CTProfile");

	/** 서버 접속 주소에 넣어도 안전한 글자인가? (한글, 영문, 숫자, _, -) */
	bool IsAllowedNameChar(TCHAR Ch)
	{
		return FChar::IsAlnum(Ch) && Ch < 128   // 영문, 숫자
			|| Ch == TEXT('_') || Ch == TEXT('-')
			|| (Ch >= 0xAC00 && Ch <= 0xD7A3);   // 한글 음절
	}
}

void UCTGameInstance::LoadProfile()
{
	PlayerName.Reset();

	if (!UGameplayStatics::DoesSaveGameExist(CTProfileSlotName, 0)) { return; }

	if (const UCTProfileSaveGame* Profile = Cast<UCTProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(CTProfileSlotName, 0)))
	{
		// 저장된 이름도 규칙을 다시 확인합니다. (파일을 직접 고쳤거나 규칙이 바뀐 경우)
		FText Ignored;
		if (ValidatePlayerName(Profile->PlayerName, Ignored))
		{
			PlayerName = Profile->PlayerName;
		}
	}
}

void UCTGameInstance::SaveProfile() const
{
	UCTProfileSaveGame* Profile = Cast<UCTProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UCTProfileSaveGame::StaticClass()));
	if (!Profile) { return; }

	Profile->PlayerName = PlayerName;
	UGameplayStatics::SaveGameToSlot(Profile, CTProfileSlotName, 0);
}

bool UCTGameInstance::ValidatePlayerName(const FString& Name, FText& OutError) const
{
	const FString Trimmed = Name.TrimStartAndEnd();

	if (Trimmed.Len() < MinNameLength || Trimmed.Len() > MaxNameLength)
	{
		OutError = FText::Format(NSLOCTEXT("CT", "Name_Length", "이름은 {0}~{1}자로 입력하세요."),
			FText::AsNumber(MinNameLength), FText::AsNumber(MaxNameLength));
		return false;
	}

	for (const TCHAR Ch : Trimmed)
	{
		if (!IsAllowedNameChar(Ch))
		{
			OutError = NSLOCTEXT("CT", "Name_Chars", "한글, 영문, 숫자, _, - 만 쓸 수 있습니다. (공백과 특수문자 불가)");
			return false;
		}
	}

	OutError = FText::GetEmpty();
	return true;
}

bool UCTGameInstance::SetPlayerName(const FString& NewName, FText& OutError)
{
	// 이름은 한 번 정하면 바꿀 수 없습니다. (바꾸려면 ChangePlayerName)
	if (HasPlayerName())
	{
		OutError = NSLOCTEXT("CT", "Name_AlreadySet", "이름은 한 번만 정할 수 있습니다.");
		return false;
	}

	return StoreValidatedName(NewName, OutError);
}

bool UCTGameInstance::ChangePlayerName(const FString& NewName, FText& OutError)
{
	// 바꿔도 되는지는 호출하는 쪽(설정: Allow Name Change)이 확인합니다.
	return StoreValidatedName(NewName, OutError);
}

bool UCTGameInstance::StoreValidatedName(const FString& NewName, FText& OutError)
{
	if (!ValidatePlayerName(NewName, OutError))
	{
		return false;
	}

	PlayerName = NewName.TrimStartAndEnd();
	SaveProfile();
	return true;
}

void UCTGameInstance::CTResetPlayerName()
{
#if !UE_BUILD_SHIPPING
	PlayerName.Reset();
	UGameplayStatics::DeleteGameInSlot(CTProfileSlotName, 0);
	UE_LOG(LogTemp, Log, TEXT("[CT] 저장된 이름을 지웠습니다. 다시 메뉴에 들어가면 이름을 입력합니다."));
#endif
}

FString UCTGameInstance::MakeNameOption() const
{
	return HasPlayerName() ? FString::Printf(TEXT("?Name=%s"), *PlayerName) : FString();
}

// ============================================================
// 방 만들기 / 참가 / 나가기
// ============================================================

void UCTGameInstance::HostGame()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	// 게임 맵을 listen 서버로 엽니다. (방장 = 서버 + 플레이어)
	// 저장한 이름을 접속 옵션으로 넘겨서 점수판에 내 이름이 나오게 합니다.
	UGameplayStatics::OpenLevel(World, GameMapName, /*bAbsolute=*/true, FString(TEXT("listen")) + MakeNameOption());
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

	// 저장한 이름을 접속 옵션으로 넘깁니다.
	PlayerController->ClientTravel(FinalAddress + MakeNameOption(), ETravelType::TRAVEL_Absolute);
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
