// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTIntroWidget.generated.h"

class UTextBlock;

/** 인트로 연출이 끝났을 때 (자동 종료 또는 건너뛰기) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCTIntroFinishedSignature);

/**
 * 게임을 켠 직후 보여 주는 인트로 화면. (WBP_Intro의 부모. ACTMenuPlayerController가 메인 메뉴보다 먼저 띄웁니다)
 *
 * 동작
 *  - 위젯 전체가 서서히 나타남 → 잠시 유지 → 서서히 사라짐 → OnIntroFinished 호출 → 메인 메뉴로 이동
 *  - 아무 키나 마우스 버튼을 누르면 건너뜁니다. (bAllowSkip)
 *  - 로고, 배경, 로봇 연출은 WBP에서 자유롭게 꾸미세요. 시작 시 BP_OnIntroStarted를 호출하므로 애니메이션을 재생할 수 있습니다.
 *
 * WBP에 만들 수 있는 위젯 (이름을 똑같이, 선택 사항)
 *  - Text_Skip : "아무 키나 눌러 건너뛰기" 같은 안내 문구
 */
UCLASS(Abstract)
class CATCHTHETAIL_API UCTIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "CT|Intro")
	FCTIntroFinishedSignature OnIntroFinished;

	/** 인트로를 바로 끝냅니다. */
	UFUNCTION(BlueprintCallable, Category = "CT|Intro")
	void FinishIntro();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 인트로가 시작될 때 호출 (로고 애니메이션, 사운드 재생용) */
	UFUNCTION(BlueprintImplementableEvent, Category = "CT|Intro")
	void BP_OnIntroStarted();

	/** 서서히 나타나는 시간(초). 0이면 바로 보입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Intro", meta = (ClampMin = "0"))
	float FadeInSeconds = 0.8f;

	/** 완전히 보이는 상태로 유지하는 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Intro", meta = (ClampMin = "0"))
	float HoldSeconds = 2.0f;

	/** 서서히 사라지는 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Intro", meta = (ClampMin = "0"))
	float FadeOutSeconds = 0.6f;

	/** 키나 마우스 입력으로 건너뛸 수 있는지 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|Intro")
	bool bAllowSkip = true;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skip;

private:
	float ElapsedSeconds = 0.f;
	bool bFinished = false;
};
