// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CTAudioSettings.generated.h"

class USoundBase;

// 게임에서 쓰는 효과음 종류
UENUM(BlueprintType)
enum class ECTSfx : uint8
{
	UIHover        UMETA(DisplayName = "UI Hover"),         // 버튼에 마우스를 올림
	UIConfirm      UMETA(DisplayName = "UI Confirm"),       // 버튼 누름
	UIBack         UMETA(DisplayName = "UI Back"),          // 뒤로가기, 닫기, 취소
	CountdownTick  UMETA(DisplayName = "Countdown Tick"),   // 카운트다운 숫자가 바뀜
	RoundStart     UMETA(DisplayName = "Round Start"),      // 라운드 시작
	TailGained     UMETA(DisplayName = "Tail Gained"),      // 내 꼬리가 늘어남
	TailLost       UMETA(DisplayName = "Tail Lost"),        // 꼬리를 잡힘
	Push           UMETA(DisplayName = "Push"),             // 밀기
	GhostRespawn   UMETA(DisplayName = "Ghost Respawn"),    // 유령에서 부활
	Victory        UMETA(DisplayName = "Victory"),          // 승리
	Footstep       UMETA(DisplayName = "Footstep"),         // 발소리
	Jump           UMETA(DisplayName = "Jump"),             // 점프
	Land           UMETA(DisplayName = "Land")              // 착지
};

// 배경음 종류
UENUM(BlueprintType)
enum class ECTMusic : uint8
{
	None UMETA(DisplayName = "None"),
	Menu UMETA(DisplayName = "Menu"),  // 메인 메뉴, 대기 로비
	Game UMETA(DisplayName = "Game")   // 게임 중
};

// 사운드 지정 (프로젝트 설정 → CT Audio)
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CT Audio"))
class CATCHTHETAIL_API UCTAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCTAudioSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// 효과음 얻기. 지정이 없으면 nullptr
	USoundBase* LoadSfx(ECTSfx Sfx) const;

	USoundBase* LoadMusic(ECTMusic Music) const;

	// 배경음
	UPROPERTY(Config, EditAnywhere, Category = "Music")
	TSoftObjectPtr<USoundBase> MenuMusic;

	UPROPERTY(Config, EditAnywhere, Category = "Music")
	TSoftObjectPtr<USoundBase> GameMusic;

	// 곡 전환 페이드 시간(초)
	UPROPERTY(Config, EditAnywhere, Category = "Music", meta = (ClampMin = "0"))
	float MusicFadeSeconds = 1.0f;

	// 효과음
	UPROPERTY(Config, EditAnywhere, Category = "SFX|UI")
	TSoftObjectPtr<USoundBase> UIHover;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|UI")
	TSoftObjectPtr<USoundBase> UIConfirm;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|UI")
	TSoftObjectPtr<USoundBase> UIBack;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Game")
	TSoftObjectPtr<USoundBase> CountdownTick;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Game")
	TSoftObjectPtr<USoundBase> RoundStart;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Game")
	TSoftObjectPtr<USoundBase> TailGained;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Game")
	TSoftObjectPtr<USoundBase> TailLost;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Game")
	TSoftObjectPtr<USoundBase> Push;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Game")
	TSoftObjectPtr<USoundBase> GhostRespawn;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Game")
	TSoftObjectPtr<USoundBase> Victory;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Movement")
	TSoftObjectPtr<USoundBase> Footstep;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Movement")
	TSoftObjectPtr<USoundBase> Jump;

	UPROPERTY(Config, EditAnywhere, Category = "SFX|Movement")
	TSoftObjectPtr<USoundBase> Land;

	// 최대 크기로 들리는 거리(cm)
	UPROPERTY(Config, EditAnywhere, Category = "SFX|World", meta = (ClampMin = "0"))
	float WorldSfxInnerRadius = 600.f;

	// 감쇠 끝 거리(cm). 넘으면 안 들림
	UPROPERTY(Config, EditAnywhere, Category = "SFX|World", meta = (ClampMin = "0"))
	float WorldSfxFalloffDistance = 2500.f;
};
