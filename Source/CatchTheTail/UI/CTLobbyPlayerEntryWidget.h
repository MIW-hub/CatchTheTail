// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CTLobbyPlayerEntryWidget.generated.h"

class UImage;
class UTextBlock;

// 로비 플레이어 목록 한 줄의 상태
enum class ECTLobbyPlayerKind : uint8
{
	Host,
	Ready,
	Waiting,
	AI,
};

// 로비 "참가 플레이어" 목록의 한 줄. 모양은 WBP_CTLobbyPlayerEntryWidget 에서 정함
UCLASS(Abstract)
class CATCHTHETAIL_API UCTLobbyPlayerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// TeamColor 가 있으면(팀전) 줄 배경이 그 색으로 물들고, 방장 표시 왼쪽의 색 칩에도 그 색이 입혀짐. 없으면 원래 분홍 줄
	void SetEntry(const FString& PlayerName, ECTLobbyPlayerKind Kind, bool bIsMe, const TOptional<FLinearColor>& TeamColor = TOptional<FLinearColor>());

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Name;

	// 방장 / 준비 / 대기 / AI
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_State;

	// 상태 글자 뒤의 알약 모양 배경 (색이 상태마다 바뀜)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_StatePill;

	// 줄 배경 (팀전에서는 팀 색으로 물듦)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_PlayerBack;

	// 방장 표시 왼쪽의 팀 색 칩 (팀전에서만 보임)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_TeamColor;

	// 팀 색을 줄 배경에 입힐 때 흰색과 섞는 비율 (1 이면 팀 색 그대로, 작을수록 연함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI", meta = (ClampMin = "0", ClampMax = "1"))
	float TeamTintStrength = 0.5f;

	// 방장일 때만 보이는 왕관
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Host;

	// 내 줄일 때만 보이는 강조
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Me;

	virtual void NativeOnInitialized() override;

	FLinearColor DefaultBackColor = FLinearColor(1.f, 0.76f, 0.86f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor HostColor = FLinearColor(1.f, 0.72f, 0.05f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor ReadyColor = FLinearColor(0.15f, 0.7f, 0.3f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor WaitingColor = FLinearColor(0.5f, 0.5f, 0.55f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FLinearColor AIColor = FLinearColor(0.1f, 0.55f, 1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText HostText = NSLOCTEXT("CT", "Lobby_StateHost", "방장");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText ReadyText = NSLOCTEXT("CT", "Lobby_StateReady", "준비");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText WaitingText = NSLOCTEXT("CT", "Lobby_StateWaiting", "대기");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CT|UI")
	FText AIText = NSLOCTEXT("CT", "Lobby_StateAI", "AI");
};
