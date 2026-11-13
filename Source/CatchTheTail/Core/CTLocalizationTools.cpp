// Copyright Moon Party Games. All Rights Reserved.
#include "CTLocalizationTools.h"
#include "CTMatchSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/Blueprint.h"
#include "Engine/Font.h"
#include "Fonts/CompositeFont.h"
#include "Internationalization/Text.h"
#include "Internationalization/TextKey.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionComment.h"

namespace
{
	// 번역 키 없는 고정 글자인가
	bool NeedsKey(const FText& Text)
	{
		return !Text.IsEmpty() && (Text.IsCultureInvariant() || FTextInspector::GetTextId(Text).GetKey().IsEmpty());
	}
}

int32 UCTLocalizationTools::MakeWidgetTextsLocalizable(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return 0; }

	// 리플렉션으로 WidgetTree 접근
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

int32 UCTLocalizationTools::RefreshMatchSettingTexts(UBlueprint* WidgetBlueprint)
{
#if WITH_EDITOR
	if (!WidgetBlueprint) { return 0; }

	const FObjectProperty* TreeProperty = CastField<FObjectProperty>(WidgetBlueprint->GetClass()->FindPropertyByName(TEXT("WidgetTree")));
	UWidgetTree* Tree = TreeProperty ? Cast<UWidgetTree>(TreeProperty->GetObjectPropertyValue_InContainer(WidgetBlueprint)) : nullptr;
	if (!Tree) { return 0; }

	int32 Changed = 0;
	for (int32 Index = 0; Index < static_cast<int32>(ECTMatchSetting::Count); ++Index)
	{
		const ECTMatchSetting Setting = static_cast<ECTMatchSetting>(Index);
		const FString Key = FCTMatchSettings::GetKey(Setting).ToString();

		if (UTextBlock* Label = Cast<UTextBlock>(Tree->FindWidget(*FString::Printf(TEXT("Label_Setting_%s"), *Key))))
		{
			Label->SetText(FCTMatchSettings::GetLabel(Setting));
			++Changed;
		}
		if (UTextBlock* Value = Cast<UTextBlock>(Tree->FindWidget(*FString::Printf(TEXT("Text_Setting_%s"), *Key))))
		{
			Value->SetText(FCTMatchSettings::FormatValue(Setting, FCTMatchSettings().Get(Setting)));
			++Changed;
		}
	}

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

void UCTLocalizationTools::SetMaterialCommentSize(UObject* CommentNode, int32 Width, int32 Height)
{
#if WITH_EDITORONLY_DATA
	UMaterialExpressionComment* Box = Cast<UMaterialExpressionComment>(CommentNode);
	if (!Box) { return; }

	Box->SizeX = FMath::Max(Width, 50);
	Box->SizeY = FMath::Max(Height, 50);
	Box->MarkPackageDirty();
#endif
}

int32 UCTLocalizationTools::RemoveUnusedMaterialNodes(UMaterial* Material)
{
#if WITH_EDITORONLY_DATA
	if (!Material) { return -1; }

	// 출력에서 거슬러 올라가며 쓰이는 노드를 모음
	TSet<UMaterialExpression*> Used;
	TFunction<void(UMaterialExpression*)> Walk = [&Used, &Walk](UMaterialExpression* Expression)
	{
		if (!Expression || Used.Contains(Expression)) { return; }
		Used.Add(Expression);
		for (int32 Index = 0; ; ++Index)
		{
			FExpressionInput* Input = Expression->GetInput(Index);
			if (!Input) { break; }
			Walk(Input->Expression);
		}
	};
	for (int32 Property = 0; Property < MP_MAX; ++Property)
	{
		if (const FExpressionInput* Input = Material->GetExpressionInputForProperty(static_cast<EMaterialProperty>(Property)))
		{
			Walk(Input->Expression);
		}
	}

	TArray<UMaterialExpression*> Unused;
	for (UMaterialExpression* Expression : Material->GetExpressions())
	{
		if (Expression && !Used.Contains(Expression) && !Expression->IsA<UMaterialExpressionComment>())
		{
			Unused.Add(Expression);
		}
	}
	for (UMaterialExpression* Expression : Unused)
	{
		Material->GetExpressionCollection().RemoveExpression(Expression);
	}
	if (Unused.Num() > 0) { Material->MarkPackageDirty(); }
	return Unused.Num();
#else
	return -1;
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
