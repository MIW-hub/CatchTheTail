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

/**
 * 배경음과 효과음 재생. 소리 크기는 설정 화면의 배경음/효과음 볼륨(사운드 클래스)을 따릅니다.
 */
UCLASS()
class CATCHTHETAIL_API UCTAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 효과음 (화면 소리, 어디서나 같은 크기)
	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void PlaySfx(ECTSfx Sfx);

	// 효과음 (월드 위치. 멀수록 작아지고 너무 멀면 안 들림)
	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void PlaySfxAtLocation(ECTSfx Sfx, FVector Location, float Volume = 1.f, float Pitch = 1.f);

	// 배경음 바꾸기. 이미 같은 곡이 나오고 있으면 그대로 둠 (맵이 바뀌어도 끊기지 않음)
	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void PlayMusic(ECTMusic Music);

	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void StopMusic();

	// 설정의 배경음 볼륨(음소거 포함)을 지금 나오는 배경음에 바로 적용. 설정이 바뀔 때 설정 시스템이 부릅니다.
	UFUNCTION(BlueprintCallable, Category = "CT|Audio")
	void RefreshMusicVolume();

	/**
	 * 위젯 안의 버튼과 체크박스에 소리를 붙입니다. (올림 / 누름)
	 * 이름에 Back, Close, Cancel, Leave가 들어간 버튼은 "뒤로가기" 소리, 나머지는 "확인" 소리입니다.
	 * 위젯 블루프린트를 고치지 않고 실행할 때 적용됩니다.
	 */
	static void ApplyUISounds(UUserWidget* Widget);

private:
	// 설정에서 읽은 배경음 볼륨 (음소거면 0)
	float GetSettingsMusicVolume() const;

	// 배경음이 끝나면 다시 시작 (반복 보강)
	UFUNCTION()
	void HandleMusicFinished();

	UPROPERTY(Transient)
	TMap<ECTSfx, TObjectPtr<USoundBase>> SfxCache;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> WorldAttenuation;

	// 반드시 강한 참조(UPROPERTY)로 들고 있어야 함. 이 컴포넌트는 소유자 없이 만들어져서 참조가 없으면 가비지 컬렉션에서 지워지고 배경음이 갑자기 끊김
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	// 페이드 아웃 중인 이전 곡. 끝까지 사라지도록 잡아 둠
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> FadingMusicComponents;

	ECTMusic CurrentMusic = ECTMusic::None;

	USoundBase* GetSfx(ECTSfx Sfx);
};
