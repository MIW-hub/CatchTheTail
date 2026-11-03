// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CTUIBuildTools.generated.h"

class UBlueprint;
class UMaterialInterface;
class UTexture2D;

// 위젯 트리 구성 에디터 도구
UCLASS()
class CATCHTHETAIL_API UCTUIBuildTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 설정 화면 위젯 트리 재구성
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildSettingsWidgetDesign(UBlueprint* WidgetBlueprint);

	// 언어 선택 화면 위젯 트리 구성
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildLanguageSelectDesign(UBlueprint* WidgetBlueprint);

	// 설정 화면 다듬기
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool RefineSettingsWidget(UBlueprint* WidgetBlueprint);

	// 이름 변경 영역 교체
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool RefineNameChange(UBlueprint* WidgetBlueprint);

	// 로비 맵 설정 창 구성
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool BuildMatchSettingsPanel(UBlueprint* WidgetBlueprint);

	// 로딩 화면(WBP_CTLoadingWidget) 위젯 트리 구성: 배경, 참가 인원, 로딩 문구, 진행 막대, 게임 설명
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildLoadingScreen(UBlueprint* WidgetBlueprint, UTexture2D* BackgroundTexture);

	// ESC 메뉴(WBP_CTPauseMenuWidget) 위젯 트리 구성: 메인(계속하기/설정/기권/게임 종료), 설정 자리, 확인 팝업
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildPauseMenu(UBlueprint* WidgetBlueprint);

	// 위젯 트리를 글자로 설명 (이름, 종류, 캔버스 위치)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static FString DescribeWidgetTree(UBlueprint* WidgetBlueprint);

	// 점수판 한 줄(WBP_CTScoreEntryWidget)에 1등 왕관 이미지(Image_Crown) 추가
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool AddScoreEntryCrown(UBlueprint* WidgetBlueprint, UTexture2D* CrownTexture);

	// HUD 스킬 칸 정리
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildHudSkillSlots(UBlueprint* WidgetBlueprint, UMaterialInterface* CooldownMaterial);

	// HUD(WBP_CTHUDWidget) 전체 재구성: 타겟/시간/순위표 카드, 내 꼬리, 키 안내
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildHudLayout(UBlueprint* WidgetBlueprint, UMaterialInterface* CooldownMaterial);

	// 번역 키 없는 한글 글자(TextBlock)에 키를 붙임. 바꾼 개수를 돌려줌
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 KeyUnlocalizedTexts(UBlueprint* WidgetBlueprint);

	// HUD 순위표 한 줄(흰 카드 안에 들어가는 작은 줄) 구성
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildHudScoreEntry(UBlueprint* WidgetBlueprint);
};
