// Copyright Moon Party Games. All Rights Reserved.
#include "CTGameInstance.h"
#include "HAL/PlatformTime.h"
#include "../Type/CTTypes.h"
#include "CTProfileSaveGame.h"
#include "CTSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UCTGameInstance::Init()
{
	Super::Init();

	// 저장된 프로필 로드
	LoadProfile();

	// 접속 끊김/이동 실패 알림
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

// 플레이어 이름

namespace
{
	const TCHAR* const CTProfileSlotName = TEXT("CTProfile");

	// 접속 주소에 안전한 글자인가
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
		// 규칙 재검사 (파일 수정/규칙 변경 대비)
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
	// 최초 1회만. 변경은 ChangePlayerName
	if (HasPlayerName())
	{
		OutError = NSLOCTEXT("CT", "Name_AlreadySet", "이름은 한 번만 정할 수 있습니다.");
		return false;
	}

	return StoreValidatedName(NewName, OutError);
}

bool UCTGameInstance::ChangePlayerName(const FString& NewName, FText& OutError)
{
	// 허용 여부는 호출 측(Allow Name Change)이 확인
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
	OnPlayerNameChanged.Broadcast();
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

// 방 만들기 / 참가 / 나가기

void UCTGameInstance::HostGame()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	// listen 서버로 열기. 이름은 접속 옵션으로 전달
	// 새 방이니 이전 방 코드는 버림
	StoredMapIndex = INDEX_NONE;
	StoredRoomCode.Reset();

	const FName FirstMap = LobbyMapName.IsNone() ? GameMapName : LobbyMapName;
	UGameplayStatics::OpenLevel(World, FirstMap, /*bAbsolute=*/true, FString(TEXT("listen")) + MakeNameOption());
}

void UCTGameInstance::ShowMenuMessage(const FText& Message)
{
	PendingMessage = Message;
	OnMenuMessage.Broadcast(Message);
}

void UCTGameInstance::QuickJoin()
{
	UCTSessionSubsystem* Session = GetSubsystem<UCTSessionSubsystem>();
	if (!bOnlineSessionAvailable || !Session || !Session->IsAvailable())
	{
		ShowMenuMessage(NSLOCTEXT("CT", "Menu_OnlineUnavailable", "스팀 연결 후 사용할 수 있습니다."));
		return;
	}
	Session->QuickJoin();
}

void UCTGameInstance::JoinByRoomCode(const FString& RoomCode)
{
	UCTSessionSubsystem* Session = GetSubsystem<UCTSessionSubsystem>();
	if (!bOnlineSessionAvailable || !Session || !Session->IsAvailable())
	{
		ShowMenuMessage(NSLOCTEXT("CT", "Menu_OnlineUnavailable", "스팀 연결 후 사용할 수 있습니다."));
		return;
	}
	Session->JoinByRoomCode(RoomCode);
}

void UCTGameInstance::LeaveToMainMenu(FText Message)
{
	PendingMessage = Message;
	StoredRoomCode.Reset();
	StoredMapIndex = INDEX_NONE;

	if (UCTSessionSubsystem* Session = GetSubsystem<UCTSessionSubsystem>()) { Session->CloseRoom(); }

	UWorld* World = GetWorld();
	if (!World) { return; }

	// 방장이면 방 닫힘, 클라이언트면 연결 끊김
	UGameplayStatics::OpenLevel(World, MainMenuMapName, /*bAbsolute=*/true);
}

void UCTGameInstance::QuitToDesktop()
{
	if (UCTSessionSubsystem* Session = GetSubsystem<UCTSessionSubsystem>()) { Session->CloseRoom(); }
	UKismetSystemLibrary::QuitGame(this, GetFirstLocalPlayerController(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
}

// 오류 처리

void UCTGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 방 입장 중 실패 (방장이 거절했거나 응답이 없음). 이유를 메뉴에 안내
	if (FailureType == ENetworkFailure::PendingConnectionFailure)
	{
		const FText Message = MakeJoinFailureMessage(ErrorString);
		LastJoinRejectTime = FPlatformTime::Seconds();
		PendingMessage = Message;
		OnMenuMessage.Broadcast(Message);
		return;
	}

	// 메뉴면 처리 불필요
	if (IsInMainMenuMap()) { return; }

	// 방장(서버) 쪽 오류는 무시. 참가자만 방장과의 연결이 끊긴 것으로 처리
	const UWorld* FailedWorld = World ? World : GetWorld();
	if (!FailedWorld || FailedWorld->GetNetMode() != NM_Client) { return; }

	UE_LOG(LogTemp, Log, TEXT("[CT] 방장과의 연결 끊김: %s"), *ErrorString);
	LeaveToMainMenu(NSLOCTEXT("CT", "Menu_Disconnected", "방장이 방을 닫았거나 연결이 끊어졌습니다."));
}

FText UCTGameInstance::MakeJoinFailureMessage(const FString& ErrorString) const
{
	if (ErrorString.Contains(CTJoinReject::GameInProgress))
	{
		return NSLOCTEXT("CT", "Menu_JoinInProgress", "게임이 이미 진행 중이라 입장할 수 없습니다.");
	}
	if (ErrorString.Contains(CTJoinReject::LobbyStarting))
	{
		return NSLOCTEXT("CT", "Menu_JoinStarting", "곧 게임이 시작되어 입장할 수 없습니다.");
	}

	// 엔진 기본 인원 초과 메시지 (Server full 등)
	if (ErrorString.Contains(TEXT("full"), ESearchCase::IgnoreCase) || ErrorString.Contains(TEXT("MaxedOut"), ESearchCase::IgnoreCase))
	{
		return NSLOCTEXT("CT", "Menu_JoinFull", "방이 가득 차서 입장할 수 없습니다.");
	}

	return NSLOCTEXT("CT", "Menu_JoinFailed", "방에 접속하지 못했습니다. 주소와 방장의 방 상태를 확인하세요.");
}

void UCTGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	// 방금 거절 사유를 이미 안내했으면 일반 안내로 덮어쓰지 않음
	if (FPlatformTime::Seconds() - LastJoinRejectTime < 3.0) { return; }

	const FText Message = NSLOCTEXT("CT", "Menu_JoinFailed", "방에 접속하지 못했습니다. 주소와 방장의 방 상태를 확인하세요.");
	PendingMessage = Message;
	OnMenuMessage.Broadcast(Message);

	if (!IsInMainMenuMap())
	{
		LeaveToMainMenu(Message);
	}
}
