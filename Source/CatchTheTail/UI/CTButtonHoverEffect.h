// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Containers/Ticker.h"
#include "CTButtonHoverEffect.generated.h"

class UButton;
class UUserWidget;

/**
 * 버튼에 마우스를 올리면 살짝 커지면서 떠오르고, 누르면 눌리는 효과를 줍니다.
 * 위젯의 NativeConstruct 에서 Attach(this) 를 한 번 부르면 그 위젯 안의 모든 버튼에 적용됩니다.
 * 버튼의 모양(스타일)은 건드리지 않고 Render Transform 만 바꿉니다.
 * 효과를 빼고 싶은 버튼은 WBP 에서 이름에 "NoHover" 를 넣으세요. (예: Button_Close_NoHover)
 */
UCLASS()
class CATCHTHETAIL_API UCTButtonHoverEffect : public UObject
{
	GENERATED_BODY()

public:
	// Owner 안의 모든 버튼에 효과를 붙임. 여러 번 불러도 안전함
	static void Attach(UUserWidget* Owner);

	virtual void BeginDestroy() override;

private:
	struct FEntry
	{
		TWeakObjectPtr<UButton> Button;
		float Scale = 1.f;
		float Lift = 0.f;
	};

	void Collect(UUserWidget* Owner);
	bool Tick(float DeltaTime);

	TArray<FEntry> Entries;
	TWeakObjectPtr<UUserWidget> OwnerWidget;
	FTSTicker::FDelegateHandle TickHandle;

	// 올렸을 때 커지는 배율, 누를 때 배율, 떠오르는 높이(px), 변하는 속도
	static constexpr float HoverScale = 1.06f;
	static constexpr float PressedScale = 0.96f;
	static constexpr float HoverLift = 2.f;
	static constexpr float InterpSpeed = 18.f;
};
