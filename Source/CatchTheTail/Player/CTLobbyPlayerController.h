// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTPlayerController.h"
#include "CTLobbyPlayerController.generated.h"

class ACameraActor;
class UCTLobbyWidget;

// 채팅 수신 (보낸 사람 이름, 내용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCTChatReceivedSignature, const FString&, SenderName, const FString&, Message);

// Enter로 채팅 입력칸에 포커스 요청
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTChatFocusRequestedSignature);

/**
 * 대기 로비용 컨트롤러.
 * - 게임 HUD 대신 로비 UI를 띄우고, 준비/시작 요청을 서버로 보냅니다.
 * - 카메라는 플레이어를 따라가지만 각도는 고정(쿼터뷰)이고, 마우스 시점 입력을 막습니다.
 * - 채팅 메시지를 서버를 거쳐 로비 전원에게 전달합니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTLobbyPlayerController : public ACTPlayerController
{
	GENERATED_BODY()

public:
	ACTLobbyPlayerController();

	// 준비 상태 변경 요청 (방장은 무시됨)
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby")
	void RequestSetReady(bool bReady);

	// 게임 시작 요청 (방장만 가능)
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby")
	void RequestStartGame();

	// 채팅 보내기. 빈 문장은 무시, 너무 길면 잘림
	UFUNCTION(BlueprintCallable, Category = "CT|Lobby|Chat")
	void SendChat(const FString& Message);

	// 서버가 호출. 받은 채팅을 이 플레이어 화면에 전달
	UFUNCTION(Client, Reliable)
	void ClientReceiveChat(const FString& SenderName, const FString& Message);

	// 채팅이 도착할 때마다 호출 (UI용)
	UPROPERTY(BlueprintAssignable, Category = "CT|Lobby|Chat")
	FCTChatReceivedSignature OnChatReceived;

	// Enter를 눌렀을 때 호출. 채팅 입력칸에 포커스를 주세요
	UPROPERTY(BlueprintAssignable, Category = "CT|Lobby|Chat")
	FCTChatFocusRequestedSignature OnChatFocusRequested;

	// 채팅 한 줄 최대 글자 수
	int32 GetMaxChatLength() const { return MaxChatLength; }

protected:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void ClientRestart_Implementation(APawn* NewPawn) override;
	virtual void CreateUI() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby")
	TSubclassOf<UCTLobbyWidget> LobbyWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Lobby")
	TObjectPtr<UCTLobbyWidget> LobbyWidget;

	/**
	 * 로비 카메라: 플레이어를 따라가지만 각도는 고정입니다. (마우스로 돌릴 수 없음)
	 * Pitch가 -90에 가까울수록 위에서 내려다보고, 0에 가까울수록 눈높이에 가깝습니다. 얼굴이 보이는 쿼터뷰는 -45 ~ -55 정도입니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera", meta = (ClampMin = "-89", ClampMax = "-10"))
	float LobbyCameraPitch = -50.f;

	// 화면 위쪽이 가리키는 월드 방향 (Yaw). 이동 방향도 이 기준을 따릅니다. 0이면 +X가 화면 위쪽
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera")
	float LobbyCameraYaw = 0.f;

	// 캐릭터와 카메라 사이 거리(cm). 가까울수록 캐릭터가 크게 보입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera", meta = (ClampMin = "100"))
	float LobbyCameraDistance = 750.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera", meta = (ClampMin = "10", ClampMax = "170"))
	float LobbyCameraFOV = 60.f;

	// 카메라가 캐릭터를 따라가는 속도. 0이면 즉시 따라감
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Camera", meta = (ClampMin = "0"))
	float LobbyCameraFollowSpeed = 8.f;

	// 채팅 한 줄 최대 글자 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Chat", meta = (ClampMin = "1"))
	int32 MaxChatLength = 100;

	// 연속 채팅 최소 간격(초). 도배 방지 (서버에서 검사)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Lobby|Chat", meta = (ClampMin = "0"))
	float ChatCooldownSeconds = 0.3f;

private:
	UFUNCTION(Server, Reliable)
	void ServerSetReady(bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerStartGame();

	UFUNCTION(Server, Reliable)
	void ServerSendChat(const FString& Message);

	void HandleChatKey();

	// 로비 카메라를 만들어 시점으로 지정
	void ApplyLobbyCamera();
	ACameraActor* GetOrCreateLobbyCamera();

	// 카메라를 캐릭터 뒤쪽 위로 이동 (각도 고정)
	void UpdateLobbyCamera(float DeltaTime, bool bSnap);

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> LobbyCamera;

	bool bLobbyCameraSnapped = false;

	double LastChatServerTime = -1000.0;
};
