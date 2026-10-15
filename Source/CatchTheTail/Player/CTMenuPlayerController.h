// Copyright Moon Party Games. All Rights Reserved.
// [보류 중] 켜는 방법은 CTGameInstance.h 맨 위 설명을 보세요.
#if 0
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CTMenuPlayerController.generated.h"

class UUserWidget;

/**
 * 메인 메뉴 맵의 플레이어 컨트롤러.
 * 메뉴 위젯(WBP_MainMenu)을 화면에 띄우고 마우스 커서를 보여 줍니다.
 */
UCLASS()
class CATCHTHETAIL_API ACTMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 메뉴 위젯 (블루프린트에서 WBP_MainMenu를 지정. UCTMainMenuWidget을 부모로 만드세요) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Menu")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CT|Menu")
	TObjectPtr<UUserWidget> MainMenuWidget;
};
#endif // 보류: 메인 메뉴 / 방 만들기
