// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "CTDirectionArrow.generated.h"

class SCTDirectionArrow;

/**
 * 방향 화살표. 선으로 그려서 별도 이미지 없이 쓸 수 있고, 각도만 정해 주면 그 방향으로 돌아감.
 * 전화부스가 알려 준 목표 방향을 HUD에 보여 줄 때 씀.
 */
UCLASS()
class CATCHTHETAIL_API UCTDirectionArrow : public UWidget
{
	GENERATED_BODY()

public:
	// 화살표 각도. 위쪽이 0도이고 시계 방향으로 커짐
	void SetAngleDegrees(float NewAngleDegrees);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor ArrowColor = FLinearColor(1.f, 0.85f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CT|UI", meta = (ClampMin = "1"))
	float LineThickness = 9.f;

private:
	float AngleDegrees = 0.f;

	TSharedPtr<SCTDirectionArrow> MyArrow;
};
