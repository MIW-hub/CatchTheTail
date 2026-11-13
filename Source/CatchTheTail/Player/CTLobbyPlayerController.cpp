// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyPlayerController.h"
#include "../Core/CTLobbyGameMode.h"
#include "../Core/CTChatFilter.h"
#include "../Core/CTGameState.h"
#include "CTPlayerState.h"
#include "../UI/CTLobbyWidget.h"
#include "../UI/CTLoadingWidget.h"
#include "../UI/CTMicIndicatorWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

ACTLobbyPlayerController::ACTLobbyPlayerController()
{
	// 폰 받아도 로비 카메라 유지
	bAutoManageActiveCameraTarget = false;

	// 기본 로딩 화면 (WBP_CTLoadingWidget). 컨트롤러 블루프린트에서 바꿀 수 있음
	static ConstructorHelpers::FClassFinder<UCTLoadingWidget> LoadingWidgetFinder(TEXT("/Game/UI/WBP_CTLoadingWidget"));
	if (LoadingWidgetFinder.Succeeded())
	{
		LoadingWidgetClass = LoadingWidgetFinder.Class;
	}
}

void ACTLobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enter: 채팅 포커스
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACTLobbyPlayerController::HandleChatKey);
	}
}

void ACTLobbyPlayerController::HandleChatKey()
{
	OnChatFocusRequested.Broadcast();
}

void ACTLobbyPlayerController::CreateUI()
{
	// 게임 HUD 대신 로비 UI
	if (LobbyWidgetClass && !LobbyWidget)
	{
		LobbyWidget = CreateWidget<UCTLobbyWidget>(this, LobbyWidgetClass);
		if (LobbyWidget)
		{
			LobbyWidget->AddToViewport(0);
		}
	}

	// 마이크 상태 (오른쪽 아래, 준비 버튼 왼쪽). 눌러서 말하기 키와 말하는 모습이 보임
	if (!MicWidget)
	{
		MicWidget = CreateWidget<UCTMicIndicatorWidget>(this, UCTMicIndicatorWidget::StaticClass());
		if (MicWidget)
		{
			// 화면에 먼저 추가한 뒤에 위치와 크기를 정해야 적용됨
			MicWidget->AddToViewport(5);
			if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld()))
			{
				// 오른쪽 아래 기준, 오른쪽 아래 모서리를 맞춤. 오프셋은 (X, Y, 폭, 높이)
				FGameViewportWidgetSlot ViewportSlot;
				ViewportSlot.Anchors = FAnchors(1.f, 1.f);
				ViewportSlot.Offsets = FMargin(-450.f, -30.f, 130.f, 100.f);
				ViewportSlot.Alignment = FVector2D(1.f, 1.f);
				ViewportSlot.ZOrder = 5;
				ViewportSubsystem->SetWidgetSlot(MicWidget, ViewportSlot);
			}
		}
	}

	// 마우스 UI, 시점 고정
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetIgnoreLookInput(true);

	ApplyLobbyCamera();
}

// 카메라

ACameraActor* ACTLobbyPlayerController::GetOrCreateLobbyCamera()
{
	if (LobbyCamera) { return LobbyCamera; }

	UWorld* World = GetWorld();
	if (!World) { return nullptr; }

	// 로컬 전용 카메라
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	LobbyCamera = World->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator(LobbyCameraPitch, LobbyCameraYaw, 0.f), Params);
	if (LobbyCamera && LobbyCamera->GetCameraComponent())
	{
		LobbyCamera->GetCameraComponent()->SetFieldOfView(LobbyCameraFOV);
		LobbyCamera->GetCameraComponent()->bConstrainAspectRatio = false;
	}
	return LobbyCamera;
}

void ACTLobbyPlayerController::ApplyLobbyCamera()
{
	if (!IsLocalController()) { return; }

	if (ACameraActor* Camera = GetOrCreateLobbyCamera())
	{
		SetViewTarget(Camera);

		// 화면 위쪽이 앞이 되게 회전 맞춤
		SetControlRotation(FRotator(0.f, LobbyCameraYaw, 0.f));
		UpdateLobbyCamera(0.f, /*bSnap=*/true);
	}
}

