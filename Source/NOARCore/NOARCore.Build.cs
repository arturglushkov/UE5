using UnrealBuildTool;

// NOARCore: the only C++ in the project (Standard v1.1, section 15).
// Holds enums, structs, native gameplay tags, project settings and shipping guards.
// Everything else is Blueprint.
public class NOARCore : ModuleRules
{
	public NOARCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"DeveloperSettings",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RenderCore",
			"RHI"
		});
	}
}
