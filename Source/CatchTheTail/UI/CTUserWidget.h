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

UCLASS(Abstract)
class CATCHTHETAIL_API UCTUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	ACTGameState* GetCTGameState() const;

	// 내 PlayerState (접속 직후 nullptr 가능)
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	ACTPlayerState* GetOwningCTPlayerState() const;

	UFUNCTION(BlueprintPure, Category = "CT|UI")
	ACTCharacter* GetOwningCTCharacter() const;

	// 색별 아이콘 (없으면 nullptr)
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	UTexture2D* GetColorIcon(ECTColor Color) const;

	// 색 이름/아이콘 설정 (None이면 숨김)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void ApplyColorToWidgets(ECTColor Color, UTextBlock* NameText, UImage* IconImage, FText NoneText) const;

	// 초 -> m:ss
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	static FText FormatTime(float Seconds);

	// 관전자 제외, 꼬리 수 내림차순 후 이름순
	UFUNCTION(BlueprintPure, Category = "CT|UI")
	static TArray<ACTPlayerState*> GetSortedPlayers(const ACTGameState* GameState);

	// 부모 아이콘 설정 상속
	void InheritColorIcons(const UCTUserWidget* From);

protected:
	// 색별 도형 아이콘 (흰색이면 색 입힘)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|UI")
	TMap<ECTColor, TObjectPtr<UTexture2D>> ColorIcons;

	// 아이콘에 색 입힘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|UI")
	bool bTintColorIcon = true;
};