void ACTLobbyPlayerController::UpdateLobbyCamera(float DeltaTime, bool bSnap)
{
	ACameraActor* Camera = LobbyCamera;
	const APawn* ControlledPawn = GetPawn();
	if (!Camera || !ControlledPawn) { return; }

	// 카메라 반대 방향으로 거리만큼 후퇴
	const FRotator CameraRotation(LobbyCameraPitch, LobbyCameraYaw, 0.f);
	const FVector Desired = ControlledPawn->GetActorLocation() - CameraRotation.Vector() * LobbyCameraDistance;

	const bool bInstant = bSnap || !bLobbyCameraSnapped || LobbyCameraFollowSpeed <= 0.f;
	const FVector NewLocation = bInstant ? Desired : FMath::VInterpTo(Camera->GetActorLocation(), Desired, DeltaTime, LobbyCameraFollowSpeed);

	Camera->SetActorLocationAndRotation(NewLocation, CameraRotation);
	bLobbyCameraSnapped = true;
}

void ACTLobbyPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalController())
	{
		UpdateLobbyCamera(DeltaTime, /*bSnap=*/false);
		UpdateLoadingScreen();
	}
}

void ACTLobbyPlayerController::UpdateLoadingScreen()
{
	const UWorld* World = GetWorld();
	const ACTGameState* CTGameState = World ? World->GetGameState<ACTGameState>() : nullptr;

	// 시작 카운트다운(3, 2, 1)은 로비 화면에서 보이게 두고, 카운트가 끝나 맵으로 넘어가기 직전에만 로딩 화면을 띄움
	const float StartRemaining = CTGameState ? CTGameState->GetLobbyStartRemainingSeconds() : 0.f;
	const bool bCounting = StartRemaining > 0.f;
	if (bCounting) { LastStartRemaining = StartRemaining; }

	// 카운트가 끝에 가까워 0이 된 것은 맵 이동 직전. 도중에 취소된 것은 아님
	const bool bFinished = !bCounting && LastStartRemaining > 0.f && LastStartRemaining <= 0.6f;
	if (!bCounting && !bFinished) { LastStartRemaining = 0.f; }

	if (bFinished && !LoadingWidget && LoadingWidgetClass)
	{
		LoadingWidget = CreateWidget<UCTLoadingWidget>(this, LoadingWidgetClass);
		if (LoadingWidget)
		{
			// 로비 UI와 채팅보다 위에 놓아 모든 입력을 막음
			LoadingWidget->AddToViewport(100);
		}
	}
	else if (!bFinished && !bCounting && LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}
}

void ACTLobbyPlayerController::ClientRestart_Implementation(APawn* NewPawn)
{
	Super::ClientRestart_Implementation(NewPawn);

	// 폰 받으면 회전이 바뀌어 재적용
	ApplyLobbyCamera();
}

// 준비 / 시작

void ACTLobbyPlayerController::RequestSetReady(bool bReady)
{
	ServerSetReady(bReady);
}

void ACTLobbyPlayerController::RequestStartGame()
{
	// 이미 시작 중이면 다시 보내지 않음 (연타 방지). 카운트다운은 로비 화면에서 보이고, 끝난 뒤에 로딩 화면이 뜸
	const UWorld* World = GetWorld();
	if (!World || LoadingWidget) { return; }
	if (World->GetTimeSeconds() - StartRequestedTime < StartPendingSeconds) { return; }
	const ACTGameState* CTGameState = World->GetGameState<ACTGameState>();
	if (CTGameState && CTGameState->GetLobbyStartRemainingSeconds() > 0.f) { return; }

	StartRequestedTime = World->GetTimeSeconds();
	ServerStartGame();
}

