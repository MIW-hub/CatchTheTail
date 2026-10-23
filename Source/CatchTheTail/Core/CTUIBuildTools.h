// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CTUIBuildTools.generated.h"

class UBlueprint;

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
};
