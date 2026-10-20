using UnrealBuildTool;

public class BochaNetEditorTarget : TargetRules
{
	public BochaNetEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "BochaNet", "BochaNetEditor" });
	}
}
