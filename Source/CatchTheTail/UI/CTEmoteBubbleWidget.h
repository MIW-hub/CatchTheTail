// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CTUserWidget.h"
#include "CTEmoteBubbleWidget.generated.h"

class UImage;
class UTexture2D;

/**
 * 이모트 말풍선 (머리 위에 뜨는 그림). 말풍선 그림 안에 이모지 그림을 채워 넣음.
 * 코드로 만들어서 따로 WBP가 필요 없음. 말풍선 그림은 /Game/2D/UI/Emote/UI_Emote_Bubble.
 */
UCLASS()
class CATCHTHETAIL_API UCTEmoteBubbleWidget : public UCTUserWidget
{
	GENERATED_BODY()

public:
	UCTEmoteBubbleWidget(const FObjectInitializer& ObjectInitializer);

	// 말풍선 안에 채울 그림 (nullptr 이면 비움). UVMin~UVMax(0~1)로 그림의 일부만 잘라 쓸 수 있음 (아틀라스)
	UFUNCTION(BlueprintCallable, Category = "CT|Emote")
	void SetEmoteIcon(UTexture2D* Icon, FVector2D UVMin = FVector2D(0.0, 0.0), FVector2D UVMax = FVector2D(1.0, 1.0));

	// 전체 투명도 0~1
	UFUNCTION(BlueprintCallable, Category = "CT|Emote")
	void SetBubbleOpacity(float Opacity);

	// 위젯을 그릴 크기 (말풍선 그림 비율에 맞춤)
	static FVector2D GetDrawSize() { return FVector2D(256.f, 248.f); }

protected:
	virtual void NativeOnInitialized() override;

	// 말풍선 그림. 생성자에서 직접 참조해야 패키징에 반드시 들어감
	UPROPERTY()
	TObjectPtr<UTexture2D> BubbleTexture;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image_Bubble;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image_EmoteIcon;
};
