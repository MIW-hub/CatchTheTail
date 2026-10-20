// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InputCoreTypes.h"
#include "CTSettingsSaveGame.h"
#include "CTSettingsSubsystem.generated.h"

class UInputMappingContext;

/** 설정이 바뀔 때마다 (UI 갱신용) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTSettingsChangedSignature);

/** "조작 방법" 목록의 한 줄 */
USTRUCT(BlueprintType)
struct FCTControlRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	FText DisplayName;

	/** 지금 쓰는 키. 키를 바꿀 수 있는 행동이 아니면 키 안내용 목록(예: W, A, S, D) */
	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	TArray<FKey> Keys;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	bool bRebindable = false;
};

/**
 * 설정을 저장하고 적용하는 곳. (게임 인스턴스에 하나, 메뉴와 게임 맵에서 같은 값을 씀)
 *
 *  - 소리: 배경음/효과음 볼륨, 모두 끄기 → 사운드 믹스의 클래스 볼륨으로 적용
 *  - 화면: 전체 화면 → 게임 사용자 설정(창 모드)으로 적용, 화면 흔들림 → 흔들림 재생 때 확인
 *  - 접근성: 색 구분 보조, 캐릭터 위 팀 표시 → 값만 저장합니다. (보여 주는 쪽에서 읽어서 사용)
 *  - 조작: 키 변경 → 매핑 컨텍스트의 복사본에 바꾼 키를 넣어서 씀 (원본 에셋은 바뀌지 않음)
 */
UCLASS()
class CATCHTHETAIL_API UCTSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "CT|Settings")
	FCTSettingsChangedSignature OnSettingsChanged;

	// ===== 읽기 =====
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	float GetMusicVolume() const { return Data.MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	float GetSfxVolume() const { return Data.SfxVolume; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsMuted() const { return Data.bMuteAll; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsFullscreen() const { return Data.bFullscreen; }

	/** 화면 흔들림을 보여 줄까? 카메라 흔들림을 재생하기 전에 확인하세요. */
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsScreenShakeEnabled() const { return Data.bScreenShake; }

	/** 색 구분 보조(색 이름 + 도형 함께 표시)를 쓸까? */
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsTeamColorAssistEnabled() const { return Data.bTeamColorAssist; }

	/** 캐릭터 머리 위에 팀/색 표시를 보여 줄까? */
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsTeamMarkerEnabled() const { return Data.bShowTeamMarker; }

	// ===== 바꾸기 (바꾸면 바로 적용하고 저장) =====
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetSfxVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetMuted(bool bMuted);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetFullscreen(bool bFullscreen);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetScreenShakeEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetTeamColorAssistEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetTeamMarkerEnabled(bool bEnabled);

	/** 모든 설정(키 포함)을 기본값으로 되돌립니다. 이름은 그대로 둡니다. */
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ResetToDefaults();

	// ===== 적용 (맵이 바뀐 뒤 다시 적용할 때 호출) =====
	/** 볼륨 / 음소거를 사운드 믹스에 적용 */
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ApplyAudio();

	/** 전체 화면 적용 */
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ApplyDisplay();

	// ===== 조작 =====
	/** 조작 방법 목록 (프로젝트 설정의 Control Entries 순서) */
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	TArray<FCTControlRow> GetControlRows() const;

	/**
	 * 행동의 키를 바꿉니다. 이미 다른 행동이 쓰는 키이거나 쓸 수 없는 키면 false와 이유를 돌려줍니다.
	 * 게임 중이면 바로 적용됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	bool SetKeyBinding(FName ActionId, FKey NewKey, FText& OutError);

	/**
	 * 바꾼 키가 들어간 매핑 컨텍스트를 돌려줍니다. 바꾼 키가 없으면 원본을 그대로 돌려줍니다.
	 * 플레이어 컨트롤러가 매핑 컨텍스트를 등록할 때 이 함수를 거치세요.
	 */
	UInputMappingContext* GetEffectiveMappingContext(UInputMappingContext* Source);

	/** 지금 게임 중인 로컬 플레이어의 키 설정을 다시 적용합니다. (메뉴에서는 아무 일도 안 함) */
	void ApplyKeyBindingsToLocalPlayer();

	// ===== 개인 설정 =====
	/** 설정 창에서 이름을 바꿀 수 있는가? (프로젝트 설정의 Allow Name Change) */
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool CanChangePlayerName() const;

	/** 이름 변경. 허용되지 않았거나 규칙에 맞지 않으면 false와 이유 */
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	bool ChangePlayerName(const FString& NewName, FText& OutError);

private:
	void LoadSettings();
	void SaveSettings() const;

	/** 값이 바뀐 뒤 저장하고 알림 */
	void CommitChange();

	/** 행동의 기본 키 (원본 매핑 컨텍스트에서 읽음) */
	FKey GetDefaultKey(FName ActionId) const;

	/** 행동이 지금 쓰는 키 (바꾼 키가 있으면 그것, 없으면 기본 키) */
	FKey GetCurrentKey(FName ActionId) const;

	/** 다른 행동이 이미 쓰는 키인가? */
	bool IsKeyUsedByOther(FName ActionId, FKey Key) const;

	FCTSettingsData Data;

	/** 원본 컨텍스트 → 바꾼 키가 들어간 복사본 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UInputMappingContext>, TObjectPtr<UInputMappingContext>> EffectiveContexts;
};
