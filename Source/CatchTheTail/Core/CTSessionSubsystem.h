// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Online/CoreOnline.h"
#include "CTSessionSubsystem.generated.h"

class IEOSPlatformHandle;
namespace UE::Online { class IOnlineServices; }

// 온라인 방 관리 (EOS: 에픽 계정 없이 기기 ID로 로그인하고, 방(로비)에 방 코드를 달아 두면 코드로 찾아 들어옴. 접속은 EOS P2P)
UCLASS()
class CATCHTHETAIL_API UCTSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 온라인 사용 가능 여부 (EOS 로그인이 끝났거나 진행 중)
	bool IsAvailable() const;

	// 온라인 서비스 이름. 없으면 빈 문자열
	FString GetServiceName() const;

	// 방장: 방 코드 방 열기. 이미 열려 있으면 무시
	void CreateRoom(const FString& RoomCode);

	// 열린 방(또는 들어간 방) 나가기
	void CloseRoom();

	// 방 코드로 찾아 입장
	void JoinByRoomCode(const FString& RoomCode);

	// 무작위 방 입장
	void QuickJoin();

	// 찾는 중이거나 입장 중인가
	bool IsBusy() const { return bBusy; }

private:
	enum class EJoinMode : uint8 { None, Code, Quick };

	// EOS 로그인: 기기 ID 만들기(이미 있으면 그대로) -> EOS 연결 로그인 -> 온라인 서비스 로그인
	void StartLogin();
	void ContinueLogin();
	void FinishLogin(bool bSuccess);
	void RunAfterLogin(TFunction<void()> Action);

	void StartSearch(EJoinMode Mode, const FString& RoomCode);
	void ShowMessage(const FText& Message) const;
	void EndBusy();

	TSharedPtr<UE::Online::IOnlineServices> Services;
	TSharedPtr<IEOSPlatformHandle, ESPMode::ThreadSafe> Platform;

	UE::Online::FAccountId LocalAccount;
	UE::Online::FLobbyId CurrentLobby;
	bool bHasLobby = false;

	bool bLoggedIn = false;
	bool bLoggingIn = false;
	bool bLoginFailed = false;
	TArray<TFunction<void()>> AfterLogin;

	bool bBusy = false;
	bool bCreating = false;
};
