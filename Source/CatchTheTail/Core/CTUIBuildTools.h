// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CTUIBuildTools.generated.h"

class UBlueprint;
class UMaterialInterface;
class UTexture2D;

/**
 * 위젯 블루프린트의 위젯 트리를 코드로 만드는 에디터 도구. 에디터 Python에서 호출합니다. (게임 실행 중에는 아무 일도 하지 않음)
 */
UCLASS()
class CATCHTHETAIL_API UCTUIBuildTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 설정 화면(WBP_CTSettingsWidget)의 위젯 트리를 새 디자인으로 다시 만듭니다. 기존 트리는 모두 지웁니다.
	 * 코드(CTSettingsWidget)가 이름으로 찾는 위젯은 모두 같은 이름으로 만듭니다. 만든 위젯 수를 돌려주고, 실패하면 0.
	 * 컴파일과 저장은 호출한 쪽에서 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildSettingsWidgetDesign(UBlueprint* WidgetBlueprint);

	/**
	 * 언어 선택 화면(WBP_CTLanguageSelectWidget)의 위젯 트리를 만듭니다. 기존 트리는 지웁니다.
	 * 코드(CTLanguageSelectWidget)가 이름으로 찾는 Button_LanguageKo, En, Ja, Button_Confirm 을 만듭니다. 만든 위젯 수를 돌려주고, 실패하면 0.
	 * 컴파일과 저장은 호출한 쪽에서 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildLanguageSelectDesign(UBlueprint* WidgetBlueprint);

	/**
	 * 설정 화면(WBP_CTSettingsWidget)을 다듬습니다. 이미 만든 트리를 그대로 두고 일부만 바꿉니다.
	 * - 버튼/체크박스 안의 글자는 줄바꿈을 끔 (게임에서 글자가 세로로 꺾이던 문제)
	 * - 언어 콤보박스를 한국어 / English / 日本語 버튼 3개로 교체 (Button_LanguageKo, En, Ja)
	 * - 조작 방법을 키 변경 목록 대신 고정 안내 줄(이동 W A S D 등)로 교체
	 * 여러 번 불러도 됩니다. 성공하면 true. 컴파일과 저장은 호출한 쪽에서 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool RefineSettingsWidget(UBlueprint* WidgetBlueprint);

	/**
	 * 설정 화면의 이름 변경 영역을 "이름 변경" 버튼 + 누르면 펼쳐지는 입력 칸으로 바꿉니다.
	 * Button_ChangeNameOpen 을 추가하고, 입력 칸(Row_NameChange)은 처음에 접어 두며, 칸 안의 버튼 글자는 "확인"으로 바꿉니다.
	 * 여러 번 불러도 됩니다. 성공하면 true. 컴파일과 저장은 호출한 쪽에서 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool RefineNameChange(UBlueprint* WidgetBlueprint);

	// 위젯 트리를 글자로 설명 (이름, 종류, 캔버스 위치)
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static FString DescribeWidgetTree(UBlueprint* WidgetBlueprint);

	// 점수판 한 줄(WBP_CTScoreEntryWidget)에 1등 왕관 이미지(Image_Crown)를 추가. 이미 있으면 이미지만 바꿉니다. 성공하면 true
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static bool AddScoreEntryCrown(UBlueprint* WidgetBlueprint, UTexture2D* CrownTexture);

	/**
	 * HUD(WBP_CTHUDWidget)의 스킬 칸을 정리합니다. 기존 위젯은 지우지 않습니다.
	 * - 달리기/잡기 쿨다운 이미지에 쿨다운 머티리얼을 넣고, 밀기 칸(패널, 아이콘, 쿨다운 이미지)을 추가
	 * - 칸마다 남은 시간 글자(Text_RunCooldown, Text_GrabCooldown, Text_PushCooldown)를 추가
	 * - 칸을 잡기, 밀기, 달리기 순서로 같은 간격으로 배치
	 * 이미 있는 위젯은 다시 만들지 않으므로 여러 번 불러도 됩니다. 새로 만든 위젯 수를 돌려주고, 실패하면 -1.
	 * 컴파일과 저장은 호출한 쪽에서 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	static int32 BuildHudSkillSlots(UBlueprint* WidgetBlueprint, UMaterialInterface* CooldownMaterial);
};
