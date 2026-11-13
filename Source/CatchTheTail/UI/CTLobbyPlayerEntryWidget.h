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
	void SetEntry(const FString& PlayerName, ECTLobbyPlayerKind Kind, bool bIsMe);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Name;

	// 방장 / 준비 / 대기 / AI
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_State;

	// 상태 글자 뒤의 알약 모양 배경 (색이 상태마다 바뀜)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_StatePill;

	// 방장일 때만 보이는 왕관
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Host;

	// 내 줄일 때만 보이는 강조
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Me;

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
