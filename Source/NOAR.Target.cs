using UnrealBuildTool;
using System.Collections.Generic;

public class NOARTarget : TargetRules
{
	public NOARTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("NOARCore");

		// Client build: no console, no debug menu, no editor content
		if (Configuration == UnrealTargetConfiguration.Shipping)
		{
			bUseLoggingInShipping = false;
			bUseChecksInShipping = false;
		}
	}
}
