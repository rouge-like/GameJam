// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Fusion : ModuleRules
{
	public Fusion(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "AudioMixer", "HTTP", "AudioCapture" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// ModuleDirectory path로 잡도록 추가 (Rider에러 방지)
		PublicIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(ModuleDirectory);
		
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
	// 언리얼 분담
	// 통신담당, AI반과 기술적논의, 좌표싱크 (흰둥)
	// UI, 오디오만들기, 재미니욕하기 (짱아)
	// 레벨구성, 카메라 인터렉티브, 헤르메스 (철수)
}
