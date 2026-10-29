// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Type/CTTypes.h"
#include "../Core/CTAudioSettings.h"
#include "CTPlayerController.generated.h"

class UInputMappingContext;
class UCTHUDWidget;
class UCTResultWidget;
class ACTGameState;
class UCameraShakeBase;

UCLASS()
class CATCHTHETAIL_API ACTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	UCTHUDWidget* GetHUDWidget() const { return HUDWidget; }

	// 결과 화면 열기
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void ShowResultScreen();

	// 로비 복귀 요청 (방장만)
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void RequestReturnToLobby();

	// 결과 화면 닫기
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void HideResultScreen();

	// 카메라 흔들림 (설정 반영)
	UFUNCTION(BlueprintCallable, Category = "CT|Camera")
	void PlayCTCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.f);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 레벨 배경음, 로비가 교체
	virtual ECTMusic GetLevelMusic() const { return ECTMusic::Game; }

	// 카운트다운 소리 감지
	virtual void PlayerTick(float DeltaTime) override;

	// UI 생성, 로비가 교체
	virtual void CreateUI();

	// UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTResultWidget> ResultWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|UI")
	TObjectPtr<UCTHUDWidget> HUDWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|UI")
	TObjectPtr<UCTResultWidget> ResultWidget;

	// 등록할 IMC 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Input")
	int32 MappingPriority = 0;

	// 카메라 상하 제한, 바닥 아래 방지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	float ViewPitchMin = -80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Camera")
	float ViewPitchMax = 5.f;

private:
	UFUNCTION()
	void HandleRoundChanged(int32 NewRoundNumber);

	// 마지막으로 소리를 낸 카운트다운 숫자
	int32 LastCountdownSoundNumber = 0;

	UFUNCTION(Server, Reliable)
	void ServerReturnToLobby();

	// 저장된 이름을 서버에 알림 (엔진이 OS 닉네임으로 덮어씀)
	UFUNCTION(Server, Reliable)
	void ServerSetPlayerName(const FString& NewName);

	// GameState 지연 도착 재시도
	void TryBindGameState();

	UFUNCTION()
	void HandlePhaseChanged(ECTGamePhase NewPhase);

	TWeakObjectPtr<ACTGameState> BoundGameState;
	FTimerHandle BindGameStateTimerHandle;
};
