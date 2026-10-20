using UnrealBuildTool;

public class BochaNetEditor : ModuleRules
{
	public BochaNetEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "UnrealEd", "BlueprintGraph", "UMG", "UMGEditor", "AssetRegistry", "Json", "Kismet", "BochaNet"
		});
	}
}
