// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CTLobbyBoundary.generated.h"

class UBoxComponent;

// 로비 가장자리에 보이지 않는 벽을 세워 점프로 밖에 나가지 못하게 함.
// 로비 게임모드가 서버에서 만들고(복제), 서버와 클라이언트가 같은 계산으로 각자 벽을 세움 (클라이언트 예측 이동이 끌려가지 않게).
// 범위는 플레이어 시작 지점들을 감싸는 상자에 여유를 더해 정함.
UCLASS()
class CATCHTHETAIL_API ACTLobbyBoundary : public AActor
{
	GENERATED_BODY()

public:
	ACTLobbyBoundary();

protected:
	virtual void BeginPlay() override;

	// 시작 지점 상자에서 바깥으로 더하는 여유(cm)
	UPROPERTY(EditDefaultsOnly, Category = "CT|Lobby")
	float Margin = 600.f;

	// 벽 두께(cm)
	UPROPERTY(EditDefaultsOnly, Category = "CT|Lobby")
	float WallThickness = 200.f;

	// 시작 지점 위로 올라가는 벽 높이(cm), 점프로 넘지 못할 만큼
	UPROPERTY(EditDefaultsOnly, Category = "CT|Lobby")
	float WallHeight = 3000.f;

private:
	UBoxComponent* AddWall(const FName& Name, const FVector& Center, const FVector& HalfExtent);
};
