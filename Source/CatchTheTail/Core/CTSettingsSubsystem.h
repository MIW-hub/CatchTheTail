// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InputCoreTypes.h"
#include "CTSettingsSaveGame.h"
#include "CTSettingsSubsystem.generated.h"

class UInputMappingContext;

// 설정 변경 시 (UI 갱신)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTSettingsChangedSignature);

// 조작 목록 한 줄
USTRUCT(BlueprintType)
struct FCTControlRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	FText DisplayName;

	// 현재 키. 변경 불가 행동은 안내용 전체 키
	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	TArray<FKey> Keys;

	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	bool bRebindable = false;
};

UCLASS()
class CATCHTHETAIL_API UCTSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "CT|Settings")
	FCTSettingsChangedSignature OnSettingsChanged;

	// 읽기
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	float GetMusicVolume() const { return Data.MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	float GetSfxVolume() const { return Data.SfxVolume; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsMuted() const { return Data.bMuteAll; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsFullscreen() const { return Data.bFullscreen; }

	// 카메라 흔들림 재생 전 확인
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsScreenShakeEnabled() const { return Data.bScreenShake; }

	// 색 구분 보조 사용 여부
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsTeamColorAssistEnabled() const { return Data.bTeamColorAssist; }

	// 머리 위 팀/색 표시 여부
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsTeamMarkerEnabled() const { return Data.bShowTeamMarker; }

	// 변경 (즉시 적용 + 저장)
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

	// 키 포함 전부 기본값. 이름 유지
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ResetToDefaults();

	// 적용 (맵 변경 후 재적용)
	// 볼륨/음소거 적용
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ApplyAudio();

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ApplyDisplay();

	// 조작
	// 조작 목록 (Control Entries 순서)
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	TArray<FCTControlRow> GetControlRows() const;

	// 키 변경. 중복/불가면 false + 이유. 게임 중이면 즉시 적용
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	bool SetKeyBinding(FName ActionId, FKey NewKey, FText& OutError);

	// 바꾼 키 반영 컨텍스트 (없으면 원본). 컨트롤러 등록 시 사용
	UInputMappingContext* GetEffectiveMappingContext(UInputMappingContext* Source);

	// 로컬 플레이어에 키 재적용. 메뉴에선 무동작
	void ApplyKeyBindingsToLocalPlayer();

	// 개인 설정
	// 이름 변경 가능 여부 (Allow Name Change)
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool CanChangePlayerName() const;

	// 이름 변경. 불허/규칙 위반 시 false + 이유
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	bool ChangePlayerName(const FString& NewName, FText& OutError);

private:
	void LoadSettings();
	void SaveSettings() const;

	// 변경 후 저장 + 알림
	void CommitChange();

	// 원본 컨텍스트의 기본 키
	FKey GetDefaultKey(FName ActionId) const;

	// 현재 키 (바꾼 키 우선)
	FKey GetCurrentKey(FName ActionId) const;

	// 다른 행동이 쓰는 키인가
	bool IsKeyUsedByOther(FName ActionId, FKey Key) const;

	FCTSettingsData Data;

	// 원본 → 바꾼 키 복사본
	UPROPERTY(Transient)
	TMap<TObjectPtr<UInputMappingContext>, TObjectPtr<UInputMappingContext>> EffectiveContexts;
};
