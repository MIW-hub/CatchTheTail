// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CTAudioSettings.h"
#include "CTAudioSubsystem.generated.h"

class UAudioComponent;
class USoundAttenuation;
class USoundBase;
class UUserWidget;

// 배경음/효과음 재생
UCLASS()
class CATCHTHETAIL_API UCTAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 2D 효과음
	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void PlaySfx(ECTSfx Sfx);

	// 월드 위치 효과음 (거리 감쇠)
	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void PlaySfxAtLocation(ECTSfx Sfx, FVector Location, float Volume = 1.f, float Pitch = 1.f);

	// 배경음 변경. 같은 곡이면 유지
	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void PlayMusic(ECTMusic Music);

	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void StopMusic();

	// 배경음 볼륨 즉시 적용
	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void RefreshMusicVolume();

	// 위젯 버튼/체크박스에 소리 부착
	static void ApplyUISounds(UUserWidget* Widget);

private:
	// 설정 배경음 볼륨 (음소거 0)
	float GetSettingsMusicVolume() const;

	// 곡 끝나면 재시작
	UFUNCTION()
	void HandleMusicFinished();

	UPROPERTY(Transient)
	TMap<ECTSfx, TObjectPtr<USoundBase>> SfxCache;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> WorldAttenuation;

	// 소유자 없는 컴포넌트라 강한 참조 필수
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	// 페이드 아웃 중인 이전 곡
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> FadingMusicComponents;

	ECTMusic CurrentMusic = ECTMusic::None;

	USoundBase* GetSfx(ECTSfx Sfx);
};
