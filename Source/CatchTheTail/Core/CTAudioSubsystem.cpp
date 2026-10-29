// Copyright Moon Party Games. All Rights Reserved.
#include "CTAudioSubsystem.h"
#include "CTSettingsSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SlateSound.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateTypes.h"

void UCTAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 월드 위치 효과음용 거리 감쇠
	const UCTAudioSettings* Settings = GetDefault<UCTAudioSettings>();
	WorldAttenuation = NewObject<USoundAttenuation>(this);
	WorldAttenuation->Attenuation.bAttenuate = true;
	WorldAttenuation->Attenuation.bSpatialize = true;
	WorldAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	WorldAttenuation->Attenuation.AttenuationShapeExtents = FVector(Settings->WorldSfxInnerRadius, 0.f, 0.f);
	WorldAttenuation->Attenuation.FalloffDistance = Settings->WorldSfxFalloffDistance;
}

USoundBase* UCTAudioSubsystem::GetSfx(ECTSfx Sfx)
{
	if (const TObjectPtr<USoundBase>* Cached = SfxCache.Find(Sfx))
	{
		if (*Cached) { return *Cached; }
	}

	USoundBase* Sound = GetDefault<UCTAudioSettings>()->LoadSfx(Sfx);
	if (Sound) { SfxCache.Add(Sfx, Sound); }
	return Sound;
}

void UCTAudioSubsystem::PlaySfx(ECTSfx Sfx)
{
	UWorld* World = GetWorld();
	USoundBase* Sound = GetSfx(Sfx);
	if (!World || !Sound) { return; }

	UGameplayStatics::PlaySound2D(World, Sound);
}

void UCTAudioSubsystem::PlaySfxAtLocation(ECTSfx Sfx, FVector Location, float Volume, float Pitch)
{
	UWorld* World = GetWorld();
	USoundBase* Sound = GetSfx(Sfx);
	if (!World || !Sound) { return; }

	UGameplayStatics::PlaySoundAtLocation(World, Sound, Location, Volume, Pitch, 0.f, WorldAttenuation);
}

void UCTAudioSubsystem::PlayMusic(ECTMusic Music)
{
	if (Music == ECTMusic::None)
	{
		StopMusic();
		return;
	}

	// 같은 곡이 이미 나오고 있으면 그대로
	if (Music == CurrentMusic && IsValid(MusicComponent) && MusicComponent->IsPlaying()) { return; }

	UWorld* World = GetWorld();
	const UCTAudioSettings* Settings = GetDefault<UCTAudioSettings>();
	USoundBase* Sound = Settings->LoadMusic(Music);
	if (!World || !Sound) { return; }

#if WITH_EDITOR
	// 에디터 다중 창은 첫 창만 재생
	const FWorldContext* Context = GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr;
	if (Context && Context->WorldType == EWorldType::PIE && Context->PIEInstance > 0) { return; }
#endif

	const float Fade = Settings->MusicFadeSeconds;

	// 다 사라진 이전 곡은 정리
	FadingMusicComponents.RemoveAll([](const UAudioComponent* Old) { return !IsValid(Old) || !Old->IsPlaying(); });

	if (IsValid(MusicComponent))
	{
		// 이전 곡 소멸 시 반복 재시작 방지
		MusicComponent->OnAudioFinished.RemoveAll(this);
		MusicComponent->FadeOut(Fade, 0.f);
		FadingMusicComponents.Add(MusicComponent);
	}

	// 생성 후 FadeIn 한 번만 재생. persist, 자동 파괴 안 함
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(World, Sound, GetSettingsMusicVolume(), 1.f, 0.f, nullptr, /*bPersistAcrossLevelTransition=*/true, /*bAutoDestroy=*/false);
	if (Component)
	{
		Component->OnAudioFinished.AddDynamic(this, &UCTAudioSubsystem::HandleMusicFinished);
		Component->FadeIn(Fade, 1.f);
	}

	UE_LOG(LogTemp, Log, TEXT("[CT] 배경음 재생: %s (%s)"), *Sound->GetName(), Component ? TEXT("성공") : TEXT("실패"));

	MusicComponent = Component;
	CurrentMusic = Music;
}

