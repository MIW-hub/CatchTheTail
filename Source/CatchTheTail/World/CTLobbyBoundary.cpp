// Copyright Moon Party Games. All Rights Reserved.
#include "CTLobbyBoundary.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

ACTLobbyBoundary::ACTLobbyBoundary()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

UBoxComponent* ACTLobbyBoundary::AddWall(const FName& Name, const FVector& Center, const FVector& HalfExtent)
{
	UBoxComponent* Wall = NewObject<UBoxComponent>(this, Name);
	Wall->SetBoxExtent(HalfExtent);
	Wall->SetWorldLocation(Center);
	Wall->SetCollisionProfileName(TEXT("BlockAll"));
	Wall->SetHiddenInGame(true);
	Wall->SetupAttachment(GetRootComponent());
	Wall->RegisterComponent();
	return Wall;
}

void ACTLobbyBoundary::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FBox Area(ForceInit);
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		Area += It->GetActorLocation();
	}
	if (!Area.IsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CT-Lobby] 플레이어 시작 지점이 없어 로비 벽을 만들지 않음"));
		return;
	}

	Area = Area.ExpandBy(FVector(Margin, Margin, 0.f));
	const FVector Center = Area.GetCenter();
	const FVector Extent = Area.GetExtent();
	const float T = WallThickness;
	const float H = WallHeight;
	const float BottomZ = Area.Min.Z - 200.f;
	const float WallCenterZ = BottomZ + H * 0.5f;
	const float WallHalfZ = H * 0.5f;

	AddWall(TEXT("WallXMin"), FVector(Area.Min.X - T, Center.Y, WallCenterZ), FVector(T, Extent.Y + 2 * T, WallHalfZ));
	AddWall(TEXT("WallXMax"), FVector(Area.Max.X + T, Center.Y, WallCenterZ), FVector(T, Extent.Y + 2 * T, WallHalfZ));
	AddWall(TEXT("WallYMin"), FVector(Center.X, Area.Min.Y - T, WallCenterZ), FVector(Extent.X + 2 * T, T, WallHalfZ));
	AddWall(TEXT("WallYMax"), FVector(Center.X, Area.Max.Y + T, WallCenterZ), FVector(Extent.X + 2 * T, T, WallHalfZ));

	UE_LOG(LogTemp, Log, TEXT("[CT-Lobby] 로비 벽 생성: 범위 %s ~ %s"), *Area.Min.ToString(), *Area.Max.ToString());
}
