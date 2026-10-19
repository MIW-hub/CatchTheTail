// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "CTGameInstance.generated.h"

class UNetDriver;

/** 메뉴 화면에 보여줄 안내 문구가 바뀔 때 (접속 실패, 연결 끊김 등) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCTMenuMessageSignature, FText, Message);

/**
 * 게임 전체에서 유지되는 객체. 방을 만들고 참가하고 메뉴로 돌아가는 일을 맡습니다.
 *
 * 방식: Listen Server
 *  - 방 만들기: 방장이 게임 맵을 "?listen" 옵션으로 열면 방장이 서버 겸 플레이어가 됩니다.
 *  - 참가: 방장의 IP 주소로 접속합니다. (같은 네트워크 / 포트 포워딩 한 환경)
 *  - Steam 같은 온라인 서비스로 방을 검색하는 기능은 아직 없습니다. (Online Subsystem 필요)
 *
 * 켜는 방법: Config/DefaultEngine.ini 의 [/Script/EngineSettings.GameMapsSettings] 에
 *   GameInstanceClass=/Script/CatchTheTail.CTGameInstance
 * 를 넣거나 프로젝트 세팅의 "게임 인스턴스 클래스"를 지정하세요.
 */
UCLASS()
class CATCHTHETAIL_API UCTGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** 방 만들기: 게임 맵을 listen 서버로 엽니다. */
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void HostGame();

	/**
	 * 방 참가: 방장의 주소로 접속합니다. 예) "192.168.0.10" 또는 "192.168.0.10:7777"
	 * 포트를 생략하면 기본 포트(DefaultPort)를 씁니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void JoinGame(const FString& Address);

	/** 메인 메뉴로 돌아갑니다. Message가 있으면 메뉴에 안내 문구로 보여줍니다. */
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void LeaveToMainMenu(FText Message);

	/** 게임 종료 */
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void QuitToDesktop();

	/** 인트로를 이미 보여줬는가? (게임을 켠 뒤 한 번만 보여 주려고, 메뉴로 돌아와도 유지) */
	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	bool HasShownIntro() const { return bIntroShown; }

	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void MarkIntroShown() { bIntroShown = true; }

	/** 메뉴가 열릴 때 보여줄 마지막 안내 문구 (없으면 빈 문구) */
	UFUNCTION(BlueprintPure, Category = "CT|Menu")
	FText GetPendingMessage() const { return PendingMessage; }

	/** 안내 문구를 비웁니다. (메뉴가 한 번 보여준 뒤) */
	UFUNCTION(BlueprintCallable, Category = "CT|Menu")
	void ClearPendingMessage() { PendingMessage = FText::GetEmpty(); }

	UPROPERTY(BlueprintAssignable, Category = "CT|Menu")
	FCTMenuMessageSignature OnMenuMessage;

protected:
	/** 메인 메뉴 맵 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	FName MainMenuMapName = TEXT("L_MainMenu");

	/** 게임 맵 이름 (방 만들기로 여는 맵) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	FName GameMapName = TEXT("TypeA");

	/** 참가할 때 주소에 포트가 없으면 붙일 포트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 DefaultPort = 7777;

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	/** 지금 메인 메뉴 맵에 있는가? */
	bool IsInMainMenuMap() const;

	FText PendingMessage;
	bool bIntroShown = false;
};