void ACTLobbyPlayerController::ServerSetReady_Implementation(bool bReady)
{
	if (ACTLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTLobbyGameMode>() : nullptr)
	{
		GameMode->SetPlayerReady(this, bReady);
	}
}

void ACTLobbyPlayerController::CTLobbyPreview()
{
	const ACTGameState* CTGameState = GetWorld() ? GetWorld()->GetGameState<ACTGameState>() : nullptr;
	if (!CTGameState) { return; }

	if (CTGameState->GetMatchSettings().GetMode() != ECTGameModeType::TreasureHunt)
	{
		RequestChangeMatchSetting(ECTMatchSetting::Mode, 1);
	}
	for (int32 Index = 0; Index < 3; ++Index) { RequestChangeMatchSetting(ECTMatchSetting::AiPlayers, 1); }
}

void ACTLobbyPlayerController::RequestChangeTeam()
{
	ServerChangeTeam();
}

void ACTLobbyPlayerController::ServerChangeTeam_Implementation()
{
	if (ACTLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTLobbyGameMode>() : nullptr)
	{
		GameMode->RequestTeamChange(this);
	}
}

void ACTLobbyPlayerController::RequestAcceptTeamSwap(APlayerState* RequesterState)
{
	ServerAcceptTeamSwap(RequesterState);
}

void ACTLobbyPlayerController::ServerAcceptTeamSwap_Implementation(APlayerState* RequesterState)
{
	if (ACTLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTLobbyGameMode>() : nullptr)
	{
		GameMode->AcceptTeamSwap(this, Cast<ACTPlayerState>(RequesterState));
	}
}

void ACTLobbyPlayerController::RequestSelectMap(int32 MapIndex)
{
	ServerSelectMap(MapIndex);
}

void ACTLobbyPlayerController::ServerSelectMap_Implementation(int32 MapIndex)
{
	if (ACTLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTLobbyGameMode>() : nullptr)
	{
		GameMode->SelectMap(this, MapIndex);
	}
}

void ACTLobbyPlayerController::RequestChangeMatchSetting(ECTMatchSetting Setting, int32 Direction)
{
	ServerChangeMatchSetting(Setting, Direction);
}

void ACTLobbyPlayerController::ServerChangeMatchSetting_Implementation(ECTMatchSetting Setting, int32 Direction)
{
	if (ACTLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTLobbyGameMode>() : nullptr)
	{
		GameMode->ChangeMatchSetting(this, Setting, Direction);
	}
}

void ACTLobbyPlayerController::ServerStartGame_Implementation()
{
	if (ACTLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTLobbyGameMode>() : nullptr)
	{
		GameMode->TryStartGame(this);
	}
}

// 채팅

void ACTLobbyPlayerController::SendChat(const FString& Message)
{
	const FString Trimmed = Message.TrimStartAndEnd();
	if (Trimmed.IsEmpty()) { return; }

	ServerSendChat(Trimmed.Left(MaxChatLength));
}

void ACTLobbyPlayerController::ServerSendChat_Implementation(const FString& Message)
{
	UWorld* World = GetWorld();
	ACTLobbyGameMode* GameMode = World ? World->GetAuthGameMode<ACTLobbyGameMode>() : nullptr;
	if (!GameMode) { return; }

	// 서버 재검사: 빈 문장, 길이, 도배
	const FString Trimmed = Message.TrimStartAndEnd().Left(MaxChatLength);
	if (Trimmed.IsEmpty()) { return; }

	const double Now = World->GetTimeSeconds();
	if (Now - LastChatServerTime < ChatCooldownSeconds) { return; }
	LastChatServerTime = Now;

	// 비속어, 은어는 가려서 보냄
	GameMode->BroadcastChat(this, FCTChatFilter::Sanitize(Trimmed));
}

void ACTLobbyPlayerController::ClientReceiveChat_Implementation(const FString& SenderName, const FString& Message)
{
	OnChatReceived.Broadcast(SenderName, Message);
}
