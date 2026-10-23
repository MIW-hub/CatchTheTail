// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CTMenuPlayerController.generated.h"

class UUserWidget;
class UCTIntroWidget;
class UCTNameEntryWidget;

UCLASS()
class CATCHTHETAIL_API ACTMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 인트로 위젯, 비면 바로 메뉴
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	TSubclassOf<UCTIntroWidget> IntroWidgetClass;

	// 메뉴 위젯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	// 이름 입력 위젯, 비면 생략
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	TSubclassOf<UCTNameEntryWidget> NameEntryWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Menu")
	TObjectPtr<UCTIntroWidget> IntroWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Menu")
	TObjectPtr<UCTNameEntryWidget> NameEntryWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Menu")
	TObjectPtr<UUserWidget> MainMenuWidget;

private:
	void ShowIntro();
	void ShowMainMenu();

	// 이름 없으면 입력, 있으면 메뉴
	void ShowNameEntryOrMenu();

	// 이름 입력 (최초 1회)
	void ShowNameEntry();

	UFUNCTION()
	void HandleNameConfirmed();

	UFUNCTION()
	void HandleIntroFinished();

	// 위젯 표시 + UI 입력 모드
	void ShowWidgetWithUIInput(UUserWidget* Widget);
};
