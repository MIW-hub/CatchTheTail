// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Type/CTTypes.h"
#include "CTUserWidget.generated.h"

class ACTGameState;
class ACTPlayerState;
class ACTCharacter;
class UTextBlock;
class UImage;
class UTexture2D;

/**
 * 이 프로젝트 모든 위젯의 공통 부모. (WBP를 만들 때 직접 쓰지 말고 자식 클래스를 쓰세요)
 *
 * 하는 일
 *  - GameState / 내 PlayerState / 내 캐릭터를 쉽게 가져오기
 *  - 색 → 이름 + 도형 아이콘 표시 (README: "UI는 색 이름과 도형 아이콘을 함께 표시")
 *  - 시간 표시 형식 (mm:ss)
 *
 * WBP에서 이름이 같은 위젯(예: Text_TailCount)을 만들면 C++이 자동으로 연결합니다. (BindWidgetOptional)
 * 없으면 그냥 건너뛰므로 필요한 것만 만들어도 됩니다.
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	ACTGameState* GetCTGameState() const;

	/** 이 위젯을 가진 플레이어(나)의 PlayerState. 접속 직후에는 nullptr일 수 있습니다. */
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	ACTPlayerState* GetOwningCTPlayerState() const;

	UFUNCTION(BlueprintPure, Category = "CT|UI")
	ACTCharacter* GetOwningCTCharacter() const;

	/** 색에 해당하는 도형 아이콘 (지정 안 했으면 nullptr) */
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	UTexture2D* GetColorIcon(ECTColor Color) const;

	/**
	 * 색 이름 텍스트와 아이콘 이미지를 한 번에 설정합니다. (둘 중 하나는 nullptr 가능)
	 * 색이 None이면 NoneText를 표시하고 아이콘을 숨깁니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void ApplyColorToWidgets(ECTColor Color, UTextBlock* NameText, UImage* IconImage, FText NoneText) const;

	/** 초 → "m:ss" */
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	static FText FormatTime(float Seconds);

	/** 관전자를 뺀 플레이어 목록. 꼬리 수 많은 순 → 이름 순 */
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	static TArray<ACTPlayerState*> GetSortedPlayers(const ACTGameState* GameState);

	/** 부모 위젯의 아이콘 설정을 물려받습니다. (내 설정이 비어 있을 때만) */
	void InheritColorIcons(const UCTUserWidget* From);

protected:
	/** 색별 도형 아이콘 (예: 빨강=원, 주황=삼각형 ...). 아이콘은 흰색으로 만들면 색이 입혀집니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|UI")
	TMap<ECTColor, TObjectPtr<UTexture2D>> ColorIcons;

	/** true면 아이콘 이미지에 해당 색을 입힙니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|UI")
	bool bTintColorIcon = true;
};
