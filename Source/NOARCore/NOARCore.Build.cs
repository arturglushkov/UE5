using UnrealBuildTool;

// NOARCore (Standard v1.1, section 15): the project's C++ framework.
// Interaction, mechanisms, lighting, character/camera, configurator, data and save live here;
// Blueprints are thin subclasses that assign meshes, input assets and tuning values.
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
			"InputCore",
			"GameplayTags",
			"DeveloperSettings",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RenderCore",
			"RHI",
			"SynthBenchmark"
		});

		PublicIncludePaths.AddRange(new string[] { ModuleDirectory + "/Public" });
	}
}
