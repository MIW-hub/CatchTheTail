// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CatchTheTail : ModuleRules
{
	public CatchTheTail(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// UMG: 위젯 블루프린트(WBP)의 부모 C++ 클래스(UUserWidget)를 쓰기 위해 필요
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG"
		});

		// Slate / SlateCore: UMG 위젯 내부에서 쓰는 UI 기반 모듈
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate", "SlateCore"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
