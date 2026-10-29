// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyGameMode.h"
#include "CTGameInstance.h"
#include "CTGameState.h"
#include "CTSessionSubsystem.h"
#include "CTMapSettings.h"
#include "../Player/CTCharacter.h"
#include "../Player/CTLobbyPlayerController.h"
#include "../Player/CTPlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACTLobbyGameMode::ACTLobbyGameMode()
{
	DefaultPawnClass = ACTCharacter::StaticClass();
	PlayerControllerClass = ACTLobbyPlayerController::StaticClass();
	GameStateClass = ACTGameState::StaticClass();
	PlayerStateClass = ACTPlayerState::StaticClass();

	// 맵 이동 시 PlayerState 유지
	bUseSeamlessTravel = true;
}

namespace
{
	// 0/O, 1/I/L 뺀 6글자 방 코드
	FString MakeRoomCode()
	{
		static const TCHAR Alphabet[] = TEXT("ABCDEFGHJKMNPQRSTUVWXYZ23456789");
		constexpr int32 AlphabetLength = UE_ARRAY_COUNT(Alphabet) - 1;

		FString Code;
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Code.AppendChar(Alphabet[FMath::RandRange(0, AlphabetLength - 1)]);
		}
		return Code;
	}
}

void ACTLobbyGameMode::InitGameState()
{
	Super::InitGameState();

	if (ACTGameState* CTGameState = GetGameState<ACTGameState>())
	{
		CTGameState->SetPhase(ECTGamePhase::Lobby);
		CTGameState->SetLobbyMinPlayers(MinPlayersToStart);

		// 방 코드는 방이 열린 동안 유지
		UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
		FString RoomCode = GameInstance ? GameInstance->GetStoredRoomCode() : FString();
		if (RoomCode.IsEmpty())
		{
			RoomCode = MakeRoomCode();
			if (GameInstance) { GameInstance->SetStoredRoomCode(RoomCode); }
		}
		CTGameState->SetRoomCode(RoomCode);

		// 코드로 찾을 수 있게 온라인 방 개설
		if (UCTSessionSubsystem* Session = GameInstance ? GameInstance->GetSubsystem<UCTSessionSubsystem>() : nullptr)
		{
			Session->CreateRoom(RoomCode);
		}

		// 맵 선택 유지, 처음이면 첫 맵
		int32 MapIndex = GameInstance ? GameInstance->GetStoredMapIndex() : INDEX_NONE;
		FCTMapInfo Ignored;
		if (!UCTMapSettings::GetMapInfo(MapIndex, Ignored))
		{
			MapIndex = UCTMapSettings::GetFirstEnabledIndex();
			if (GameInstance) { GameInstance->SetStoredMapIndex(MapIndex); }
		}
		CTGameState->SetSelectedMapIndex(MapIndex);

		// 경기 설정 유지, 처음이면 기본값
		FCTMatchSettings MatchSettings;
		if (GameInstance && GameInstance->HasStoredMatchSettings())
		{
			MatchSettings = GameInstance->GetStoredMatchSettings();
		}
		MatchSettings.Sanitize();
		CTGameState->SetMatchSettings(MatchSettings);
		if (GameInstance) { GameInstance->SetStoredMatchSettings(MatchSettings); }

		CTGameState->SetStartCountdown(0.0);
	}
}

void ACTLobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty()) { return; }

	// 시작 확정 후 입장 불가
	if (GetWorldTimerManager().IsTimerActive(StartTimerHandle))
	{
		ErrorMessage = CTJoinReject::LobbyStarting;
		UE_LOG(LogTemp, Log, TEXT("[CT] 입장 거절: 로비 시작 중 (%s)"), *Address);
	}

	// 최대 인원 초과 거절
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (CTGameState && GetNumPlayers() >= CTGameState->GetMatchSettings().MaxPlayers)
	{
		ErrorMessage = CTJoinReject::LobbyFull;
		UE_LOG(LogTemp, Log, TEXT("[CT] 입장 거절: 최대 인원 (%s)"), *Address);
	}
}

void ACTLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// PlayerState 정리 후 재확인
	GetWorldTimerManager().SetTimer(RecheckTimerHandle, this, &ACTLobbyGameMode::RecheckStartCondition, 0.3f, false);
}

void ACTLobbyGameMode::RecheckStartCondition()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || !GetWorldTimerManager().IsTimerActive(StartTimerHandle)) { return; }

	// 인원 부족 시 시작 취소
	if (!CTGameState->CanStartLobby())
	{
		GetWorldTimerManager().ClearTimer(StartTimerHandle);
		CTGameState->SetStartCountdown(0.0);
		UE_LOG(LogTemp, Log, TEXT("[CT] 인원 이탈로 로비 시작 취소"));
	}
}

void ACTLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	InitLobbyPlayer(NewPlayer);
}

void ACTLobbyGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);
	InitLobbyPlayer(Cast<APlayerController>(C));
}

