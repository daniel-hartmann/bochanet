using UnrealBuildTool;

public class BochaNet : ModuleRules
{
	public BochaNet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "AnimGraphRuntime", "AIModule", "NavigationSystem", "GameplayTasks" });
	}
}
