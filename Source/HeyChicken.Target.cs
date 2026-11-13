// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

// 패키징된 실행 파일 이름을 HeyChicken.exe 로 만드는 빌드 타깃.
// 게임 코드 모듈은 그대로 CatchTheTail 이라 에셋 경로(/Script/CatchTheTail)는 바뀌지 않는다.
// 패키징할 때 빌드 타깃은 DefaultGame.ini 의 BuildTarget=HeyChicken 으로 자동 선택된다.
public class HeyChickenTarget : TargetRules
{
	public HeyChickenTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("CatchTheTail");
	}
}
