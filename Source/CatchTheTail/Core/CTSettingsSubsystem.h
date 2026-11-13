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

	// 이 키가 하는 일 설명 (설정 창에서 줄 아래에 작게 보임)
	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	FText Description;

	// 키 이름 대신 보일 글자 (예: "1 ~ 9"). 비면 Keys 이름
	UPROPERTY(BlueprintReadOnly, Category = "CT|Settings")
	FText KeysLabel;
};

// 코드에서 직접 받는 키 (입력 매핑 에셋 밖). 설정 창에서 바꿀 수 있음
namespace CTCodeKeys
{
	inline const FName EmoteMenu(TEXT("EmoteMenu"));  // 이모트 창 열기/닫기 (기본 R)
	inline const FName Phone(TEXT("Phone"));          // 전화부스, 유령 이동 (기본 F)
	inline const FName TeamVoice(TEXT("TeamVoice"));  // 팀 음성 (보물찾기, 기본 T). 누르는 동안 팀원에게만, 거리와 상관없이 들림
	inline const FName GrabAlt(TEXT("GrabAlt"));      // 잡기 보조 / 꼬리 대결 연타 (기본 G)
}

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

	// 음성 채팅
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsMicEnabled() const { return Data.bMicEnabled; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool IsMicAlwaysOn() const { return Data.bMicAlwaysOn; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	float GetVoiceVolume() const { return Data.VoiceVolume; }

	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	float GetMicSensitivity() const { return Data.MicSensitivity; }

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

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetMicEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetMicAlwaysOn(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetVoiceVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetMicSensitivity(float Sensitivity);

	// 언어
	// 지원 문화 코드 목록
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	static TArray<FString> GetSupportedLanguages();

	// 그 언어로 쓴 이름. 번역 제외
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	static FString GetLanguageDisplayName(const FString& CultureCode);

	// 현재 언어 코드. 저장값 없으면 영어
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	FString GetLanguage() const;

	// 언어 변경. bConfirm false면 미리보기
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void SetLanguage(const FString& CultureCode, bool bConfirm = true);

	// 언어 선택 화면을 거쳤는가
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool HasChosenLanguage() const;

	// 현재 언어로 확정
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ConfirmLanguage();

	// 현재 언어 문구 적용 (시작 시 자동)
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ApplyLanguage();

	// 키 포함 전부 기본값. 이름 유지
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ResetToDefaults();

	// 볼륨/음소거 적용 (맵 변경 후 재적용)
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

	// 바꾼 키 컨텍스트. 없으면 원본
	UInputMappingContext* GetEffectiveMappingContext(UInputMappingContext* Source);

	// 로컬 플레이어에 키 재적용. 메뉴에선 무동작
	void ApplyKeyBindingsToLocalPlayer();

	// 코드에서 받는 키의 현재 키 (바꾼 키 우선, 없으면 기본 R, F, G)
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	FKey GetCodeKey(FName CodeKeyId) const;

	// 키 설정만 처음으로 (소리, 화면 등 다른 설정은 그대로)
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	void ResetKeyBindings();

	// 개인 설정
	// 이름 변경 가능 여부 (Allow Name Change)
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	bool CanChangePlayerName() const;

	// 이름 변경. 불허/규칙 위반 시 false + 이유
	UFUNCTION(BlueprintCallable, Category = "CT|Settings")
	bool ChangePlayerName(const FString& NewName, FText& OutError);

	// 저장된 이모트 등록 목록 (비어 있으면 저장된 적 없음)
	const TArray<int32>& GetSavedEmoteLoadout() const { return Data.EmoteLoadout; }

	// 이모트 등록 목록 저장
	void SaveEmoteLoadout(const TArray<int32>& Loadout);

	// 행동의 현재 키 (바꾼 키 우선). HUD 키 안내용
	UFUNCTION(BlueprintPure, Category = "CT|Settings")
	FKey GetActionKey(FName ActionId) const { return GetCurrentKey(ActionId); }

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
