// Copyright Moon Party Games. All Rights Reserved.
#include "CTSettingsSubsystem.h"
#include "CTGameInstance.h"
#include "CTAudioSubsystem.h"
#include "CTSettingsConfig.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameUserSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Misc/CommandLine.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	const TCHAR* const CTSettingsSlotName = TEXT("CTSettings");

	const UCTSettingsConfig* Config()
	{
		return GetDefault<UCTSettingsConfig>();
	}

	const FCTControlEntry* FindEntry(FName Id)
	{
		return Config()->ControlEntries.FindByPredicate([Id](const FCTControlEntry& Entry) { return Entry.Id == Id; });
	}

	// Id별 기본 한글명
	FText DisplayNameFor(const FCTControlEntry& Entry)
	{
		if (!Entry.DisplayName.IsEmpty())
		{
			return FText::FromString(Entry.DisplayName);
		}

		static const TMap<FName, FText> Defaults = {
			{ TEXT("Move"), NSLOCTEXT("CT", "Ctl_Move", "이동") },
			{ TEXT("Jump"), NSLOCTEXT("CT", "Ctl_Jump", "점프") },
			{ TEXT("Dash"), NSLOCTEXT("CT", "Ctl_Dash", "대시") },
			{ TEXT("Run"),  NSLOCTEXT("CT", "Ctl_Run", "달리기") },
			{ TEXT("Grab"), NSLOCTEXT("CT", "Ctl_Grab", "잡기") },
			{ TEXT("Push"), NSLOCTEXT("CT", "Ctl_Push", "밀기") },
			{ TEXT("Look"), NSLOCTEXT("CT", "Ctl_Look", "시점") },
		};
		if (const FText* Found = Defaults.Find(Entry.Id)) { return *Found; }
		return FText::FromName(Entry.Id);
	}

	// 원본 컨텍스트의 첫 키보드/마우스 키
	FKey FirstKeyboardMouseKey(const UInputMappingContext* Context, const UInputAction* Action)
	{
		if (!Context || !Action) { return EKeys::Invalid; }

		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Action == Action && !Mapping.Key.IsGamepadKey())
			{
				return Mapping.Key;
			}
		}
		return EKeys::Invalid;
	}
}

void UCTSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadSettings();

	// 테스트용: -CTTestMusicVolume=0.2 -CTTestSfxVolume=0.5 (저장하지 않음)
	float TestVolume = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("CTTestMusicVolume="), TestVolume)) { Data.MusicVolume = FMath::Clamp(TestVolume, 0.f, 1.f); }
	if (FParse::Value(FCommandLine::Get(), TEXT("CTTestSfxVolume="), TestVolume)) { Data.SfxVolume = FMath::Clamp(TestVolume, 0.f, 1.f); }

	ApplyLanguage();
	ApplyDisplay();
	ApplyAudio(); // 월드 없으면 생략. 맵 열릴 때 컨트롤러가 재적용

	// 조작 목록 확인용 로그
	UE_LOG(LogTemp, Log, TEXT("[CT] 설정 시스템 시작: 조작 항목 %d개 (프로젝트 설정)"), Config()->ControlEntries.Num());
	for (const FCTControlRow& Row : GetControlRows())
	{
		UE_LOG(LogTemp, Log, TEXT("[CT] 설정 조작: %s  바꿀 수 있음=%d  키 %d개 (%s)"),
			*Row.DisplayName.ToString(), Row.bRebindable ? 1 : 0, Row.Keys.Num(),
			Row.Keys.IsEmpty() ? TEXT("-") : *Row.Keys[0].GetDisplayName().ToString());
	}
}

// 저장 / 불러오기

