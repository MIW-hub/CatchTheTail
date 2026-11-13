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
#include "Net/VoiceConfig.h"
#include "HAL/IConsoleManager.h"
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
			{ TEXT("Voice"), NSLOCTEXT("CT", "Ctl_Voice", "음성 말하기") },
		};
		if (const FText* Found = Defaults.Find(Entry.Id)) { return *Found; }
		return FText::FromName(Entry.Id);
	}

	// 현재 언어에 맞는 문구 (한/영/일)
	FText Loc3(const TCHAR* Ko, const TCHAR* En, const TCHAR* Ja)
	{
		const FString Language = FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName();
		if (Language == TEXT("ko")) { return FText::FromString(Ko); }
		if (Language == TEXT("ja")) { return FText::FromString(Ja); }
		return FText::FromString(En);
	}

	bool IsCodeKeyId(FName Id)
	{
		return Id == CTCodeKeys::EmoteMenu || Id == CTCodeKeys::Phone || Id == CTCodeKeys::GrabAlt;
	}

	FKey DefaultCodeKey(FName Id)
	{
		if (Id == CTCodeKeys::EmoteMenu) { return EKeys::R; }
		if (Id == CTCodeKeys::Phone) { return EKeys::F; }
		if (Id == CTCodeKeys::GrabAlt) { return EKeys::G; }
		if (Id == CTCodeKeys::TeamVoice) { return EKeys::T; }
		return EKeys::Invalid;
	}

	// 숫자 키 1~9는 이모트용이라 다른 동작에 못 씀
	bool IsEmoteNumberKey(const FKey& Key)
	{
		static const FKey Numbers[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
		for (const FKey& Number : Numbers) { if (Number == Key) { return true; } }
		return false;
	}

	FText DescriptionFor(FName Id)
	{
		if (Id == TEXT("Move")) { return Loc3(TEXT("캐릭터를 움직여요."), TEXT("Move your character."), TEXT("キャラクターを動かします。")); }
		if (Id == TEXT("Look")) { return Loc3(TEXT("마우스로 시점을 돌려요."), TEXT("Turn the camera with the mouse."), TEXT("マウスで視点を回します。")); }
		if (Id == TEXT("Jump")) { return Loc3(TEXT("점프해요."), TEXT("Jump."), TEXT("ジャンプします。")); }
		if (Id == TEXT("Dash") || Id == TEXT("Run")) { return Loc3(TEXT("잠깐 빠르게 달려요. 쓰고 나면 쉬어야 해요."), TEXT("Sprint for a moment. It needs a short rest after."), TEXT("少しの間速く走ります。使ったあとは休みが必要です。")); }
		if (Id == TEXT("Grab")) { return Loc3(TEXT("상대 꼬리를 잡아요. 잡은 뒤에는 연타해서 당겨요."), TEXT("Grab an opponent's tail, then mash to pull it."), TEXT("相手のしっぽをつかみます。つかんだあとは連打で引っ張ります。")); }
		if (Id == TEXT("Push")) { return Loc3(TEXT("앞에 있는 상대를 밀어내요."), TEXT("Shove the player in front of you."), TEXT("前にいる相手を押します。")); }
		if (Id == TEXT("Voice")) { return Loc3(TEXT("누르고 있는 동안 말해요. 가까이 있는 사람에게 들려요."), TEXT("Hold to talk. Nearby players can hear you."), TEXT("押している間話せます。近くの人に聞こえます。")); }
		if (Id == CTCodeKeys::EmoteMenu) { return Loc3(TEXT("이모트 창을 열고 닫아요. 숫자 키에 등록할 수 있어요."), TEXT("Open or close the emote menu. Assign emotes to the number keys."), TEXT("エモートウィンドウを開閉します。数字キーに登録できます。")); }
		if (Id == CTCodeKeys::Phone) { return Loc3(TEXT("전화부스를 써요. 유령일 때는 다른 자리로 한 번 옮겨요."), TEXT("Use the phone booth. As a ghost, move to another spot once."), TEXT("電話ボックスを使います。ゴーストのときは一度だけ別の場所へ移動します。")); }
		if (Id == CTCodeKeys::TeamVoice) { return Loc3(TEXT("보물찾기에서 누르고 있는 동안 팀원에게만, 아무리 멀어도 들려요."), TEXT("In Treasure Hunt, hold to talk to your team only, at any distance."), TEXT("トレジャーハントで押している間、チームメイトにだけどんなに遠くても聞こえます。")); }
		if (Id == CTCodeKeys::GrabAlt) { return Loc3(TEXT("잡기 보조 키예요. 꼬리 대결 중에는 연타해요."), TEXT("Second key for Grab. Mash it during a tail struggle."), TEXT("つかむの補助キーです。しっぽの勝負中は連打します。")); }
		return FText::GetEmpty();
	}

	FText CodeKeyDisplayName(FName Id)
	{
		if (Id == CTCodeKeys::EmoteMenu) { return Loc3(TEXT("이모트 창"), TEXT("Emote Menu"), TEXT("エモート")); }
		if (Id == CTCodeKeys::Phone) { return Loc3(TEXT("전화 / 유령 이동"), TEXT("Phone / Ghost Move"), TEXT("電話 / ゴースト移動")); }
		if (Id == CTCodeKeys::TeamVoice) { return Loc3(TEXT("팀 음성"), TEXT("Team Voice"), TEXT("チームボイス")); }
		if (Id == CTCodeKeys::GrabAlt) { return Loc3(TEXT("잡기 (보조)"), TEXT("Grab (Alt)"), TEXT("つかむ (補助)")); }
		return FText::GetEmpty();
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

	// 테스트용 볼륨 옵션 (저장 안 함)
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
		if (Data.VoiceScaleVersion < 2)
		{
			// 척도가 바뀌었으므로 기본값(예전 100%와 같은 크기)으로 되돌림
			Data.VoiceVolume = 1.f / CTVoiceMaxGain;
			Data.VoiceScaleVersion = 2;
		}
		Data.VoiceVolume = FMath::Clamp(Data.VoiceVolume, 0.f, 1.f);
		Data.MicSensitivity = FMath::Clamp(Data.MicSensitivity, 0.f, 1.f);
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

void UCTSettingsSubsystem::SaveEmoteLoadout(const TArray<int32>& Loadout)
{
	Data.EmoteLoadout = Loadout;
	SaveSettings();
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

void UCTSettingsSubsystem::SetMicEnabled(bool bEnabled)
{
	Data.bMicEnabled = bEnabled;
	CommitChange();
}

void UCTSettingsSubsystem::SetMicAlwaysOn(bool bEnabled)
{
	Data.bMicAlwaysOn = bEnabled;
	CommitChange();
}

void UCTSettingsSubsystem::SetVoiceVolume(float Volume)
{
	Data.VoiceVolume = FMath::Clamp(Volume, 0.f, 1.f);
	ApplyAudio();
	CommitChange();
}

void UCTSettingsSubsystem::SetMicSensitivity(float Sensitivity)
{
	Data.MicSensitivity = FMath::Clamp(Sensitivity, 0.f, 1.f);
	ApplyAudio();
	CommitChange();
}

void UCTSettingsSubsystem::ApplyAudio()
{
	UGameInstance* GameInstance = GetGameInstance();

	// 마이크 감도: 이 값보다 작은 소리는 보내지 않음 (감도가 클수록 기준이 낮음. 0.9일 때 0.01)
	UVOIPStatics::SetMicThreshold((1.f - Data.MicSensitivity) * 0.1f);

	// 마이크 입력 증폭 (보내는 소리 자체를 키움). 기본 1.0이라 작게 들린다는 의견이 있어 올림
	if (IConsoleVariable* MicGain = IConsoleManager::Get().FindConsoleVariable(TEXT("voice.MicInputGain")))
	{
		MicGain->Set(CTMicInputGain, ECVF_SetByGameSetting);
	}

	// 배경음은 실행 중 곡에 직접 볼륨 (음소거 포함)
	if (UCTAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UCTAudioSubsystem>() : nullptr)
	{
		Audio->RefreshMusicVolume();
	}

	// 효과음은 사운드 믹스로
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

	// 다른 사람 목소리 (음소거 포함)
	const float VoiceVolume = Data.bMuteAll ? 0.f : Data.VoiceVolume * CTVoiceMaxGain;
	if (USoundClass* VoiceClass = Config()->VoiceSoundClass.LoadSynchronous())
	{
		UGameplayStatics::SetSoundMixClassOverride(World, Mix, VoiceClass, VoiceVolume, 1.f, 0.f, true);
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
	// -CTLanguage 로 언어 강제 (테스트용)
	FString Forced;
	if (FParse::Value(FCommandLine::Get(), TEXT("CTLanguage="), Forced) && IsSupportedLanguage(Forced)) { return Forced; }

	if (IsSupportedLanguage(Data.Language)) { return Data.Language; }

	// 저장값 없으면 영어 (시스템 언어 무시)
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
	// 에디터는 게임 문구만 변경
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
	// 키 포함 초기화. 언어는 유지
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

FKey UCTSettingsSubsystem::GetCodeKey(FName CodeKeyId) const
{
	if (const FKey* Overridden = Data.KeyOverrides.Find(CodeKeyId))
	{
		return *Overridden;
	}
	return DefaultCodeKey(CodeKeyId);
}

void UCTSettingsSubsystem::ResetKeyBindings()
{
	Data.KeyOverrides.Empty();
	EffectiveContexts.Empty();
	ApplyKeyBindingsToLocalPlayer();
	CommitChange();
}

FKey UCTSettingsSubsystem::GetDefaultKey(FName ActionId) const
{
	if (IsCodeKeyId(ActionId)) { return DefaultCodeKey(ActionId); }

	const FCTControlEntry* Entry = FindEntry(ActionId);
	if (!Entry) { return EKeys::Invalid; }

	return FirstKeyboardMouseKey(Entry->Context.LoadSynchronous(), Entry->Action.LoadSynchronous());
}

FKey UCTSettingsSubsystem::GetCurrentKey(FName ActionId) const
{
	if (IsCodeKeyId(ActionId)) { return GetCodeKey(ActionId); }

	if (const FKey* Overridden = Data.KeyOverrides.Find(ActionId))
	{
		return *Overridden;
	}
	return GetDefaultKey(ActionId);
}

bool UCTSettingsSubsystem::IsKeyUsedByOther(FName ActionId, FKey Key) const
{
	// 숫자 키 1~9는 이모트 전용
	if (IsEmoteNumberKey(Key)) { return true; }

	// 코드에서 받는 키(이모트 창, 전화, 잡기 보조)와도 겹치면 안 됨
	for (const FName CodeId : { CTCodeKeys::EmoteMenu, CTCodeKeys::Phone, CTCodeKeys::GrabAlt, CTCodeKeys::TeamVoice })
	{
		if (CodeId != ActionId && GetCodeKey(CodeId) == Key) { return true; }
	}

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
		Row.Description = DescriptionFor(Entry.Id);

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

	// 코드에서 받는 키: 이모트 창, 전화, 잡기 보조 (바꿀 수 있음)
	for (const FName CodeId : { CTCodeKeys::EmoteMenu, CTCodeKeys::Phone, CTCodeKeys::GrabAlt, CTCodeKeys::TeamVoice })
	{
		FCTControlRow Row;
		Row.Id = CodeId;
		Row.DisplayName = CodeKeyDisplayName(CodeId);
		Row.Description = DescriptionFor(CodeId);
		Row.bRebindable = true;
		Row.Keys.Add(GetCodeKey(CodeId));
		Rows.Add(Row);
	}

	// 안내만 하는 줄 (바꿀 수 없음)
	{
		FCTControlRow EmoteKeys;
		EmoteKeys.Id = TEXT("EmoteKeys");
		EmoteKeys.DisplayName = Loc3(TEXT("이모트 사용"), TEXT("Use Emote"), TEXT("エモート使用"));
		EmoteKeys.Description = Loc3(TEXT("등록해 둔 이모트를 바로 써요."), TEXT("Play the emote you assigned to that number."), TEXT("登録したエモートをすぐ使います。"));
		EmoteKeys.KeysLabel = FText::FromString(TEXT("1 ~ 9"));
		Rows.Add(EmoteKeys);

		FCTControlRow MashClick;
		MashClick.Id = TEXT("MashClick");
		MashClick.DisplayName = Loc3(TEXT("대결 연타 (마우스)"), TEXT("Struggle Mash (Mouse)"), TEXT("勝負の連打 (マウス)"));
		MashClick.Description = Loc3(TEXT("꼬리 대결 중에는 마우스 왼쪽 클릭으로도 연타할 수 있어요."), TEXT("During a tail struggle, left-click also counts as a mash."), TEXT("しっぽの勝負中は左クリックでも連打できます。"));
		MashClick.Keys.Add(EKeys::LeftMouseButton);
		Rows.Add(MashClick);
	}

	return Rows;
}

bool UCTSettingsSubsystem::SetKeyBinding(FName ActionId, FKey NewKey, FText& OutError)
{
	const FCTControlEntry* Entry = FindEntry(ActionId);
	if (!IsCodeKeyId(ActionId) && (!Entry || !Entry->bRebindable))
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

	if (!IsCodeKeyId(ActionId)) { ApplyKeyBindingsToLocalPlayer(); } // 코드 키는 매핑 재적용 불필요 (매 프레임 읽음)
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
