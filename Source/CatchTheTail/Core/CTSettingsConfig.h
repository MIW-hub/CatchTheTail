// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CTSettingsConfig.generated.h"

class UInputAction;
class UInputMappingContext;
class USoundClass;
class USoundMix;

/** 설정 창의 "조작 방법" 목록에 나오는 행동 한 줄 */
USTRUCT(BlueprintType)
struct FCTControlEntry
{
	GENERATED_BODY()

	/** 행동 이름 (Move, Jump, Dash, Grab, Push ...). 저장된 키 설정이 이 이름을 씁니다. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	FName Id;

	/** 표시 이름. 비워 두면 Id에 맞는 기본 한글 이름을 씁니다. (ini에는 한글을 쓰지 않는 것이 안전함) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	FString DisplayName;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	TSoftObjectPtr<UInputAction> Action;

	/** 이 행동의 키가 들어 있는 매핑 컨텍스트 (예: IMC_Move) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	TSoftObjectPtr<UInputMappingContext> Context;

	/** true면 키를 바꿀 수 있고, false면 키 안내만 보여 줍니다. (이동처럼 키 여러 개가 묶인 행동은 안내만) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Controls")
	bool bRebindable = true;
};

/**
 * 설정 창이 쓰는 프로젝트 설정. (프로젝트 세팅 → 게임 → CatchTheTail Settings)
 * DefaultGame.ini의 [/Script/CatchTheTail.CTSettingsConfig]에 저장됩니다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CatchTheTail Settings"))
class CATCHTHETAIL_API UCTSettingsConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ----- 소리 -----
	/** 볼륨 설정에 쓰는 사운드 믹스 (비워 두면 소리 설정이 적용되지 않음) */
	UPROPERTY(Config, EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundMix> SettingsSoundMix;

	/** 배경음에 쓰는 사운드 클래스. 배경음 사운드(BGM)가 이 클래스에 속해야 볼륨이 적용됩니다. */
	UPROPERTY(Config, EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	/** 효과음에 쓰는 사운드 클래스 */
	UPROPERTY(Config, EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundClass> SfxSoundClass;

	// ----- 조작 -----
	/** 조작 방법에 나올 행동 목록 (위에서부터 표시) */
	UPROPERTY(Config, EditAnywhere, Category = "Controls")
	TArray<FCTControlEntry> ControlEntries;

	// ----- 개인 설정 -----
	/**
	 * true면 설정 창에서 이름을 바꿀 수 있습니다.
	 * 기본은 false: 이름은 처음 한 번만 정할 수 있습니다.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Personal")
	bool bAllowNameChange = false;
};
