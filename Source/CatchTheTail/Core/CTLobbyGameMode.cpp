// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyGameMode.h"
#include "CTGameInstance.h"
#include "CTGameState.h"
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

	// 게임 맵으로 이동할 때 PlayerState(이름 등)를 유지
	bUseSeamlessTravel = true;
}

namespace
{
	// 헷갈리는 글자(0/O, 1/I/L)를 뺀 6글자 방 코드
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

		// 방 코드는 방이 열려 있는 동안 유지 (게임을 하고 돌아와도 같은 코드)
		UCTGameInstance* GameInstance = GetGameInstance<UCTGameInstance>();
		FString RoomCode = GameInstance ? GameInstance->GetStoredRoomCode() : FString();
		if (RoomCode.IsEmpty())
		{
			RoomCode = MakeRoomCode();
			if (GameInstance) { GameInstance->SetStoredRoomCode(RoomCode); }
		}
		CTGameState->SetRoomCode(RoomCode);

		CTGameState->SetStartCountdown(0.0);
	}
}

void ACTLobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty()) { return; }

	// 시작이 정해진 뒤에는 맵 이동 중이라 새로 들어올 수 없음
	if (GetWorldTimerManager().IsTimerActive(StartTimerHandle))
	{
		ErrorMessage = CTJoinReject::LobbyStarting;
		UE_LOG(LogTemp, Log, TEXT("[CT] 입장 거절: 로비 시작 중 (%s)"), *Address);
	}
}

void ACTLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// 나간 PlayerState가 정리된 뒤에 조건을 다시 확인
	GetWorldTimerManager().SetTimer(RecheckTimerHandle, this, &ACTLobbyGameMode::RecheckStartCondition, 0.3f, false);
}

void ACTLobbyGameMode::RecheckStartCondition()
{
	ACTGameState* CTGameState = GetGameState<ACTGameState>();
	if (!CTGameState || !GetWorldTimerManager().IsTimerActive(StartTimerHandle)) { return; }

	// 시작 카운트다운 중 인원이 부족해졌으면 시작 취소 (방장이 다시 시작할 수 있음)
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

	// 방장은 준비가 필요 없고, 시작이 정해진 뒤에는 바꿀 수 없음
	if (RequesterState->IsHost()) { return; }
	if (CTGameState->GetLobbyStartRemainingSeconds() > 0.f) { return; }

	RequesterState->SetReady(bReady);
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

	const FString MapName = GameInstance->GetGameMapName().ToString();
	UE_LOG(LogTemp, Log, TEXT("[CT] 게임 맵으로 이동: %s"), *MapName);
	World->ServerTravel(MapName);
}
