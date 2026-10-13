// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CTPlayerController.generated.h"

class UInputMappingContext;

/**
 * 플레이어 컨트롤러.
 * 하는 일: Enhanced Input의 "키 설정(Mapping Context)"을 등록합니다.
 * (실제 입력 처리는 ACTCharacter가 합니다.)
 */
UCLASS()
class CATCHTHETAIL_API ACTPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	/** 등록할 키 설정들 (블루프린트에서 IMC_Default 등을 넣어 주세요) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	int32 MappingPriority = 0;

	/** 카메라가 위아래로 돌 수 있는 각도 제한 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	float ViewPitchMin = -60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	float ViewPitchMax = 30.f;
};
