// Copyright Moon Party Games. All Rights Reserved.
#include "CTSessionSubsystem.h"
#include "CTGameInstance.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "Online/OnlineSessionNames.h"

namespace
{
	// 세션의 방 코드 설정 이름
	const FName RoomCodeKey(TEXT("CTROOMCODE"));

	constexpr int32 MaxRoomPlayers = 8;
	constexpr int32 MaxSearchResults = 50;

	IOnlineSubsystem* FindSubsystem()
	{
		// 스팀 없으면 LAN
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
		return Subsystem ? Subsystem : IOnlineSubsystem::Get(NULL_SUBSYSTEM);
	}

	FString NormalizeCode(const FString& Code)
	{
		return Code.TrimStartAndEnd().ToUpper();
	}

	bool ReadRoomCode(const FOnlineSessionSearchResult& Result, FString& OutCode)
	{
		const FOnlineSessionSetting* Setting = Result.Session.SessionSettings.Settings.Find(RoomCodeKey);
		if (!Setting || Setting->Data.GetType() != EOnlineKeyValuePairDataType::String) { return false; }
		Setting->Data.GetValue(OutCode);
		return true;
	}
}

void UCTSessionSubsystem::Deinitialize()
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
	Super::Deinitialize();
}

IOnlineSessionPtr UCTSessionSubsystem::GetSessionInterface() const
{
	IOnlineSubsystem* Subsystem = FindSubsystem();
	return Subsystem ? Subsystem->GetSessionInterface() : nullptr;
}

bool UCTSessionSubsystem::IsAvailable() const
{
	return GetSessionInterface().IsValid();
}

FString UCTSessionSubsystem::GetServiceName() const
{
	IOnlineSubsystem* Subsystem = FindSubsystem();
	return Subsystem ? Subsystem->GetSubsystemName().ToString() : FString();
}

bool UCTSessionSubsystem::IsLan() const
{
	IOnlineSubsystem* Subsystem = FindSubsystem();
	return !Subsystem || Subsystem->GetSubsystemName() == NULL_SUBSYSTEM;
}

void UCTSessionSubsystem::ShowMessage(const FText& Message) const
{
	if (UCTGameInstance* GameInstance = Cast<UCTGameInstance>(GetGameInstance()))
	{
		GameInstance->ShowMenuMessage(Message);
	}
}

void UCTSessionSubsystem::EndBusy()
{
	bBusy = false;
	PendingMode = EJoinMode::None;
	PendingRoomCode.Reset();
}

// 방 열기 / 닫기

void UCTSessionSubsystem::CreateRoom(const FString& RoomCode)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || RoomCode.IsEmpty()) { return; }

	// 로비 복귀 후에도 같은 방 유지
	if (Sessions->GetNamedSession(NAME_GameSession)) { return; }

	const bool bLan = IsLan();

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = MaxRoomPlayers;
	Settings.NumPrivateConnections = 0;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bAllowInvites = true;
	Settings.bIsLANMatch = bLan;
	Settings.bUsesPresence = !bLan;
	Settings.bUseLobbiesIfAvailable = !bLan;
	Settings.bIsDedicated = false;
	Settings.Set(RoomCodeKey, NormalizeCode(RoomCode), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UCTSessionSubsystem::HandleCreateSessionComplete));
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		UE_LOG(LogTemp, Warning, TEXT("[CT] 방 열기 실패 (%s)"), *GetServiceName());
	}
}

void UCTSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
	UE_LOG(LogTemp, Log, TEXT("[CT] 방 열기 %s (%s)"), bWasSuccessful ? TEXT("성공") : TEXT("실패"), *GetServiceName());
}

void UCTSessionSubsystem::CloseRoom()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession)) { return; }

	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UCTSessionSubsystem::HandleDestroySessionComplete));
	if (!Sessions->DestroySession(NAME_GameSession))
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
}

void UCTSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
}

// 찾기 / 입장

void UCTSessionSubsystem::JoinByRoomCode(const FString& RoomCode)
{
	const FString Code = NormalizeCode(RoomCode);
	if (Code.IsEmpty())
	{
		ShowMessage(NSLOCTEXT("CT", "Menu_EmptyRoomCode", "방 코드를 입력하세요."));
		return;
	}
	StartSearch(EJoinMode::Code, Code);
}

void UCTSessionSubsystem::QuickJoin()
{
	StartSearch(EJoinMode::Quick, FString());
}

