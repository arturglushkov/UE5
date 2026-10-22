using UnrealBuildTool;
using System.Collections.Generic;

public class NOAREditorTarget : TargetRules
{
	public NOAREditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NOARCore");
	}
}
