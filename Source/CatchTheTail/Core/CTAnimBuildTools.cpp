// Copyright Moon Party Games. All Rights Reserved.
#include "CTAnimBuildTools.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"

float UCTAnimBuildTools::BuildMontageSegments(UAnimMontage* Montage, const TArray<UAnimSequence*>& Anims, const TArray<float>& StartTimes, const TArray<float>& EndTimes, const TArray<float>& PlayRates, const TArray<FName>& SectionNames, FName LoopSection, float BlendInTime, float BlendOutTime)
{
#if WITH_EDITOR
	const int32 Count = Anims.Num();
	if (!Montage || Count == 0 || StartTimes.Num() != Count || EndTimes.Num() != Count || PlayRates.Num() != Count || SectionNames.Num() != Count)
	{
		return -1.f;
	}

	Montage->Modify();

	if (Montage->SlotAnimTracks.Num() == 0)
	{
		Montage->AddSlot(TEXT("DefaultGroup.DefaultSlot"));
	}

	FAnimTrack& Track = Montage->SlotAnimTracks[0].AnimTrack;
	Track.AnimSegments.Reset();
	Montage->CompositeSections.Reset();

	float Cursor = 0.f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (!Anims[Index] || FMath::IsNearlyZero(PlayRates[Index]) || EndTimes[Index] <= StartTimes[Index])
		{
			return -1.f;
		}

		FAnimSegment Segment;
		Segment.SetAnimReference(Anims[Index], true);
		Segment.StartPos = Cursor;
		Segment.AnimStartTime = StartTimes[Index];
		Segment.AnimEndTime = EndTimes[Index];
		Segment.AnimPlayRate = PlayRates[Index];
		Segment.LoopingCount = 1;
		Track.AnimSegments.Add(Segment);

		Montage->AddAnimCompositeSection(SectionNames[Index], Cursor);
		Cursor += (EndTimes[Index] - StartTimes[Index]) / FMath::Abs(PlayRates[Index]);
	}

	// 반복 섹션만 자기 자신으로 연결
	for (int32 Index = 0; Index < Montage->CompositeSections.Num(); ++Index)
	{
		FCompositeSection& Section = Montage->CompositeSections[Index];
		Section.NextSectionName = (!LoopSection.IsNone() && Section.SectionName == LoopSection) ? LoopSection : NAME_None;
	}

	Montage->SetCompositeLength(Cursor);
	Montage->BlendIn.SetBlendTime(BlendInTime);
	Montage->BlendOut.SetBlendTime(BlendOutTime);
	Montage->MarkPackageDirty();
	return Montage->GetPlayLength();
#else
	return -1.f;
#endif
}
