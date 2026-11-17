// Copyright Moon Party Games. All Rights Reserved.
#include "CTRoundBreakRowWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCTRoundBreakRowWidget::SetEntry(const FString& Name, int32 Score, const FLinearColor& TeamColor, bool bIsMe)
{
	if (Text_Name) { Text_Name->SetText(FText::FromString(Name)); }
	if (Text_Score) { Text_Score->SetText(FText::AsNumber(Score)); }
	if (Image_Color) { Image_Color->SetColorAndOpacity(TeamColor); }
	BP_OnSetEntry(bIsMe);
}