float UCTAudioSubsystem::GetSettingsMusicVolume() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UCTSettingsSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<UCTSettingsSubsystem>() : nullptr;
	if (!Settings) { return 1.f; }
	return Settings->IsMuted() ? 0.f : Settings->GetMusicVolume();
}

void UCTAudioSubsystem::RefreshMusicVolume()
{
	// 사운드 믹스 대신 배경음에 직접 볼륨
	if (IsValid(MusicComponent))
	{
		MusicComponent->SetVolumeMultiplier(GetSettingsMusicVolume());
		UE_LOG(LogTemp, Log, TEXT("[CT] 배경음 볼륨 %.2f (재생 중: %s)"), GetSettingsMusicVolume(), MusicComponent->IsPlaying() ? TEXT("예") : TEXT("아니오"));
	}
}

void UCTAudioSubsystem::HandleMusicFinished()
{
	// 곡이 끝났는데 재생 대상이면 재시작
	if (CurrentMusic == ECTMusic::None || !IsValid(MusicComponent)) { return; }

	UE_LOG(LogTemp, Log, TEXT("[CT] 배경음이 끝나 다시 재생"));
	MusicComponent->Play(0.f);
	// 볼륨 0이었다면 설정 볼륨 재적용
	RefreshMusicVolume();
}

void UCTAudioSubsystem::StopMusic()
{
	if (IsValid(MusicComponent))
	{
		MusicComponent->OnAudioFinished.RemoveAll(this);
		MusicComponent->FadeOut(GetDefault<UCTAudioSettings>()->MusicFadeSeconds, 0.f);
		FadingMusicComponents.Add(MusicComponent);
	}
	MusicComponent = nullptr;
	CurrentMusic = ECTMusic::None;
}

void UCTAudioSubsystem::ApplyUISounds(UUserWidget* Widget)
{
	const UGameInstance* GameInstance = Widget ? Widget->GetGameInstance() : nullptr;
	UCTAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UCTAudioSubsystem>() : nullptr;
	if (!Audio || !Widget->WidgetTree) { return; }

	auto MakeSound = [Audio](ECTSfx Sfx)
	{
		FSlateSound Sound;
		Sound.SetResourceObject(Audio->GetSfx(Sfx));
		return Sound;
	};
	const FSlateSound HoverSound = MakeSound(ECTSfx::UIHover);
	const FSlateSound ConfirmSound = MakeSound(ECTSfx::UIConfirm);
	const FSlateSound BackSound = MakeSound(ECTSfx::UIBack);

	Widget->WidgetTree->ForEachWidget([&](UWidget* Child)
	{
		if (UButton* Button = Cast<UButton>(Child))
		{
			// 이름으로 뒤로가기 구분
			const FString Name = Button->GetName();
			const bool bBack = Name.Contains(TEXT("Back")) || Name.Contains(TEXT("Close")) || Name.Contains(TEXT("Cancel")) || Name.Contains(TEXT("Leave"));

			FButtonStyle Style = Button->GetStyle();
			Style.SetHoveredSound(HoverSound);
			Style.SetPressedSound(bBack ? BackSound : ConfirmSound);
			Button->SetStyle(Style);
		}
		else if (UCheckBox* CheckBox = Cast<UCheckBox>(Child))
		{
			FCheckBoxStyle Style = CheckBox->GetWidgetStyle();
			Style.SetHoveredSound(HoverSound);
			Style.SetCheckedSound(ConfirmSound);
			Style.SetUncheckedSound(ConfirmSound);
			CheckBox->SetWidgetStyle(Style);
		}
	});
}
