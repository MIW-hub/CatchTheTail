// Copyright Moon Party Games. All Rights Reserved.
#include "CTAnimTools.h"

#if WITH_EDITOR
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Factories/AnimMontageFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/Package.h"
#endif

#if WITH_EDITOR
namespace
{
	UEdGraph* FindAnimGraph(UAnimBlueprint* AnimBlueprint)
	{
		if (!AnimBlueprint) { return nullptr; }
		for (UEdGraph* Graph : AnimBlueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetFName() == TEXT("AnimGraph")) { return Graph; }
		}
		return nullptr;
	}

	// 이름이 같은 핀을 방향까지 맞춰 찾음 (없으면 nullptr)
	UEdGraphPin* PinOf(UEdGraphNode* Node, const TCHAR* Name, EEdGraphPinDirection Direction)
	{
		if (!Node) { return nullptr; }
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == Direction && Pin->PinName.ToString().Equals(Name, ESearchCase::IgnoreCase)) { return Pin; }
		}
		return nullptr;
	}
}
#endif

FString UCTAnimTools::DescribeAnimGraph(UAnimBlueprint* AnimBlueprint)
{
#if WITH_EDITOR
	FString Out;
	UEdGraph* Graph = FindAnimGraph(AnimBlueprint);
	if (!Graph) { return TEXT("AnimGraph 를 찾지 못함"); }

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node) { continue; }
		Out += FString::Printf(TEXT("노드 %s [%s]\n"), *Node->GetName(), *Node->GetClass()->GetName());
		if (const UAnimGraphNode_Slot* Slot = Cast<UAnimGraphNode_Slot>(Node)) { Out += FString::Printf(TEXT("  슬롯 이름: %s\n"), *Slot->Node.SlotName.ToString()); }
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->bHidden) { continue; }
			FString Links;
			for (UEdGraphPin* Linked : Pin->LinkedTo) { Links += FString::Printf(TEXT("%s.%s "), *Linked->GetOwningNode()->GetName(), *Linked->PinName.ToString()); }
			Out += FString::Printf(TEXT("  핀 %s %s -> %s\n"), Pin->Direction == EGPD_Input ? TEXT("입력") : TEXT("출력"), *Pin->PinName.ToString(), *Links);
		}
	}
	if (AnimBlueprint->TargetSkeleton)
	{
		const FReferenceSkeleton& Ref = AnimBlueprint->TargetSkeleton->GetReferenceSkeleton();
		Out += TEXT("뼈: ");
		for (int32 Index = 0; Index < Ref.GetNum(); ++Index) { Out += Ref.GetBoneName(Index).ToString() + TEXT(", "); }
		Out += TEXT("\n");
	}
	return Out;
#else
	return FString();
#endif
}

