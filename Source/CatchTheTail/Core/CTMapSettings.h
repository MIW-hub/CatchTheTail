// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CTMapSettings.generated.h"

class UTexture2D;
class UWorld;

// 로비에서 고를 수 있는 맵 한 개
USTRUCT(BlueprintType)
struct FCTMapInfo
{
	GENERATED_BODY()

	// 표시 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map")
	FText DisplayName;

	// 맵 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map", meta = (MultiLine = "true"))
	FText Description;

	// 썸네일. 비면 이미지 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	// 열릴 레벨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map")
	TSoftObjectPtr<UWorld> Level;

	// 끄면 목록에서 숨김
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map")
	bool bEnabled = true;
};

// 로비 맵 선택 목록 (프로젝트 설정 → CT Maps)
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CT Maps"))
class CATCHTHETAIL_API UCTMapSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCTMapSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// 선택 가능한 맵 수
	static int32 GetMapCount();

	// 번호로 맵 정보 얻기. 범위 밖/비활성이면 false
	static bool GetMapInfo(int32 Index, FCTMapInfo& OutInfo);

	// 선택 가능한 첫 번호. 없으면 INDEX_NONE
	static int32 GetFirstEnabledIndex();

	// 이동용 레벨 이름. 없으면 빈 문자열
	static FString GetTravelName(int32 Index);

	// 맵 목록. 번호가 복제되므로 순서 유지
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Maps")
	TArray<FCTMapInfo> Maps;
};
