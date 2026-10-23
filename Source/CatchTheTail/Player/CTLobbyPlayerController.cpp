// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyPlayerController.h"
#include "../Core/CTLobbyGameMode.h"
#include "../UI/CTLobbyWidget.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

ACTLobbyPlayerController::ACTLobbyPlayerController()
{
	// 폰을 받아도 시점이 캐릭터 카메라로 넘어가지 않게 함 (로비 카메라 유지)
	bAutoManageActiveCameraTarget = false;
}

void ACTLobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enter: 채팅 입력칸에 포커스
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

	// 마우스로 버튼을 누르고, 시점은 고정
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

	// 이 컴퓨터에서만 쓰는 카메라
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

		// 이동 방향 기준을 카메라 방향에 맞춤: 화면 위쪽 = 앞
		SetControlRotation(FRotator(0.f, LobbyCameraYaw, 0.f));
		UpdateLobbyCamera(0.f, /*bSnap=*/true);
	}
}

void ACTLobbyPlayerController::UpdateLobbyCamera(float DeltaTime, bool bSnap)
{
	ACameraActor* Camera = LobbyCamera;
	const APawn* ControlledPawn = GetPawn();
	if (!Camera || !ControlledPawn) { return; }

	// 캐릭터에서 카메라 방향 반대쪽으로 거리만큼 물러난 위치
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
	}
}

void ACTLobbyPlayerController::ClientRestart_Implementation(APawn* NewPawn)
{
	Super::ClientRestart_Implementation(NewPawn);

	// 폰을 받으면 컨트롤 회전이 폰 방향으로 바뀌므로 다시 카메라 방향에 맞춤
	ApplyLobbyCamera();
}

// 준비 / 시작

void ACTLobbyPlayerController::RequestSetReady(bool bReady)
{
	ServerSetReady(bReady);
}

void ACTLobbyPlayerController::RequestStartGame()
{
	ServerStartGame();
}

void ACTLobbyPlayerController::ServerSetReady_Implementation(bool bReady)
{
	if (ACTLobbyGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACTLobbyGameMode>() : nullptr)
	{
		GameMode->SetPlayerReady(this, bReady);
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

	// 서버에서도 검사: 빈 문장, 길이, 도배
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