void UCTSettingsSubsystem::LoadSettings()
{
	Data = FCTSettingsData();

	if (!UGameplayStatics::DoesSaveGameExist(CTSettingsSlotName, 0)) { return; }

	if (const UCTSettingsSaveGame* Saved = Cast<UCTSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(CTSettingsSlotName, 0)))
	{
		Data = Saved->Data;
		Data.MusicVolume = FMath::Clamp(Data.MusicVolume, 0.f, 1.f);
		Data.SfxVolume = FMath::Clamp(Data.SfxVolume, 0.f, 1.f);
	}
}

void UCTSettingsSubsystem::SaveSettings() const
{
	if (UCTSettingsSaveGame* Saved = Cast<UCTSettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(UCTSettingsSaveGame::StaticClass())))
	{
		Saved->Data = Data;
		UGameplayStatics::SaveGameToSlot(Saved, CTSettingsSlotName, 0);
	}
}

void UCTSettingsSubsystem::CommitChange()
{
	SaveSettings();
	OnSettingsChanged.Broadcast();
}

// 소리 / 화면 / 접근성

void UCTSettingsSubsystem::SetMusicVolume(float Volume)
{
	Data.MusicVolume = FMath::Clamp(Volume, 0.f, 1.f);
	ApplyAudio();
	CommitChange();
}

void UCTSettingsSubsystem::SetSfxVolume(float Volume)
{
	Data.SfxVolume = FMath::Clamp(Volume, 0.f, 1.f);
	ApplyAudio();
	CommitChange();
}

void UCTSettingsSubsystem::SetMuted(bool bMuted)
{
	Data.bMuteAll = bMuted;
	ApplyAudio();
	CommitChange();
}

void UCTSettingsSubsystem::SetFullscreen(bool bFullscreen)
{
	Data.bFullscreen = bFullscreen;
	ApplyDisplay();
	CommitChange();
}

void UCTSettingsSubsystem::SetScreenShakeEnabled(bool bEnabled)
{
	Data.bScreenShake = bEnabled;
	CommitChange();
}

void UCTSettingsSubsystem::SetTeamColorAssistEnabled(bool bEnabled)
{
	Data.bTeamColorAssist = bEnabled;
	CommitChange();
}

void UCTSettingsSubsystem::SetTeamMarkerEnabled(bool bEnabled)
{
	Data.bShowTeamMarker = bEnabled;
	CommitChange();
}

void UCTSettingsSubsystem::ApplyAudio()
{
	UGameInstance* GameInstance = GetGameInstance();

	// 배경음은 사운드 믹스 대신 지금 나오는 배경음에 직접 볼륨을 줌 (음소거 포함)
	if (UCTAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UCTAudioSubsystem>() : nullptr)
	{
		Audio->RefreshMusicVolume();
	}

	// 효과음은 사운드 믹스로 (버튼 소리, 발소리 등 여러 곳에서 나서)
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (!World) { return; } // 맵 열릴 때 컨트롤러가 다시 적용

	USoundMix* Mix = Config()->SettingsSoundMix.LoadSynchronous();
	if (!Mix)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT] 효과음 볼륨을 적용하지 못함: 사운드 믹스가 없습니다 (프로젝트 설정 CatchTheTail Settings 확인)"));
		return;
	}

	UGameplayStatics::SetBaseSoundMix(World, Mix);

	// 음소거는 볼륨 0
	const float SfxVolume = Data.bMuteAll ? 0.f : Data.SfxVolume;
	if (USoundClass* SfxClass = Config()->SfxSoundClass.LoadSynchronous())
	{
		UGameplayStatics::SetSoundMixClassOverride(World, Mix, SfxClass, SfxVolume, 1.f, 0.f, true);
	}
}

void UCTSettingsSubsystem::ApplyDisplay()
{
	// 에디터 플레이 창 제외
	if (GIsEditor || !GEngine) { return; }

	UGameUserSettings* UserSettings = GEngine->GetGameUserSettings();
	if (!UserSettings) { return; }

	const EWindowMode::Type Target = Data.bFullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed;
	if (UserSettings->GetFullscreenMode() != Target)
	{
		UserSettings->SetFullscreenMode(Target);
		UserSettings->ApplySettings(false);
	}
}

