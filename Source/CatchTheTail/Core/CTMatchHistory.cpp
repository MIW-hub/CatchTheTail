// Copyright Moon Party Games. All Rights Reserved.
#include "CTMatchHistory.h"

namespace
{
	// 프로그램이 켜져 있는 동안만 유지 (로비로 돌아가도 남음, 방을 나가면 ClearRecords)
	TArray<FCTMatchRecord> GRoomRecords;
}

TArray<FCTMatchRecord> UCTMatchHistory::LoadRecords()
{
	return GRoomRecords;
}

void UCTMatchHistory::AddRecord(const FCTMatchRecord& Record)
{
	GRoomRecords.Insert(Record, 0);
	if (GRoomRecords.Num() > MaxRecords)
	{
		GRoomRecords.SetNum(MaxRecords);
	}
}

void UCTMatchHistory::ClearRecords()
{
	GRoomRecords.Reset();
}
