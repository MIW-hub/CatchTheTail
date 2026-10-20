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
	// ===== 플레이어 이름 (메인 메뉴 전에 입력해서 이 컴퓨터에 저장) =====
	/** 저장된 이름. 아직 없으면 빈 문자열 */
	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	FString GetPlayerName() const { return PlayerName; }

	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	bool HasPlayerName() const { return !PlayerName.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	int32 GetMinNameLength() const { return MinNameLength; }

	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	int32 GetMaxNameLength() const { return MaxNameLength; }

	/** 이름이 규칙에 맞는지 검사합니다. 맞지 않으면 OutError에 이유가 들어갑니다. */
	UFUNCTION(BlueprintPure, Category = "CT|Profile")
	bool ValidatePlayerName(const FString& Name, FText& OutError) const;

	/**
	 * 이름을 검사하고 저장합니다. 성공하면 true, 실패하면 false와 OutError.
	 * 이름은 처음 한 번만 정할 수 있습니다. 이미 저장된 이름이 있으면 실패합니다.
	 */
	/** [개발용] 저장된 이름을 지웁니다. 콘솔에서 CTResetPlayerName (배포 빌드에서는 동작하지 않음) */
	UFUNCTION(Exec)
	void CTResetPlayerName();

	UFUNCTION(BlueprintCallable, Category = "CT|Profile")
	bool SetPlayerName(const FString& NewName, FText& OutError);

	/**
	 * 이름을 바꿉니다. 처음 한 번만 정하는 규칙(SetPlayerName)을 거치지 않으므로,
	 * 설정(Allow Name Change)에서 허용했을 때만 부르세요. (UCTSettingsSubsystem::ChangePlayerName이 확인함)
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|Profile")
	bool ChangePlayerName(const FString& NewName, FText& OutError);

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

	/** 이름 최소 글자 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Profile", meta = (ClampMin = "1"))
	int32 MinNameLength = 2;

	/** 이름 최대 글자 수 (점수판에 들어가는 길이) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Profile", meta = (ClampMin = "1"))
	int32 MaxNameLength = 12;

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	/** 지금 메인 메뉴 맵에 있는가? */
	bool IsInMainMenuMap() const;

	void LoadProfile();
	void SaveProfile() const;

	/** 이름을 검사해서 저장 */
	bool StoreValidatedName(const FString& NewName, FText& OutError);

	/** 서버에 이름을 전달하는 접속 옵션. 예) "?Name=Chick" (이름이 없으면 빈 문자열) */
	FString MakeNameOption() const;

	/** 저장된 이름 */
	FString PlayerName;

	FText PendingMessage;
	bool bIntroShown = false;
};