namespace
{
	// 지원 언어: 문화 코드와 그 언어로 쓴 이름
	struct FCTLanguage
	{
		const TCHAR* Code;
		const TCHAR* Name;
	};

	const FCTLanguage SupportedLanguages[] =
	{
		{ TEXT("ko"), TEXT("\uD55C\uAD6D\uC5B4") },
		{ TEXT("en"), TEXT("English") },
		{ TEXT("ja"), TEXT("\u65E5\u672C\u8A9E") },
	};

	bool IsSupportedLanguage(const FString& Code)
	{
		for (const FCTLanguage& Language : SupportedLanguages)
		{
			if (Code == Language.Code) { return true; }
		}
		return false;
	}
}

TArray<FString> UCTSettingsSubsystem::GetSupportedLanguages()
{
	TArray<FString> Result;
	for (const FCTLanguage& Language : SupportedLanguages)
	{
		Result.Add(Language.Code);
	}
	return Result;
}

FString UCTSettingsSubsystem::GetLanguageDisplayName(const FString& CultureCode)
{
	for (const FCTLanguage& Language : SupportedLanguages)
	{
		if (CultureCode == Language.Code) { return Language.Name; }
	}
	return CultureCode;
}

FString UCTSettingsSubsystem::GetLanguage() const
{
	// 실행 옵션 -CTLanguage=ja 로 언어를 강제 (테스트용)
	FString Forced;
	if (FParse::Value(FCommandLine::Get(), TEXT("CTLanguage="), Forced) && IsSupportedLanguage(Forced)) { return Forced; }

	if (IsSupportedLanguage(Data.Language)) { return Data.Language; }

	// 저장값이 없으면(처음 실행) 영어. 컴퓨터 언어는 따르지 않음
	return FString(TEXT("en"));
}

void UCTSettingsSubsystem::SetLanguage(const FString& CultureCode, bool bConfirm)
{
	if (!IsSupportedLanguage(CultureCode)) { return; }

	Data.Language = CultureCode;
	if (bConfirm) { Data.bLanguageChosen = true; }
	ApplyLanguage();
	CommitChange();
}

bool UCTSettingsSubsystem::HasChosenLanguage() const
{
	FString Forced;
	if (FParse::Value(FCommandLine::Get(), TEXT("CTLanguage="), Forced) && IsSupportedLanguage(Forced)) { return true; }
	return Data.bLanguageChosen && IsSupportedLanguage(Data.Language);
}

void UCTSettingsSubsystem::ConfirmLanguage()
{
	Data.Language = GetLanguage();
	Data.bLanguageChosen = true;
	CommitChange();
}

void UCTSettingsSubsystem::ApplyLanguage()
{
	const FString Code = GetLanguage();

#if WITH_EDITOR
	// 에디터에서는 에디터 화면 언어는 두고 게임 문구만 바꿈
	if (GIsEditor)
	{
		FTextLocalizationManager::Get().EnableGameLocalizationPreview(Code);
		return;
	}
#endif

	FInternationalization::Get().SetCurrentCulture(Code);
}

void UCTSettingsSubsystem::ResetToDefaults()
{
	// 키 포함 초기화. 컨텍스트 정리는 ApplyKeyBindingsToLocalPlayer
	// 언어는 초기화하지 않고 유지
	const FString KeepLanguage = Data.Language;
	const bool bKeepChosen = Data.bLanguageChosen;
	Data = FCTSettingsData();
	Data.Language = KeepLanguage;
	Data.bLanguageChosen = bKeepChosen;

	ApplyKeyBindingsToLocalPlayer();
	EffectiveContexts.Empty();
	ApplyAudio();
	ApplyDisplay();
	CommitChange();
}

// 조작 (키 변경)

