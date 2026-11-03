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

	// 로비 위젯의 경기 설정 항목 이름/값 글자를 코드의 현재 문구로 다시 채움 (문구가 바뀐 뒤 번역 충돌 방지). 바꾼 개수 반환
	UFUNCTION(BlueprintCallable, Category = "CT|Localization")
	static int32 RefreshMatchSettingTexts(UBlueprint* WidgetBlueprint);

	// 머티리얼 그래프의 코멘트 박스(MaterialExpressionComment) 크기 지정. 파이썬에서는 크기를 못 바꿔서 둠
	UFUNCTION(BlueprintCallable, Category = "CT|Localization")
	static void SetMaterialCommentSize(UObject* CommentNode, int32 Width, int32 Height);

	// 머티리얼에서 출력 어디에도 이어지지 않은 노드를 지움 (코멘트 박스는 남김). 지운 개수를 돌려줌
	UFUNCTION(BlueprintCallable, Category = "CT|Localization")
	static int32 RemoveUnusedMaterialNodes(UMaterial* Material);

	// 폰트에 보조 폰트 추가. 이미 있으면 false
	UFUNCTION(BlueprintCallable, Category = "CT|Localization")
	static bool AddFallbackFontFace(UFont* TargetFont, UObject* FontFaceAsset, FName EntryName);
};
