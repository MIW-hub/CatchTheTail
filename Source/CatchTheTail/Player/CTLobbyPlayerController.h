// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTPlayerController.h"
#include "../Core/CTMatchSettings.h"
#include "CTLobbyPlayerController.generated.h"

class ACameraActor;
class UCTLobbyWidget;

// 채팅 수신 (보낸 사람 이름, 내용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCTChatReceivedSignature, const FString&, SenderName, const FString&, Message);

// Enter로 채팅 입력칸에 포커스 요청
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTChatFocusRequestedSignature);

// 대기 로비 컨트롤러
UCLASS()
class CATCHTHETAIL_API ACTLobbyPlayerController : public ACTPlayerController
{
	GENERATED_BODY()

public:
	ACTLobbyPlayerController();

	// 준비 요청 (방장 무시)
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby")
	void RequestSetReady(bool bReady);

	// 시작 요청 (방장만)
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby")
	void RequestStartGame();

	// 맵 선택 요청 (방장만)
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby")
	void RequestSelectMap(int32 MapIndex);

	// 경기 설정 변경 요청 (방장만)
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby")
	void RequestChangeMatchSetting(ECTMatchSetting Setting, int32 Direction);

	// 채팅 전송 요청
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby|Chat")
	void SendChat(const FString& Message);

	// 채팅 수신 (서버 호출)
	UFUNCTION(Client, Reliable)
	void ClientReceiveChat(const FString& SenderName, const FString& Message);

	// 채팅 도착 알림
	UPROPERTY(BlueprintAssignable, Category = "CT|Lobby|Chat")
	FCTChatReceivedSignature OnChatReceived;

	// Enter 입력 알림, 입력칸 포커스용
	UPROPERTY(BlueprintAssignable, Category = "CT|Lobby|Chat")
	FCTChatFocusRequestedSignature OnChatFocusRequested;

	int32 GetMaxChatLength() const { return MaxChatLength; }

protected:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual ECTMusic GetLevelMusic() const override { return ECTMusic::Menu; }
	virtual void ClientRestart_Implementation(APawn* NewPawn) override;
	virtual void CreateUI() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby")
	TSubclassOf<UCTLobbyWidget> LobbyWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Lobby")
	TObjectPtr<UCTLobbyWidget> LobbyWidget;

	// 로비 카메라 Pitch (고정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera", meta = (ClampMin = "-89", ClampMax = "-10"))
	float LobbyCameraPitch = -50.f;

	// 화면 위쪽 월드 방향(Yaw), 이동 기준
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera")
	float LobbyCameraYaw = 0.f;

	// 캐릭터~카메라 거리(cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera", meta = (ClampMin = "100"))
	float LobbyCameraDistance = 750.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera", meta = (ClampMin = "10", ClampMax = "170"))
	float LobbyCameraFOV = 60.f;

	// 카메라 추적 속도, 0이면 즉시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera", meta = (ClampMin = "0"))
	float LobbyCameraFollowSpeed = 8.f;

	// 채팅 최대 글자 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Chat", meta = (ClampMin = "1"))
	int32 MaxChatLength = 100;

	// 채팅 최소 간격(초), 서버 검사
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Chat", meta = (ClampMin = "0"))
	float ChatCooldownSeconds = 0.3f;

private:
	UFUNCTION(Server, Reliable)
	void ServerSetReady(bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerStartGame();

	UFUNCTION(Server, Reliable)
	void ServerSelectMap(int32 MapIndex);

	UFUNCTION(Server, Reliable)
	void ServerChangeMatchSetting(ECTMatchSetting Setting, int32 Direction);

	UFUNCTION(Server, Reliable)
	void ServerSendChat(const FString& Message);

	void HandleChatKey();

	// 로비 카메라 생성
	void ApplyLobbyCamera();
	ACameraActor* GetOrCreateLobbyCamera();

	// 카메라 위치 갱신 (각도 고정)
	void UpdateLobbyCamera(float DeltaTime, bool bSnap);

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> LobbyCamera;

	bool bLobbyCameraSnapped = false;

	double LastChatServerTime = -1000.0;
};
