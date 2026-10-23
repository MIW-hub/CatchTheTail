// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CTLocalizationTools.generated.h"

class UFont;

/**
 * 번역 작업용 에디터 도구. 에디터 Python에서 호출합니다. (게임 실행 중에는 아무 일도 하지 않음)
 */
UCLASS()
class CATCHTHETAIL_API UCTLocalizationTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 위젯 블루프린트 안의 글자(Text, 입력칸 힌트)에 번역 키를 붙입니다.
	 * 번역 키가 없는 고정 글자는 번역 수집에서 빠지므로, 이 함수로 "CT_UI" 네임스페이스와 "블루프린트이름.위젯이름" 키를 부여합니다.
	 * 이미 키가 있는 글자는 건드리지 않습니다. 바꾼 글자 수를 돌려줍니다. 에셋 저장은 호출한 쪽에서 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|Localization")
	static int32 MakeWidgetTextsLocalizable(UBlueprint* WidgetBlueprint);

	/**
	 * 폰트에 보조 폰트(Fallback)를 추가합니다. 기본 폰트에 없는 글자(예: 한국어 폰트에 없는 일본어 글자)만 보조 폰트로 표시됩니다.
	 * 이미 같은 폰트 페이스가 있으면 아무것도 하지 않고 false. 에셋 저장은 호출한 쪽에서 합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|Localization")
	static bool AddFallbackFontFace(UFont* TargetFont, UObject* FontFaceAsset, FName EntryName);
};
