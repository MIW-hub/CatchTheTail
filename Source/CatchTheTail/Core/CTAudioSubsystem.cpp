// Copyright Moon Party Games. All Rights Reserved.
#include "CTAudioSubsystem.h"
#include "CTSettingsSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
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
	if (Music == CurrentMusic && MusicComponent.IsValid() && MusicComponent->IsPlaying()) { return; }

	UWorld* World = GetWorld();
	const UCTAudioSettings* Settings = GetDefault<UCTAudioSettings>();
	USoundBase* Sound = Settings->LoadMusic(Music);
	if (!World || !Sound) { return; }

	const float Fade = Settings->MusicFadeSeconds;
	if (MusicComponent.IsValid())
	{
		// 이전 곡이 사라질 때 반복 보강이 다시 켜지지 않게 먼저 해제
		MusicComponent->OnAudioFinished.RemoveAll(this);
		MusicComponent->FadeOut(Fade, 0.f);
	}

	// 만들기만 하고(CreateSound2D) 재생은 FadeIn 한 번만. 맵이 바뀌어도 이어지도록 persist, 끝나도 지우지 않고 우리가 관리
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
	// 사운드 믹스 대신 배경음에 직접 볼륨을 줌 (슬라이더를 내리는 만큼 확실히 작아짐)
	if (MusicComponent.IsValid())
	{
		MusicComponent->SetVolumeMultiplier(GetSettingsMusicVolume());
		UE_LOG(LogTemp, Log, TEXT("[CT] 배경음 볼륨 %.2f (재생 중: %s)"), GetSettingsMusicVolume(), MusicComponent->IsPlaying() ? TEXT("예") : TEXT("아니오"));
	}
}

void UCTAudioSubsystem::HandleMusicFinished()
{
	// 곡이 끝났는데 아직 이 곡을 틀어야 하면 처음부터 다시
	if (CurrentMusic == ECTMusic::None || !MusicComponent.IsValid()) { return; }

	UE_LOG(LogTemp, Log, TEXT("[CT] 배경음이 끝나 다시 재생"));
	MusicComponent->Play(0.f);
	// 볼륨 0 이었다면 다시 틀어도 안 들리므로 설정 볼륨을 다시 적용
	RefreshMusicVolume();
}

void UCTAudioSubsystem::StopMusic()
{
	if (MusicComponent.IsValid())
	{
		MusicComponent->OnAudioFinished.RemoveAll(this);
		MusicComponent->FadeOut(GetDefault<UCTAudioSettings>()->MusicFadeSeconds, 0.f);
	}
	MusicComponent.Reset();
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
			// 이름으로 뒤로가기 계열을 구분
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
