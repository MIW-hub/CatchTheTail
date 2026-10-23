// Copyright Moon Party Games. All Rights Reserved.
#include "CTLocalizationTools.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/Blueprint.h"
#include "Engine/Font.h"
#include "Fonts/CompositeFont.h"
#include "Internationalization/Text.h"
#include "Internationalization/TextKey.h"

namespace
{
	// 번역 키가 없거나 고정 글자(번역 대상 아님)인가
	bool NeedsKey(const FText& Text)
	{
		return !Text.IsEmpty() && (Text.IsCultureInvariant() || FTextInspector::GetTextId(Text).GetKey().IsEmpty());
	}
}

int32 UCTLocalizationTools::MakeWidgetTextsLocalizable(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return 0; }

	// UWidgetBlueprint는 에디터 모듈에 있어서, 리플렉션으로 WidgetTree를 찾음
	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	UWidgetTree* Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return 0; }

	static const FTextKey Namespace(TEXT("CT_UI"));
	const FString BlueprintName = WidgetBlueprint->GetName();
	int32 Changed = 0;

	Tree->ForEachWidget([&](UWidget* Widget)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			const FText Current = TextBlock->GetText();
			if (NeedsKey(Current))
			{
				const FString Key = FString::Printf(TEXT("%s.%s"), *BlueprintName, *TextBlock->GetName());
				TextBlock->SetText(FText::ChangeKey(Namespace, FTextKey(Key), FText::FromString(Current.ToString())));
				++Changed;
			}
		}
		else if (UEditableTextBox* TextBox = Cast<UEditableTextBox>(Widget))
		{
			const FText Hint = TextBox->GetHintText();
			if (NeedsKey(Hint))
			{
				const FString Key = FString::Printf(TEXT("%s.%s.Hint"), *BlueprintName, *TextBox->GetName());
				TextBox->SetHintText(FText::ChangeKey(Namespace, FTextKey(Key), FText::FromString(Hint.ToString())));
				++Changed;
			}
		}
	});

	if (Changed > 0)
	{
		WidgetBlueprint->Modify();
		WidgetBlueprint->MarkPackageDirty();
	}
	return Changed;
#else
	return 0;
#endif
}

bool UCTLocalizationTools::AddFallbackFontFace(UFont* TargetFont, UObject* FontFaceAsset, FName EntryName)
{
#if WITH_EDITOR
	if (!TargetFont || !FontFaceAsset) { return false; }

	TArray<FTypefaceEntry>& Fonts = TargetFont->GetMutableInternalCompositeFont().FallbackTypeface.Typeface.Fonts;
	for (const FTypefaceEntry& Existing : Fonts)
	{
		if (Existing.Font.GetFontFaceAsset() == FontFaceAsset) { return false; }
	}

	FTypefaceEntry Entry(EntryName);
	Entry.Font = FFontData(FontFaceAsset);
	Fonts.Add(Entry);

	TargetFont->Modify();
	TargetFont->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
