// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyPlayerController.h"
#include "../Core/CTLobbyGameMode.h"
#include "../Core/CTGameState.h"
#include "../UI/CTLobbyWidget.h"
#include "../UI/CTLoadingWidget.h"
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

	// 서버가 시작을 확정했거나(모두), 방금 내가 시작을 눌렀으면 띄움
	const float StartRemaining = CTGameState ? CTGameState->GetLobbyStartRemainingSeconds() : 0.f;
	const bool bConfirmed = StartRemaining > 0.f;
	if (bConfirmed) { LastStartRemaining = StartRemaining; }
	const bool bPending = World && (World->GetTimeSeconds() - StartRequestedTime) < StartPendingSeconds;
	const bool bWantLoading = bConfirmed || bPending;

	if (bWantLoading && !LoadingWidget && LoadingWidgetClass)
	{
		LoadingWidget = CreateWidget<UCTLoadingWidget>(this, LoadingWidgetClass);
		if (LoadingWidget)
		{
			// 로비 UI와 채팅보다 위에 놓아 모든 입력을 막음
			LoadingWidget->AddToViewport(100);
		}
	}
	else if (!bWantLoading && LoadingWidget)
	{
		// 카운트가 끝나서 0이 된 것은 맵 이동 직전이라 그대로 두고, 도중에 취소된 경우에만 치움
		if (LastStartRemaining > 0.f && LastStartRemaining <= 0.6f) { return; }

		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
		LastStartRemaining = 0.f;
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
	// 이미 시작 중이면 다시 보내지 않음 (연타 방지)
	const UWorld* World = GetWorld();
	if (!World || LoadingWidget) { return; }

	// 서버 응답이 오기 전에도 바로 로딩 화면으로 덮어 다른 버튼이 눌리지 않게 함
	StartRequestedTime = World->GetTimeSeconds();
	UpdateLoadingScreen();

	ServerStartGame();
}

void ACTLobbyPlayerController::ServerSetReady_Implementation(bool bReady)
{
	if (ACTLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTLobbyGameMode>() : nullptr)
	{
		GameMode->SetPlayerReady(this, bReady);
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

	GameMode->BroadcastChat(this, Trimmed);
}

void ACTLobbyPlayerController::ClientReceiveChat_Implementation(const FString& SenderName, const FString& Message)
{
	OnChatReceived.Broadcast(SenderName, Message);
}
