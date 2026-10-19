// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CTMenuPlayerController.generated.h"

class UUserWidget;
class UCTIntroWidget;

/**
 * 메인 메뉴 맵의 플레이어 컨트롤러.
 * 게임을 켠 뒤 처음에는 인트로(WBP_Intro)를 보여 주고, 끝나면 메뉴 위젯(WBP_MainMenu)으로 넘어갑니다.
 * 게임에서 메뉴로 돌아온 경우에는 인트로를 건너뜁니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 인트로 위젯 (UCTIntroWidget을 부모로 만든 WBP_Intro). 비워 두면 인트로 없이 바로 메뉴 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	TSubclassOf<UCTIntroWidget> IntroWidgetClass;

	/** 메뉴 위젯 (블루프린트에서 WBP_MainMenu를 지정. UCTMainMenuWidget을 부모로 만드세요) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Menu")
	TObjectPtr<UCTIntroWidget> IntroWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Menu")
	TObjectPtr<UUserWidget> MainMenuWidget;

private:
	void ShowIntro();
	void ShowMainMenu();

	UFUNCTION()
	void HandleIntroFinished();

	/** 위젯을 화면에 올리고 UI 입력 모드로 바꿉니다. */
	void ShowWidgetWithUIInput(UUserWidget* Widget);
};
