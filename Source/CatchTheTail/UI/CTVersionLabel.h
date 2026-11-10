// Copyright Moon Party Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

class UWorld;

// 인트로와 메인메뉴 화면 오른쪽 아래에 "v0.1.0 · 초기 버전"을 보임.
// 버전은 프로젝트 설정(Project Settings > Description > Project Version, DefaultGame.ini 의 ProjectVersion)에서 읽고,
// "초기 버전" 문구는 현재 언어(한/영/일)에 맞춰 바뀜. 클릭은 가로채지 않음.
namespace CTVersionLabel
{
	void Show(const UWorld* World, TSharedPtr<SWidget>& Holder);
	void Hide(const UWorld* World, TSharedPtr<SWidget>& Holder);
}
