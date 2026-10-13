// Copyright Moon Party Games. All Rights Reserved.
#include "CTPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Camera/PlayerCameraManager.h"

void ACTPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 내 화면을 조작하는 컨트롤러에서만 입력을 등록합니다.
	// (서버에 있는 다른 사람의 컨트롤러는 건너뜁니다)
	if (!IsLocalController())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			for (UInputMappingContext* Context : DefaultMappingContexts)
			{
				if (Context)
				{
					Subsystem->AddMappingContext(Context, MappingPriority);
				}
			}
		}
	}

	// 마우스 커서 숨기고 게임 입력만 받기
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = ViewPitchMin;
		PlayerCameraManager->ViewPitchMax = ViewPitchMax;
	}
}
