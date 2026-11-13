// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "CTCircularProgress.generated.h"

// 원형 진행 표시 (위에서 시계 방향으로 차오름). 팀 변경 요청 패널의 5초 표시에 씀
UCLASS()
class CATCHTHETAIL_API UCTCircularProgress : public UWidget
{
	GENERATED_BODY()

public:
	UCTCircularProgress();

	// 0 ~ 1
	UFUNCTION(BlueprintCallable, Category = "CT|UI")
	void SetPercent(float NewPercent);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|UI", meta = (ClampMin = "0", ClampMax = "1"))
	float Percent = 1.f;

	// 차오르는 색
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|UI")
	FLinearColor FillColor = FLinearColor(1.f, 0.153f, 0.514f, 1.f);

	// 배경 고리 색
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|UI")
	FLinearColor TrackColor = FLinearColor(1.f, 1.f, 1.f, 0.55f);

	// 고리 두께 (화면 픽셀)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|UI", meta = (ClampMin = "1"))
	float Thickness = 8.f;

private:
	TSharedPtr<class SCTCircularProgress> SlateProgress;
};
