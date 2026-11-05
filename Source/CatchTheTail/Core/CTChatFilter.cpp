// Copyright Moon Party Games. All Rights Reserved.
#include "CTChatFilter.h"

namespace
{
	// 금지어 목록. 소문자, 공백 없이 적음. 필요하면 여기에 추가
	const TCHAR* const BannedWords[] =
	{
		// 한글 욕설
		TEXT("시발"), TEXT("씨발"), TEXT("씨빨"), TEXT("시빨"), TEXT("쉬발"), TEXT("씹"), TEXT("좆"), TEXT("존나"), TEXT("졸라"),
		TEXT("병신"), TEXT("븅신"), TEXT("빙신"), TEXT("지랄"), TEXT("염병"), TEXT("미친놈"), TEXT("미친년"), TEXT("개새"), TEXT("새끼"),
		TEXT("개년"), TEXT("썅"), TEXT("쌍년"), TEXT("쌍놈"), TEXT("닥쳐"), TEXT("꺼져"), TEXT("뒤져"), TEXT("뒈져"), TEXT("죽어버려"),
		TEXT("엿먹"), TEXT("걸레"), TEXT("창녀"), TEXT("창놈"), TEXT("보지"), TEXT("자지"), TEXT("섹스"), TEXT("딸딸이"), TEXT("애미"),
		TEXT("에미"), TEXT("느금"), TEXT("니애미"), TEXT("니미"), TEXT("엠창"), TEXT("앰창"), TEXT("패드립"),
		// 초성, 줄임말
		TEXT("ㅅㅂ"), TEXT("ㅆㅂ"), TEXT("ㅂㅅ"), TEXT("ㅄ"), TEXT("ㅈㄹ"), TEXT("ㅁㅊ"), TEXT("ㅗ"), TEXT("ㅅㄲ"), TEXT("ㄲㅈ"), TEXT("ㄷㅊ"),
		// 혐오, 비하 은어
		TEXT("한남"), TEXT("김치녀"), TEXT("된장녀"), TEXT("맘충"), TEXT("틀딱"), TEXT("급식충"), TEXT("일베"), TEXT("메갈"),
		TEXT("짱깨"), TEXT("쪽발이"), TEXT("똥남아"), TEXT("흑형"),
		// 영어
		TEXT("fuck"), TEXT("shit"), TEXT("bitch"), TEXT("asshole"), TEXT("bastard"), TEXT("dick"), TEXT("pussy"), TEXT("cunt"),
		TEXT("nigger"), TEXT("nigga"), TEXT("faggot"), TEXT("whore"), TEXT("slut"), TEXT("retard"), TEXT("kys"), TEXT("wtf"), TEXT("stfu"),
	};

	bool IsKept(TCHAR Character)
	{
		// 검사에 쓰는 글자: 한글(자모 포함), 영문, 숫자
		return FChar::IsAlnum(Character) || (Character >= 0xAC00 && Character <= 0xD7A3) || (Character >= 0x3130 && Character <= 0x318F);
	}
}

FString FCTChatFilter::Sanitize(const FString& Message, bool* bOutFiltered)
{
	// 띄어쓰기, 기호를 뺀 글자만 모아 검사하고, 원문 위치를 기억해 둠
	FString Normalized;
	TArray<int32> SourceIndex;
	for (int32 Index = 0; Index < Message.Len(); ++Index)
	{
		const TCHAR Character = Message[Index];
		if (IsKept(Character))
		{
			Normalized.AppendChar(FChar::ToLower(Character));
			SourceIndex.Add(Index);
		}
	}

	FString Result = Message;
	bool bFiltered = false;
	for (const TCHAR* Word : BannedWords)
	{
		const FString Banned(Word);
		int32 SearchFrom = 0;
		while (true)
		{
			const int32 Found = Normalized.Find(Banned, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
			if (Found == INDEX_NONE) { break; }

			// 걸린 글자부터 끝 글자까지(사이 기호 포함) 가림
			for (int32 Cursor = SourceIndex[Found]; Cursor <= SourceIndex[Found + Banned.Len() - 1]; ++Cursor)
			{
				if (IsKept(Result[Cursor])) { Result[Cursor] = TEXT('*'); }
			}
			bFiltered = true;
			SearchFrom = Found + Banned.Len();
		}
	}

	if (bOutFiltered) { *bOutFiltered = bFiltered; }
	return Result;
}
