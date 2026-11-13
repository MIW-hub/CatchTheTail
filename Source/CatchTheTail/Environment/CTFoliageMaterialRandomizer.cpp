// Copyright Moon Party Games. All Rights Reserved.
#include "CTFoliageMaterialRandomizer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

ACTFoliageMaterialRandomizer::ACTFoliageMaterialRandomizer()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ACTFoliageMaterialRandomizer::BeginPlay()
{
	Super::BeginPlay();
	RandomizeFoliageMaterials();
}

void ACTFoliageMaterialRandomizer::RandomizeFoliageMaterials()
{
	UWorld* World = GetWorld();
	if (bApplied || !World || !TargetMesh)
	{
		return;
	}

	TArray<UMaterialInterface*> Materials;
	for (UMaterialInterface* Material : CandidateMaterials)
	{
		if (Material)
		{
			Materials.Add(Material);
		}
	}
	if (Materials.IsEmpty())
	{
		return;
	}

	// 대상 메시 폴리지 수집
	TArray<UInstancedStaticMeshComponent*> Sources;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (*It == this)
		{
			continue;
		}
		TInlineComponentArray<UInstancedStaticMeshComponent*> Components(*It);
		for (UInstancedStaticMeshComponent* Component : Components)
		{
			if (Component->GetStaticMesh() == TargetMesh && Component->GetInstanceCount() > 0)
			{
				Sources.Add(Component);
			}
		}
	}
	if (Sources.IsEmpty())
	{
		return;
	}
	bApplied = true;

	// 머티리얼별 컴포넌트 (설정은 첫 소스 기준)
	const UInstancedStaticMeshComponent* Template = Sources[0];
	TArray<UInstancedStaticMeshComponent*> Targets;
	for (UMaterialInterface* Material : Materials)
	{
		UInstancedStaticMeshComponent* Target = NewObject<UInstancedStaticMeshComponent>(this);
		Target->SetStaticMesh(TargetMesh);
		Target->SetCollisionProfileName(Template->GetCollisionProfileName());
		Target->SetCollisionEnabled(Template->GetCollisionEnabled());
		Target->SetCastShadow(Template->CastShadow);
		Target->SetCullDistances(Template->InstanceStartCullDistance, Template->InstanceEndCullDistance);

		if (MaterialSlotIndex >= 0)
		{
			Target->SetMaterial(MaterialSlotIndex, Material);
		}
		else
		{
			for (int32 Slot = 0; Slot < Target->GetNumMaterials(); ++Slot)
			{
				Target->SetMaterial(Slot, Material);
			}
		}

		Target->SetupAttachment(GetRootComponent());
		Target->RegisterComponent();
		Targets.Add(Target);
	}

	// 위치 시드 -> 서버/클라 동일 결과
	for (UInstancedStaticMeshComponent* Source : Sources)
	{
		const int32 Count = Source->GetInstanceCount();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FTransform InstanceTransform;
			if (!Source->GetInstanceTransform(Index, InstanceTransform, true))
			{
				continue;
			}

			FRandomStream Stream(GetTypeHash(FIntVector(InstanceTransform.GetLocation())));
			Targets[Stream.RandRange(0, Targets.Num() - 1)]->AddInstance(InstanceTransform, true);
		}
		Source->ClearInstances();
	}
}
