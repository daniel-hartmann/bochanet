using UnrealBuildTool;

public class BochaNetTarget : TargetRules
{
	public BochaNetTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("BochaNet");
	}
}
