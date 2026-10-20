// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Type/CTTypes.h"
#include "CTPlayerController.generated.h"

class UInputMappingContext;
class UCTHUDWidget;
class UCTResultWidget;
class ACTGameState;
class UCameraShakeBase;

/**
 * 플레이어 컨트롤러.
 * 하는 일
 *  - Enhanced Input의 "키 설정(Mapping Context)"을 등록합니다. (실제 입력 처리는 ACTCharacter)
 *  - 내 화면의 UI를 만듭니다: 게임 중 HUD, 경기가 끝나면 결과 화면
 */
UCLASS()
class CATCHTHETAIL_API ACTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	UCTHUDWidget* GetHUDWidget() const { return HUDWidget; }

	/** 결과 화면 열기 (마우스 커서 표시) */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void ShowResultScreen();

	/** 결과 화면 닫기 (게임 입력으로 복귀) */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void HideResultScreen();

	/**
	 * 카메라 흔들림을 재생합니다. 설정에서 "화면 흔들림"을 껐으면 재생하지 않습니다.
	 * 흔들림 연출은 PlayCameraShake 대신 이 함수를 쓰세요.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|Camera")
	void PlayCTCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.f);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ===== UI (블루프린트에서 WBP를 넣어 주세요. 비워 두면 만들지 않음) =====
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTResultWidget> ResultWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|UI")
	TObjectPtr<UCTHUDWidget> HUDWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|UI")
	TObjectPtr<UCTResultWidget> ResultWidget;

	/** 등록할 키 설정들 (블루프린트에서 IMC_Default 등을 넣어 주세요) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	int32 MappingPriority = 0;

	/** 카메라가 위아래로 돌 수 있는 각도 제한 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	float ViewPitchMin = -80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	float ViewPitchMax = 20.f;

private:
	void CreateUI();

	/** 클라이언트에서는 GameState가 늦게 도착하므로 찾을 때까지 다시 시도합니다. */
	void TryBindGameState();

	UFUNCTION()
	void HandlePhaseChanged(ECTGamePhase NewPhase);

	TWeakObjectPtr<ACTGameState> BoundGameState;
	FTimerHandle BindGameStateTimerHandle;
};
