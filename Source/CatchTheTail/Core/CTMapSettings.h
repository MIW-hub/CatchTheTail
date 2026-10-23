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

	// 화면에 표시할 맵 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map")
	FText DisplayName;

	// 맵 설명 (선택 창과 로비 미리보기에 표시)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map", meta = (MultiLine = "true"))
	FText Description;

	// 맵 이미지 (썸네일). 비워 두면 이미지 없이 표시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	// 실제로 열릴 레벨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map")
	TSoftObjectPtr<UWorld> Level;

	// 끄면 목록에 나오지 않음 (준비 중인 맵 숨기기)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|Map")
	bool bEnabled = true;
};

/**
 * 로비 맵 선택 목록. 프로젝트 설정 → Game → CT Maps 에서 편집합니다.
 * 서버와 클라이언트가 같은 목록을 보도록 프로젝트 설정에 둡니다. (선택한 번호만 서버가 복제)
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CT Maps"))
class CATCHTHETAIL_API UCTMapSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCTMapSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// 목록의 맵 중 선택 가능한 것(bEnabled)의 개수
	static int32 GetMapCount();

	// 번호로 맵 정보 얻기. 범위 밖이거나 비활성이면 false
	static bool GetMapInfo(int32 Index, FCTMapInfo& OutInfo);

	// 선택 가능한 맵 중 첫 번호. 없으면 INDEX_NONE
	static int32 GetFirstEnabledIndex();

	// 이동에 쓸 레벨 이름 (예: /Game/Levels/TypeA). 비어 있으면 빈 문자열
	static FString GetTravelName(int32 Index);

	// 맵 목록. 순서가 선택 창에 나오는 순서입니다. 번호(0부터)가 서버와 클라이언트 사이에 오가므로 중간에 끼워 넣으면 선택이 밀릴 수 있습니다.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Maps")
	TArray<FCTMapInfo> Maps;
};
