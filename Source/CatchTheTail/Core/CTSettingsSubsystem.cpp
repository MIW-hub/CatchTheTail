// Copyright Moon Party Games. All Rights Reserved.
#include "CTSettingsSubsystem.h"
#include "CTGameInstance.h"
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

	/** 행동 이름에 맞는 기본 한글 표시 이름 */
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

	/** 원본 컨텍스트에서 행동에 묶인 첫 번째 키보드/마우스 키 */
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
	ApplyDisplay();
	ApplyAudio(); // 아직 월드가 없으면 건너뜀. 맵이 열릴 때 컨트롤러가 다시 적용합니다.

	// 조작 목록 확인용 로그: 프로젝트 설정(Control Entries)이 잘 읽혔는지, 키가 보이는지
	UE_LOG(LogTemp, Log, TEXT("[CT] 설정 시스템 시작: 조작 항목 %d개 (프로젝트 설정)"), Config()->ControlEntries.Num());
	for (const FCTControlRow& Row : GetControlRows())
	{
		UE_LOG(LogTemp, Log, TEXT("[CT] 설정 조작: %s  바꿀 수 있음=%d  키 %d개 (%s)"),
			*Row.DisplayName.ToString(), Row.bRebindable ? 1 : 0, Row.Keys.Num(),
			Row.Keys.IsEmpty() ? TEXT("-") : *Row.Keys[0].GetDisplayName().ToString());
	}
}

// ============================================================
// 저장 / 불러오기
// ============================================================

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

// ============================================================
// 소리 / 화면 / 접근성
// ============================================================

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
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	USoundMix* Mix = Config()->SettingsSoundMix.LoadSynchronous();
	if (!World || !Mix) { return; }

	UGameplayStatics::SetBaseSoundMix(World, Mix);

	// 모두 끄기는 두 클래스의 볼륨을 0으로 만듭니다.
	const float MusicVolume = Data.bMuteAll ? 0.f : Data.MusicVolume;
	const float SfxVolume = Data.bMuteAll ? 0.f : Data.SfxVolume;

	if (USoundClass* MusicClass = Config()->MusicSoundClass.LoadSynchronous())
	{
		UGameplayStatics::SetSoundMixClassOverride(World, Mix, MusicClass, MusicVolume, 1.f, 0.f, true);
	}
	if (USoundClass* SfxClass = Config()->SfxSoundClass.LoadSynchronous())
	{
		UGameplayStatics::SetSoundMixClassOverride(World, Mix, SfxClass, SfxVolume, 1.f, 0.f, true);
	}
}

void UCTSettingsSubsystem::ApplyDisplay()
{
	// 에디터 플레이 창은 건드리지 않습니다.
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

void UCTSettingsSubsystem::ResetToDefaults()
{
	// 키 설정까지 처음 상태로. (바꾼 키가 들어간 컨텍스트는 ApplyKeyBindingsToLocalPlayer가 정리)
	Data = FCTSettingsData();

	ApplyKeyBindingsToLocalPlayer();
	EffectiveContexts.Empty();
	ApplyAudio();
	ApplyDisplay();
	CommitChange();
}

// ============================================================
// 조작 (키 변경)
// ============================================================

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

		// 키를 바꿀 수 있는 다른 행동은 "지금 쓰는 키"를 비교합니다.
		if (Entry.Id != ActionId && GetCurrentKey(Entry.Id) == Key)
		{
			return true;
		}
	}

	// 키를 바꿀 수 없는 행동(이동, 시점 등)이 쓰는 키는 원본 매핑에서 확인합니다.
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
			// 안내용: 이 행동에 묶인 키보드/마우스 키를 모두 보여 줍니다. (예: 이동 = W, A, S, D)
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

	// 기본 키와 같으면 "바꾼 키" 기록을 지웁니다.
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

		// 원본 에셋은 건드리지 않고 복사본에 바꾼 키를 넣습니다.
		if (!Copy)
		{
			Copy = DuplicateObject<UInputMappingContext>(Source, this);
		}

		// 원래 매핑의 모디파이어 / 트리거(눌림 방식 등)는 그대로 가져갑니다.
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
		return Source; // 바꾼 키가 없으면 원본 그대로
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

		// 게임 중(컨텍스트가 등록된 상태)일 때만 다시 등록합니다. 메뉴에서는 캐시만 비웁니다.
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

// ============================================================
// 개인 설정
// ============================================================

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
