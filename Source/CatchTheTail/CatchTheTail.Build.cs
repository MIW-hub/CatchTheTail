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
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "DeveloperSettings", "ApplicationCore", "AIModule", "NavigationSystem", "PhysicsCore"
		});

		// Slate / SlateCore: UMG 위젯 내부에서 쓰는 UI 기반 모듈
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate", "SlateCore", "EngineSettings"
		});

		// 에디터 전용: 애니메이션 블루프린트에 상체 슬롯을 넣고 몽타주를 만드는 도구(Core/CTAnimTools.cpp). 게임 빌드에는 들어가지 않음
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AnimGraph", "BlueprintGraph", "KismetCompiler", "AnimGraphRuntime" });
		}

		// Uncomment if you are using online features
		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils" });

		// 개발용 EOS 시험(Core/CTEosTest.cpp): 에픽 계정 없이 기기 ID로 EOS 로그인 확인
		PrivateDependencyModuleNames.AddRange(new string[] { "EOSShared", "EOSSDK", "OnlineServicesInterface", "OnlineServicesCommon", "CoreOnline", "OnlineServicesEOSGS", "SocketSubsystemEOS" });

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
