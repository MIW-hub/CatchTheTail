// Copyright Moon Party Games. All Rights Reserved.
#include "CTMapEntryWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

void UCTMapEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Entry)
	{
		Button_Entry->OnClicked.AddUniqueDynamic(this, &UCTMapEntryWidget::HandleClicked);
	}
}

void UCTMapEntryWidget::SetEntry(int32 InMapIndex, const FCTMapInfo& Info, bool bSelected)
{
	MapIndex = InMapIndex;

	if (Text_MapName) { Text_MapName->SetText(Info.DisplayName); }
	if (Text_MapDescription) { Text_MapDescription->SetText(Info.Description); }

	if (Image_Thumbnail)
	{
		// 이미지가 없으면 칸을 숨김
		if (Info.Thumbnail.IsNull())
		{
			Image_Thumbnail->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_Thumbnail->SetVisibility(ESlateVisibility::HitTestInvisible);
			Image_Thumbnail->SetBrushFromSoftTexture(Info.Thumbnail, /*bMatchSize=*/false);
		}
	}

	SetSelected(bSelected);
}

void UCTMapEntryWidget::SetSelected(bool bSelected)
{
	if (Widget_Selected)
	{
		Widget_Selected->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	BP_OnSelectionChanged(bSelected);
}

void UCTMapEntryWidget::HandleClicked()
{
	OnEntryClicked.Broadcast(MapIndex);
}
