// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Containers/Ticker.h"
#include "CTButtonHoverEffect.generated.h"

class UButton;
class UUserWidget;

// 버튼 호버/눌림 효과
UCLASS()
class CATCHTHETAIL_API UCTButtonHoverEffect : public UObject
{
	GENERATED_BODY()

public:
	// 모든 버튼에 적용. 중복 호출 안전
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

	// 호버/눌림 배율, 떠오르는 높이, 속도
	static constexpr float HoverScale = 1.06f;
	static constexpr float PressedScale = 0.96f;
	static constexpr float HoverLift = 2.f;
	static constexpr float InterpSpeed = 18.f;
};
