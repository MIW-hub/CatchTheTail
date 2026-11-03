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
class UCTPauseMenuWidget;
class UCTPhonePopupWidget;
class ACTGameState;
class UCameraShakeBase;

UCLASS()
class CATCHTHETAIL_API ACTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACTPlayerController();

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

	// ESC 메뉴 (설정, 기권, 게임 종료). 게임은 멈추지 않고 이동과 시점 입력만 막음
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void OpenPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void ClosePauseMenu();

	UFUNCTION(BlueprintPure, Category = "CT|UI")
	bool IsPauseMenuOpen() const;

	// 기권 요청. 서버가 시간과 상태를 다시 검사함
	UFUNCTION(BlueprintCallable, Category = "CT|Game")
	void RequestForfeit();

	// 전화부스 팝업. 부스 안에서 F키로 열고, 대상을 고르면 닫힘
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void OpenPhonePopup();

	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void ClosePhonePopup();

	UFUNCTION(BlueprintPure, Category = "CT|UI")
	bool IsPhonePopupOpen() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
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

	// ESC 메뉴 (기본값 WBP_CTPauseMenuWidget)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTPauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|UI")
	TObjectPtr<UCTPauseMenuWidget> PauseMenuWidget;

	// 전화부스 팝업 (기본은 코드로 만든 UCTPhonePopupWidget)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	TSubclassOf<UCTPhonePopupWidget> PhonePopupWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|UI")
	TObjectPtr<UCTPhonePopupWidget> PhonePopupWidget;

	// 이 컨트롤러에서 ESC 메뉴를 열 수 있는가 (대기 로비는 닫음)
	virtual bool CanOpenPauseMenu() const;

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

	UFUNCTION(Server, Reliable)
	void ServerForfeit();

	// ESC 키: 열려 있으면 한 단계 뒤로(설정, 확인 창), 더 갈 곳이 없으면 닫기
	void HandlePauseKey();

	// F키: 전화부스 안이면 팝업 열기/닫기
	void HandlePhoneKey();

	// 팝업에서 대상을 골랐을 때: 팝업을 닫고 서버에 요청
	UFUNCTION()
	void HandlePhoneQuerySelected(ECTPhoneQuery Query);

	// 팝업을 계속 열어 둘 수 있는 상태인가 (부스 안, 경기 중, 유령 아님)
	bool CanKeepPhonePopup() const;

	UFUNCTION(Server, Reliable)
	void ServerRequestPhoneCall(ECTPhoneQuery Query);

	// 유령이 F키로 무작위 시작점으로 이동 요청 (유령 상태 유지). 서버가 상태를 다시 검사함
	UFUNCTION(Server, Reliable)
	void ServerRequestGhostRelocate();

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
