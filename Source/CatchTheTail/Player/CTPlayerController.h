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

	// 게임이 끝난 뒤 대기 로비로 돌아가기 요청 (방장만 가능)
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void RequestReturnToLobby();

	// 결과 화면 닫기
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void HideResultScreen();

	// 카메라 흔들림 (설정 반영), PlayCameraShake 대체
	UFUNCTION(BlueprintCallable, Category = "CT|Camera")
	void PlayCTCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.f);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 이 레벨에서 재생할 배경음. 로비 컨트롤러가 교체
	virtual ECTMusic GetLevelMusic() const { return ECTMusic::Game; }

	// 카운트다운 숫자가 바뀔 때 소리
	virtual void PlayerTick(float DeltaTime) override;

	// 화면에 띄울 UI 생성. 로비 컨트롤러가 교체
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

	// 카메라 상하 각도 제한. 위로 올려다볼수록 카메라가 캐릭터 뒤 아래로 내려가므로
	// 뒤 시점 거리(450cm) 기준 약 +19도부터 카메라가 바닥 아래로 들어갑니다. 여유를 두고 +5도까지만 허용
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

	// 엔진은 접속 시 OS 닉네임(컴퓨터 이름)을 Name 옵션으로 덮어쓰므로, 저장된 이름을 서버에 직접 알림
	UFUNCTION(Server, Reliable)
	void ServerSetPlayerName(const FString& NewName);

	// GameState 늦게 오므로 재시도
	void TryBindGameState();

	UFUNCTION()
	void HandlePhaseChanged(ECTGamePhase NewPhase);

	TWeakObjectPtr<ACTGameState> BoundGameState;
	FTimerHandle BindGameStateTimerHandle;
};
