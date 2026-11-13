// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTMicIndicatorWidget.generated.h"

class UImage;
class UTexture2D;
class USizeBox;
class UTextBlock;

/**
 * 마이크 상태 표시 패널 (패널 + 아이콘 + 파장 막대 + 키 글자). HUD의 마이크 표시와 같은 모양을 코드로 만들어 로비 등에서 씀.
 * 평소 흰 패널, 말하는 중 마젠타 패널과 움직이는 흰 파장, 마이크를 끄면 회색.
 */
UCLASS()
class CATCHTHETAIL_API UCTMicIndicatorWidget : public UCTUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UImage> Image_MicPanel;

	// 패널 이미지: 평소와 꺼짐은 흰 상자, 말하는 중은 분홍 상자
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PanelWhite;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PanelPink;

	int32 PanelState = -1;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image_MicIcon;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> WaveBoxes[3];

	UPROPERTY(Transient)
	TObjectPtr<UImage> WaveImages[3];

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text_MicKey;

	FString LastKeyName;
	float WaveTime = 0.f;
	float WaveBlend = 0.f;
};