bool UCTAnimTools::AddUpperBodyLayer(UAnimBlueprint* AnimBlueprint, FName SlotName, FName BoneName, int32 BlendDepth)
{
#if WITH_EDITOR
	UEdGraph* Graph = FindAnimGraph(AnimBlueprint);
	if (!Graph) { return false; }

	UAnimGraphNode_Slot* DefaultSlotNode = nullptr;
	UAnimGraphNode_Slot* ExistingUpper = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UAnimGraphNode_Slot* Slot = Cast<UAnimGraphNode_Slot>(Node))
		{
			if (Slot->Node.SlotName == SlotName) { ExistingUpper = Slot; }
			else if (Slot->Node.SlotName == FName(TEXT("DefaultSlot"))) { DefaultSlotNode = Slot; }
		}
	}
	if (!DefaultSlotNode) { return false; }

	// 이미 완성돼 있으면(섞기 노드의 기본 포즈까지 연결) 건너뜀. 중간까지만 된 예전 시도는 걷기 포즈 연결을 되찾고 지운 뒤 다시 만듦
	UEdGraphPin* DefaultSource = PinOf(DefaultSlotNode, TEXT("Source"), EGPD_Input);
	if (!DefaultSource) { return false; }
	UEdGraphPin* LocomotionOut = nullptr;
	TArray<UEdGraphNode*> Stale;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UAnimGraphNode_LayeredBoneBlend* Existing = Cast<UAnimGraphNode_LayeredBoneBlend>(Node))
		{
			const UEdGraphPin* BasePin = PinOf(Existing, TEXT("BasePose"), EGPD_Input);
			if (BasePin && !BasePin->LinkedTo.IsEmpty()) { return true; } // 이미 완성
			Stale.Add(Existing);
		}
	}
	if (ExistingUpper)
	{
		if (UEdGraphPin* OldSource = PinOf(ExistingUpper, TEXT("Source"), EGPD_Input)) { if (!OldSource->LinkedTo.IsEmpty()) { LocomotionOut = OldSource->LinkedTo[0]; } }
		Stale.Add(ExistingUpper);
	}
	if (!LocomotionOut && !DefaultSource->LinkedTo.IsEmpty() && Stale.IsEmpty()) { LocomotionOut = DefaultSource->LinkedTo[0]; }
	if (!LocomotionOut) { return false; }
	for (UEdGraphNode* StaleNode : Stale) { StaleNode->BreakAllNodeLinks(); Graph->RemoveNode(StaleNode); }

	const UEdGraphSchema* Schema = Graph->GetSchema();
	Graph->Modify();
	const int32 BaseX = DefaultSlotNode->NodePosX;
	const int32 BaseY = DefaultSlotNode->NodePosY;

	// 걷기 포즈는 한 곳에서 두 곳으로 못 나가므로 저장 노드에 담고 두 번 불러 씀
	FGraphNodeCreator<UAnimGraphNode_SaveCachedPose> SaveCreator(*Graph);
	UAnimGraphNode_SaveCachedPose* Save = SaveCreator.CreateNode();
	Save->CacheName = TEXT("UpperBodyLocomotion");
	Save->NodePosX = BaseX - 1100;
	Save->NodePosY = BaseY + 120;
	SaveCreator.Finalize();

	auto MakeUse = [&](int32 OffsetY) -> UAnimGraphNode_UseCachedPose*
	{
		FGraphNodeCreator<UAnimGraphNode_UseCachedPose> UseCreator(*Graph);
		UAnimGraphNode_UseCachedPose* Use = UseCreator.CreateNode();
		Use->SaveCachedPoseNode = Save;
		Use->NodePosX = BaseX - 800;
		Use->NodePosY = BaseY + OffsetY;
		UseCreator.Finalize();
		return Use;
	};
	UAnimGraphNode_UseCachedPose* UseForBase = MakeUse(0);
	UAnimGraphNode_UseCachedPose* UseForSlot = MakeUse(260);

	FGraphNodeCreator<UAnimGraphNode_Slot> SlotCreator(*Graph);
	UAnimGraphNode_Slot* UpperSlot = SlotCreator.CreateNode();
	UpperSlot->Node.SlotName = SlotName;
	UpperSlot->NodePosX = BaseX - 520;
	UpperSlot->NodePosY = BaseY + 260;
	SlotCreator.Finalize();

	FGraphNodeCreator<UAnimGraphNode_LayeredBoneBlend> BlendCreator(*Graph);
	UAnimGraphNode_LayeredBoneBlend* Blend = BlendCreator.CreateNode();
	if (Blend->Node.LayerSetup.Num() == 0) { Blend->Node.AddPose(); }
	FBranchFilter Filter;
	Filter.BoneName = BoneName;
	Filter.BlendDepth = BlendDepth;
	Blend->Node.LayerSetup[0].BranchFilters.Add(Filter);
	Blend->NodePosX = BaseX - 300;
	Blend->NodePosY = BaseY;
	BlendCreator.Finalize();

	UEdGraphPin* SaveIn = PinOf(Save, TEXT("Pose"), EGPD_Input);
	UEdGraphPin* UseBaseOut = PinOf(UseForBase, TEXT("Pose"), EGPD_Output);
	UEdGraphPin* UseSlotOut = PinOf(UseForSlot, TEXT("Pose"), EGPD_Output);
	UEdGraphPin* BlendBase = PinOf(Blend, TEXT("BasePose"), EGPD_Input);
	UEdGraphPin* BlendPose0 = PinOf(Blend, TEXT("BlendPoses_0"), EGPD_Input);
	UEdGraphPin* BlendOut = PinOf(Blend, TEXT("Pose"), EGPD_Output);
	UEdGraphPin* SlotSource = PinOf(UpperSlot, TEXT("Source"), EGPD_Input);
	UEdGraphPin* SlotOut = PinOf(UpperSlot, TEXT("Pose"), EGPD_Output);
	if (!SaveIn || !UseBaseOut || !UseSlotOut || !BlendBase || !BlendPose0 || !BlendOut || !SlotSource || !SlotOut) { return false; }

	DefaultSource->BreakAllPinLinks();
	LocomotionOut->BreakAllPinLinks();
	Schema->TryCreateConnection(LocomotionOut, SaveIn);
	Schema->TryCreateConnection(UseBaseOut, BlendBase);
	Schema->TryCreateConnection(UseSlotOut, SlotSource);
	Schema->TryCreateConnection(SlotOut, BlendPose0);
	Schema->TryCreateConnection(BlendOut, DefaultSource);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
	return true;