void UCTSessionSubsystem::StartSearch(EJoinMode Mode, const FString& RoomCode)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		ShowMessage(NSLOCTEXT("CT", "Menu_OnlineUnavailable", "스팀 연결 후 사용할 수 있습니다."));
		return;
	}
	if (bBusy) { return; }

	// 남은 이전 방 정리 후 재시도
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		bBusy = true;
		TWeakObjectPtr<UCTSessionSubsystem> WeakThis(this);
		Sessions->DestroySession(NAME_GameSession, FOnDestroySessionCompleteDelegate::CreateLambda([WeakThis, Mode, RoomCode](FName, bool)
		{
			if (UCTSessionSubsystem* Self = WeakThis.Get())
			{
				Self->bBusy = false;
				Self->StartSearch(Mode, RoomCode);
			}
		}));
		return;
	}

	bBusy = true;
	PendingMode = Mode;
	PendingRoomCode = RoomCode;
	ShowMessage(NSLOCTEXT("CT", "Menu_Searching", "방을 찾는 중입니다..."));

	const bool bLan = IsLan();
	Search = MakeShared<FOnlineSessionSearch>();
	Search->MaxSearchResults = MaxSearchResults;
	Search->bIsLanQuery = bLan;
	if (!bLan)
	{
				Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UCTSessionSubsystem::HandleFindSessionsComplete));
	if (!Sessions->FindSessions(0, Search.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		EndBusy();
		ShowMessage(NSLOCTEXT("CT", "Menu_JoinFailed", "방에 입장하지 못했습니다."));
	}
}

void UCTSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !Search.IsValid())
	{
		EndBusy();
		return;
	}
	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);

	// 입장 가능한 방만 (코드 있으면 코드 일치)
	TArray<const FOnlineSessionSearchResult*> Candidates;
	if (bWasSuccessful)
	{
		for (const FOnlineSessionSearchResult& Result : Search->SearchResults)
		{
			if (!Result.IsValid() || Result.Session.NumOpenPublicConnections <= 0) { continue; }

			FString Code;
			if (!ReadRoomCode(Result, Code)) { continue; }
			if (PendingMode == EJoinMode::Code && NormalizeCode(Code) != PendingRoomCode) { continue; }

			Candidates.Add(&Result);
		}
	}

	if (Candidates.IsEmpty())
	{
		const FText Message = PendingMode == EJoinMode::Code
			? NSLOCTEXT("CT", "Menu_RoomNotFound", "해당 방 코드를 찾을 수 없습니다.")
			: NSLOCTEXT("CT", "Menu_NoOpenRoom", "참가할 수 있는 방이 없습니다.");
		EndBusy();
		ShowMessage(Message);
		return;
	}

	const FOnlineSessionSearchResult& Chosen = *Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UCTSessionSubsystem::HandleJoinSessionComplete));
	if (!Sessions->JoinSession(0, NAME_GameSession, Chosen))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		EndBusy();
		ShowMessage(NSLOCTEXT("CT", "Menu_JoinFailed", "방에 입장하지 못했습니다."));
	}
}

void UCTSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}
	EndBusy();

	if (Result != EOnJoinSessionCompleteResult::Success || !Sessions.IsValid())
	{
		const FText Message = Result == EOnJoinSessionCompleteResult::SessionIsFull
			? NSLOCTEXT("CT", "Menu_RoomFull", "방이 가득 찼습니다.")
			: NSLOCTEXT("CT", "Menu_JoinFailed", "방에 입장하지 못했습니다.");
		ShowMessage(Message);
		return;
	}

	FString ConnectString;
	UGameInstance* GameInstance = GetGameInstance();
	APlayerController* Controller = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	if (!Controller || !Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		Sessions->DestroySession(SessionName);
		ShowMessage(NSLOCTEXT("CT", "Menu_JoinFailed", "방에 입장하지 못했습니다."));
		return;
	}

	// 이름은 접속 옵션으로 전달
	const UCTGameInstance* CTGameInstance = Cast<UCTGameInstance>(GameInstance);
	const FString NameOption = CTGameInstance ? CTGameInstance->MakeNameOption() : FString();
	UE_LOG(LogTemp, Log, TEXT("[CT] 방 입장: %s"), *ConnectString);
	Controller->ClientTravel(ConnectString + NameOption, TRAVEL_Absolute);
}
