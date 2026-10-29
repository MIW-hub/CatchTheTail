// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CTLocalizationTools.generated.h"

class UFont;

// 번역 작업 에디터 도구
UCLASS()
class CATCHTHETAIL_API UCTLocalizationTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 위젯 글자에 번역 키 부여. 바꾼 개수 반환
	UFUNCTION(BlueprintCallable, Category = "CT|Localization")
	static int32 MakeWidgetTextsLocalizable(UBlueprint* WidgetBlueprint);

	// 폰트에 보조 폰트 추가. 이미 있으면 false
	UFUNCTION(BlueprintCallable, Category = "CT|Localization")
	static bool AddFallbackFontFace(UFont* TargetFont, UObject* FontFaceAsset, FName EntryName);
};