FKey UCTSettingsSubsystem::GetDefaultKey(FName ActionId) const
{
	const FCTControlEntry* Entry = FindEntry(ActionId);
	if (!Entry) { return EKeys::Invalid; }

	return FirstKeyboardMouseKey(Entry->Context.LoadSynchronous(), Entry->Action.LoadSynchronous());
}

FKey UCTSettingsSubsystem::GetCurrentKey(FName ActionId) const
{
	if (const FKey* Overridden = Data.KeyOverrides.Find(ActionId))
	{
		return *Overridden;
	}
	return GetDefaultKey(ActionId);
}

bool UCTSettingsSubsystem::IsKeyUsedByOther(FName ActionId, FKey Key) const
{
	TSet<const UInputMappingContext*> Contexts;
	TSet<const UInputAction*> RebindableActions;

	for (const FCTControlEntry& Entry : Config()->ControlEntries)
	{
		if (const UInputMappingContext* Context = Entry.Context.LoadSynchronous())
		{
			Contexts.Add(Context);
		}

		if (!Entry.bRebindable) { continue; }

		if (const UInputAction* Action = Entry.Action.LoadSynchronous())
		{
			RebindableActions.Add(Action);
		}

		// 변경 가능 행동은 현재 키 비교
		if (Entry.Id != ActionId && GetCurrentKey(Entry.Id) == Key)
		{
			return true;
		}
	}

	// 변경 불가 행동(이동, 시점)의 키는 원본 매핑에서 확인
	for (const UInputMappingContext* Context : Contexts)
	{
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (RebindableActions.Contains(Mapping.Action)) { continue; }
			if (Mapping.Key == Key) { return true; }
		}
	}

	return false;
}

TArray<FCTControlRow> UCTSettingsSubsystem::GetControlRows() const
{
	TArray<FCTControlRow> Rows;

	for (const FCTControlEntry& Entry : Config()->ControlEntries)
	{
		FCTControlRow Row;
		Row.Id = Entry.Id;
		Row.DisplayName = DisplayNameFor(Entry);
		Row.bRebindable = Entry.bRebindable;

		if (Entry.bRebindable)
		{
			const FKey Current = GetCurrentKey(Entry.Id);
			if (Current.IsValid()) { Row.Keys.Add(Current); }
		}
		else if (const UInputMappingContext* Context = Entry.Context.LoadSynchronous())
		{
			// 안내용: 묶인 키보드/마우스 키 전부
			const UInputAction* Action = Entry.Action.LoadSynchronous();
			for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
			{
				if (Mapping.Action == Action && !Mapping.Key.IsGamepadKey())
				{
					Row.Keys.AddUnique(Mapping.Key);
				}
			}
		}

		Rows.Add(Row);
	}

	return Rows;
}

bool UCTSettingsSubsystem::SetKeyBinding(FName ActionId, FKey NewKey, FText& OutError)
{
	const FCTControlEntry* Entry = FindEntry(ActionId);
	if (!Entry || !Entry->bRebindable)
	{
		OutError = NSLOCTEXT("CT", "Key_NotRebindable", "이 동작의 키는 바꿀 수 없습니다.");
		return false;
	}

	if (!NewKey.IsValid() || NewKey.IsGamepadKey())
	{
		OutError = NSLOCTEXT("CT", "Key_Invalid", "사용할 수 없는 키입니다.");
		return false;
	}

	if (NewKey == EKeys::Escape)
	{
		OutError = NSLOCTEXT("CT", "Key_Escape", "Esc는 메뉴 키라서 쓸 수 없습니다.");
		return false;
	}

	if (IsKeyUsedByOther(ActionId, NewKey))
	{
		OutError = FText::Format(NSLOCTEXT("CT", "Key_InUse", "{0} 키는 이미 다른 동작에서 쓰고 있습니다."), NewKey.GetDisplayName());
		return false;
	}

	// 기본 키면 바꾼 키 기록 삭제
	if (NewKey == GetDefaultKey(ActionId))
	{
		Data.KeyOverrides.Remove(ActionId);
	}
	else
	{
		Data.KeyOverrides.Add(ActionId, NewKey);
	}

	ApplyKeyBindingsToLocalPlayer();
	CommitChange();
	OutError = FText::GetEmpty();
	return true;
}

