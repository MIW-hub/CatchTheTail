// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

// 채팅 비속어, 은어 필터. 서버에서 보내기 전에 걸러냄
struct CATCHTHETAIL_API FCTChatFilter
{
	// 금지어가 들어 있으면 그 글자만 '*'로 가린 문장을 돌려줌.
	// 띄어쓰기와 특수문자로 끊어 쓴 것(시 발, 씨.발)도 같이 걸러냄. bOutFiltered는 하나라도 가렸는지
	static FString Sanitize(const FString& Message, bool* bOutFiltered = nullptr);
};
