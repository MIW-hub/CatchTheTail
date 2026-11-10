// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "CTEmoteBubbleComponent.generated.h"

class UTexture2D;

/**
 * 머리 위 이모트 말풍선. 2D 그림이지만 3D 월드에 떠 있고 항상 카메라를 향함.
 * 흐름: 머리에서 천천히 올라옴(점점 빨라짐) -> 멈춰서 유지 -> 점점 빨리 사라짐.
 * 모든 시간과 거리는 BP에서 바꿀 수 있음.
 */
UCLASS(ClassGroup = (CT), meta = (BlueprintSpawnableComponent))
class CATCHTHETAIL_API UCTEmoteBubbleComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UCTEmoteBubbleComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 말풍선을 띄우고 Icon 을 안에 채움. 이미 떠 있으면 처음부터 다시
	UFUNCTION(BlueprintCallable, Category = "CT|Emote")
	void ShowEmote(UTexture2D* Icon, FVector2D UVMin = FVector2D(0.0, 0.0), FVector2D UVMax = FVector2D(1.0, 1.0));

	// 바로 숨김
	UFUNCTION(BlueprintCallable, Category = "CT|Emote")
	void HideEmoteNow();

	UFUNCTION(BlueprintPure, Category = "CT|Emote")
	bool IsShowingEmote() const { return Phase != EPhase::Idle; }

	// 올라오는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "0.05"))
	float RiseSeconds = 2.f;

	// 멈춰서 유지하는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "0"))
	float HoldSeconds = 2.f;

	// 사라지는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "0.05"))
	float FadeSeconds = 2.f;

	// 머리 위 시작 위치 (캐릭터 기준 cm, 말풍선 아래 끝이 이 위치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote")
	FVector StartOffset = FVector(0.f, 0.f, 60.f);

	// 화면 기준 오른쪽으로 옮기는 거리(cm). 말풍선이 머리 가운데가 아니라 머리 오른쪽에 뜸. 음수면 왼쪽
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote")
	float ScreenOffsetRight = 75.f;

	// 올라오는 높이(cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "0"))
	float RiseDistance = 50.f;

	// 말풍선 가로 크기(cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "5"))
	float WorldWidth = 45.f;

	// 천천히 -> 빠르게 정도. 1이면 일정한 속도, 클수록 처음이 느리고 끝이 빠름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "1"))
	float EaseExponent = 2.f;

	// 나타날 때 커지는 시간(초). 0이면 크기 변화 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CT|Emote", meta = (ClampMin = "0"))
	float PopSeconds = 0.25f;

private:
	enum class EPhase : uint8 { Idle, Rise, Hold, Fade };

	void ApplyState(float Alpha, float Opacity, float Scale);
	void FaceCamera();

	EPhase Phase = EPhase::Idle;
	float PhaseTime = 0.f;
	float ElapsedTotal = 0.f;
};