void ACTLobbyGameMode::InitLobbyPlayer(APlayerController* NewPlayer)
{
	ACTPlayerState* NewPlayerState = NewPlayer ? NewPlayer->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!NewPlayerState) { return; }

	// listen 서버의 로컬 플레이어가 방장
	NewPlayerState->SetIsHost(NewPlayer->IsLocalController());
	NewPlayerState->SetReady(false);

	// 직전 게임의 기록 초기화
	NewPlayerState->SetColors(ECTColor::None, ECTColor::None);
	NewPlayerState->SetIsGhost(false);
	NewPlayerState->SetTailCount(LobbyTailCount);
}

void ACTLobbyGameMode::SetPlayerReady(APlayerController* Requester, bool bReady)
{
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	ACTPlayerState* RequesterState = Requester ? Requester->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!CTGameState || !RequesterState) { return; }

	// 방장은 준비 불필요, 시작 확정 후 변경 불가
	if (RequesterState->IsHost()) { return; }
	if (CTGameState->GetLobbyStartRemainingSeconds() > 0.f) { return; }

	RequesterState->SetReady(bReady);
}

bool ACTLobbyGameMode::SelectMap(APlayerController* Requester, int32 MapIndex)
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	const ACTPlayerState* RequesterState = Requester ? Requester->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!CTGameState || !RequesterState || !RequesterState->IsHost()) { return false; }

	// 시작 확정 후 변경 불가
	if (CTGameState->GetLobbyStartRemainingSeconds() > 0.f || GetWorldTimerManager().IsTimerActive(StartTimerHandle)) { return false; }

	// 목록에 있고 사용 가능한 맵만
	FCTMapInfo Ignored;
	if (!UCTMapSettings::GetMapInfo(MapIndex, Ignored)) { return false; }

	CTGameState->SetSelectedMapIndex(MapIndex);
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->SetStoredMapIndex(MapIndex);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 맵 선택: %d"), MapIndex);
	return true;
}

bool ACTLobbyGameMode::ChangeMatchSetting(APlayerController* Requester, ECTMatchSetting Setting, int32 Direction)
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	const ACTPlayerState* RequesterState = Requester ? Requester->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!CTGameState || !RequesterState || !RequesterState->IsHost()) { return false; }

	// 시작 확정 후 변경 불가
	if (CTGameState->GetLobbyStartRemainingSeconds() > 0.f || GetWorldTimerManager().IsTimerActive(StartTimerHandle)) { return false; }
	if (static_cast<int32>(Setting) < 0 || Setting >= ECTMatchSetting::Count) { return false; }

	FCTMatchSettings NewSettings = CTGameState->GetMatchSettings();
	if (!NewSettings.Step(Setting, Direction)) { return false; }

	// 현재 인원/경기 시간에 맞지 않으면 불가
	if (!NewSettings.IsAllowed(GetNumPlayers())) { return false; }

	CTGameState->SetMatchSettings(NewSettings);
	if (UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>())
	{
		GameInstance->SetStoredMatchSettings(NewSettings);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 경기 설정 변경: %s = %d"), *FCTMatchSettings::GetKey(Setting).ToString(), NewSettings.Get(Setting));
	return true;
}

bool ACTLobbyGameMode::TryStartGame(APlayerController* Requester)
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	const ACTPlayerState* RequesterState = Requester ? Requester->GetPlayerState<ACTPlayerState>() : nullptr;
	if (!CTGameState || !RequesterState || !RequesterState->IsHost()) { return false; }

	if (CTGameState->GetPhase() != ECTGamePhase::Lobby) { return false; }
	if (GetWorldTimerManager().IsTimerActive(StartTimerHandle)) { return false; }
	if (!CTGameState->CanStartLobby()) { return false; }

	CTGameState->SetStartCountdown(CTGameState->GetServerWorldTimeSeconds() + StartDelaySeconds);
	GetWorldTimerManager().SetTimer(StartTimerHandle, this, &ACTLobbyGameMode::TravelToGameMap, FMath::Max(StartDelaySeconds, 0.1f), false);

	UE_LOG(LogTemp, Log, TEXT("[CT] 로비 시작: %.1f초 뒤 게임 맵으로 이동"), StartDelaySeconds);
	return true;
}

void ACTLobbyGameMode::BroadcastChat(APlayerController* Sender, const FString& Message)
{
	const APlayerState* SenderState = Sender ? Sender->PlayerState : nullptr;
	const FString SenderName = SenderState ? SenderState->GetPlayerName() : FString();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ACTLobbyPlayerController* Receiver = Cast<ACTLobbyPlayerController>(It->Get()))
		{
			Receiver->ClientReceiveChat(SenderName, Message);
		}
	}
}

void ACTLobbyGameMode::TravelToGameMap()
{
	const UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
	UWorld* World = GetWorld();
	if (!GameInstance || !World) { return; }

	// 선택 맵, 목록 비면 기본 게임 맵
	const ACTGameState* CTGameState = GetGameState<ACTGameState>();
	FString MapName = CTGameState ? UCTMapSettings::GetTravelName(CTGameState->GetSelectedMapIndex()) : FString();
	if (MapName.IsEmpty())
	{
		MapName = GameInstance->GetGameMapName().ToString();
	}
	UE_LOG(LogTemp, Log, TEXT("[CT] 게임 맵으로 이동: %s"), *MapName);
	World->ServerTravel(MapName);
}
