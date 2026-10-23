// Copyright Moon Party Games. All Rights Reserved.
#include "CTMapSettings.h"
#include "Engine/World.h"

UCTMapSettings::UCTMapSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("CT Maps");

	// 기본 목록. 프로젝트 설정에서 바꾸면 그 값이 우선합니다.
	FCTMapInfo TypeA;
	TypeA.DisplayName = NSLOCTEXT("CT", "Map_TypeA", "타입 A");
	TypeA.Description = NSLOCTEXT("CT", "Map_TypeA_Desc", "기본 맵입니다.");
	TypeA.Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Levels/TypeA.TypeA")));
	Maps.Add(TypeA);
}

int32 UCTMapSettings::GetMapCount()
{
	int32 Count = 0;
	for (const FCTMapInfo& Info : GetDefault<UCTMapSettings>()->Maps)
	{
		if (Info.bEnabled && !Info.Level.IsNull()) { ++Count; }
	}
	return Count;
}

bool UCTMapSettings::GetMapInfo(int32 Index, FCTMapInfo& OutInfo)
{
	const TArray<FCTMapInfo>& List = GetDefault<UCTMapSettings>()->Maps;
	if (!List.IsValidIndex(Index)) { return false; }

	const FCTMapInfo& Info = List[Index];
	if (!Info.bEnabled || Info.Level.IsNull()) { return false; }

	OutInfo = Info;
	return true;
}

int32 UCTMapSettings::GetFirstEnabledIndex()
{
	const TArray<FCTMapInfo>& List = GetDefault<UCTMapSettings>()->Maps;
	for (int32 Index = 0; Index < List.Num(); ++Index)
	{
		if (List[Index].bEnabled && !List[Index].Level.IsNull()) { return Index; }
	}
	return INDEX_NONE;
}

FString UCTMapSettings::GetTravelName(int32 Index)
{
	FCTMapInfo Info;
	return GetMapInfo(Index, Info) ? Info.Level.GetLongPackageName() : FString();
}