UInputMappingContext* UCTSettingsSubsystem::GetEffectiveMappingContext(UInputMappingContext* Source)
{
	if (!Source) { return nullptr; }

	if (const TObjectPtr<UInputMappingContext>* Cached = EffectiveContexts.Find(Source))
	{
		return Cached->Get();
	}

	UInputMappingContext* Copy = nullptr;

	for (const FCTControlEntry& Entry : Config()->ControlEntries)
	{
		if (!Entry.bRebindable) { continue; }

		const FKey* Overridden = Data.KeyOverrides.Find(Entry.Id);
		if (!Overridden || Entry.Context.LoadSynchronous() != Source) { continue; }

		const UInputAction* Action = Entry.Action.LoadSynchronous();
		if (!Action) { continue; }

		// 원본 에셋 보존, 복사본에 적용
		if (!Copy)
		{
			Copy = DuplicateObject<UInputMappingContext>(Source, this);
		}

		// 기존 모디파이어/트리거 유지
		TArray<TObjectPtr<UInputModifier>> Modifiers;
		TArray<TObjectPtr<UInputTrigger>> Triggers;
		for (const FEnhancedActionKeyMapping& Mapping : Copy->GetMappings())
		{
			if (Mapping.Action == Action)
			{
				Modifiers = Mapping.Modifiers;
				Triggers = Mapping.Triggers;
				break;
			}
		}

		Copy->UnmapAllKeysFromAction(Action);
		FEnhancedActionKeyMapping& NewMapping = Copy->MapKey(Action, *Overridden);
		NewMapping.Modifiers = Modifiers;
		NewMapping.Triggers = Triggers;
	}

	if (!Copy)
	{
		return Source; // 바꾼 키 없으면 원본
	}

	EffectiveContexts.Add(Source, Copy);
	return Copy;
}

void UCTSettingsSubsystem::ApplyKeyBindingsToLocalPlayer()
{
	UGameInstance* GameInstance = GetGameInstance();
	ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

	TSet<UInputMappingContext*> Originals;
	for (const FCTControlEntry& Entry : Config()->ControlEntries)
	{
		if (UInputMappingContext* Context = Entry.Context.LoadSynchronous())
		{
			Originals.Add(Context);
		}
	}

	for (UInputMappingContext* Original : Originals)
	{
		UInputMappingContext* OldCopy = EffectiveContexts.FindRef(Original);

		// 게임 중(컨텍스트 등록 상태)만 재등록. 메뉴에선 캐시만 비움
		const bool bWasActive = Input &&
			(Input->HasMappingContext(Original) || (OldCopy && Input->HasMappingContext(OldCopy)));

		EffectiveContexts.Remove(Original);
		if (!bWasActive) { continue; }

		Input->RemoveMappingContext(Original);
		if (OldCopy)
		{
			Input->RemoveMappingContext(OldCopy);
		}

		Input->AddMappingContext(GetEffectiveMappingContext(Original), 0);
	}
}

// 개인 설정

bool UCTSettingsSubsystem::CanChangePlayerName() const
{
	return Config()->bAllowNameChange;
}

bool UCTSettingsSubsystem::ChangePlayerName(const FString& NewName, FText& OutError)
{
	if (!CanChangePlayerName())
	{
		OutError = NSLOCTEXT("CT", "Name_ChangeLocked", "이름은 바꿀 수 없습니다.");
		return false;
	}

	UCTGameInstance* GameInstance = Cast<UCTGameInstance>(GetGameInstance());
	if (!GameInstance) { return false; }

	return GameInstance->ChangePlayerName(NewName, OutError);
}