#else
	return false;
#endif
}

FString UCTAnimTools::CreateHoldThrowMontages(UAnimSequence* GrabSequence, FName SlotName, FName SlotGroupName)
{
#if WITH_EDITOR
	if (!GrabSequence) { return TEXT("잡기 애니메이션이 없음"); }
	USkeleton* Skeleton = GrabSequence->GetSkeleton();
	if (!Skeleton) { return TEXT("스켈레톤이 없음"); }

	// 상체 슬롯을 DefaultGroup 과 다른 슬롯 그룹에 등록 (같은 그룹이면 달리기/잡기 몽타주와 서로 끊음)
	Skeleton->Modify();
	Skeleton->AddSlotGroupName(SlotGroupName);
	Skeleton->SetSlotGroupName(SlotName, SlotGroupName);

	FString Report;
	auto MakeMontage = [&](const TCHAR* AssetName, float PlayRate, bool bLoopTail) -> UAnimMontage*
	{
		const FString PackagePath = FString::Printf(TEXT("/Game/Character/Animation/Blender/%s"), AssetName);
		UPackage* Package = CreatePackage(*PackagePath);
		UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();
		Factory->TargetSkeleton = Skeleton;
		Factory->SourceAnimation = GrabSequence;
		UAnimMontage* Montage = Cast<UAnimMontage>(Factory->FactoryCreateNew(UAnimMontage::StaticClass(), Package, FName(AssetName), RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn));
		if (!Montage) { Report += FString::Printf(TEXT("%s 만들기 실패\n"), AssetName); return nullptr; }

		if (Montage->SlotAnimTracks.Num() > 0)
		{
			Montage->SlotAnimTracks[0].SlotName = SlotName;
			if (Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num() > 0) { Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimPlayRate = PlayRate; }
		}
		Montage->SetCompositeLength(Montage->CalculateSequenceLength());
		Montage->BlendIn.SetBlendTime(0.15f);
		Montage->BlendOut.SetBlendTime(bLoopTail ? 0.2f : 0.15f);

		if (bLoopTail)
		{
			// 들어 올리는 동작은 한 번 보이고, 끝 자세 구간("Loop")이 계속 반복됨
			const float LoopStart = Montage->GetPlayLength() * 0.72f;
			const int32 LoopIndex = Montage->AddAnimCompositeSection(FName(TEXT("Loop")), LoopStart);
			if (Montage->CompositeSections.IsValidIndex(LoopIndex)) { Montage->CompositeSections[LoopIndex].NextSectionName = FName(TEXT("Loop")); }
		}
		Montage->MarkPackageDirty();
		Report += FString::Printf(TEXT("%s 만듦 (길이 %.2f초)\n"), AssetName, Montage->GetPlayLength());
		return Montage;
	};

	MakeMontage(TEXT("AM_Hold_Object_Montage"), 1.f, true);
	MakeMontage(TEXT("AM_Throw_Object_Montage"), 2.2f, false);
	return Report;
#else
	return FString();
#endif
}
