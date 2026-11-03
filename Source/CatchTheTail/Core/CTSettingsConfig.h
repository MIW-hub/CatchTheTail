// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CTSettingsConfig.generated.h"

class UInputAction;
class UInputMappingContext;
class USoundClass;
class USoundMix;

// 조작 방법 목록의 한 줄
USTRUCT(BlueprintType)
struct FCTControlEntry
{
	GENERATED_BODY()

	// 행동 ID. 저장된 키 설정의 키
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	FName Id;

	// 비면 Id별 기본 한글명 (ini에 한글 금지)
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	FString DisplayName;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	TSoftObjectPtr<UInputAction> Action;

	// 이 행동 키가 있는 매핑 컨텍스트
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	TSoftObjectPtr<UInputMappingContext> Context;

	// false면 키 안내만 (이동 등 다중 키)
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	bool bRebindable = true;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CatchTheTail Settings"))
class CATCHTHETAIL_API UCTSettingsConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// 소리
	// 볼륨용 사운드 믹스. 비면 소리 설정 무효
	UPROPERTY(Config, EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundMix> SettingsSoundMix;

	// 배경음 클래스. BGM이 속해야 볼륨 적용
	UPROPERTY(Config, EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	// 효과음 클래스
	UPROPERTY(Config, EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundClass> SfxSoundClass;

	// 조작
	// 조작 목록 (위에서부터 표시)
	UPROPERTY(Config, EditAnywhere, Category = "Controls")
	TArray<FCTControlEntry> ControlEntries;

	// 개인 설정
	// true면 설정 창에서 이름 변경 가능
	UPROPERTY(Config, EditAnywhere, Category = "Personal")
	bool bAllowNameChange = false;
};
