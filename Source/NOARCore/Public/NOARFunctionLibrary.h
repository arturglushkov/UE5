#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NOARTypes.h"
#include "NOARFunctionLibrary.generated.h"

class UNOARSettings;
class UInputMappingContext;
class UInputAction;

/** Small helpers Blueprints cannot express cleanly: shipping guards, scalability tier, hardware facts. */
UCLASS()
class NOARCORE_API UNOARFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Project tunables (Project Settings > Game > NOAR) */
	UFUNCTION(BlueprintPure, Category = "NOAR")
	static const UNOARSettings* GetNOARSettings();

	/** True in Shipping builds. WBP_DebugMenu is never created when this is true (compile-time, not a flag). */
	UFUNCTION(BlueprintPure, Category = "NOAR|Build")
	static bool IsShippingBuild();

	/** True when the debug menu may exist (Development / DebugGame / Editor) */
	UFUNCTION(BlueprintPure, Category = "NOAR|Build")
	static bool IsDebugMenuAllowed();

	/** Engine version string for the Settings screen */
	UFUNCTION(BlueprintPure, Category = "NOAR|Build")
	static FString GetEngineVersionString();

	/** Apply a NOAR tier to all scalability groups (sg.*) and persist. Photo mode uses ApplyPhotoModeOverrides instead. */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Quality")
	static void ApplyQualityTier(ENOARQualityTier Tier, bool bHardwareRayTracing);

	/** Temporarily raise reflections/shadows to tier 4 for a still (section 11); call with bEnable=false to restore. */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Quality")
	static void ApplyPhotoModeOverrides(bool bEnable, ENOARQualityTier RestoreTier);

	/** True when the RHI reports hardware ray tracing (Windows RTX / experimental Apple Silicon M2+) */
	UFUNCTION(BlueprintPure, Category = "NOAR|Hardware")
	static bool IsHardwareRayTracingAvailable();

	/** True on macOS (Software Lumen is the production baseline there) */
	UFUNCTION(BlueprintPure, Category = "NOAR|Hardware")
	static bool IsMac();

	/** Tier chosen from Synth Benchmark GPU index: <150 Low, <300 Medium, <550 High, else Ultra */
	UFUNCTION(BlueprintPure, Category = "NOAR|Quality")
	static ENOARQualityTier TierFromGPUIndex(float GPUIndex);

	/**
	 * Editor/setup helper (used by Scripts/Editor/noar_setup_input.py): adds Key -> Action to a mapping context with the
	 * usual modifiers. Python cannot edit the mapping returned by MapKey (it gets a copy), so this lives in C++.
	 * bSwizzleYX: X axis value goes to Y (W/S on a 2D move action); bNegate: invert; DeadZone > 0 adds a radial dead zone;
	 * Scale != 1 adds a scalar modifier.
	 */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Input")
	static void AddKeyMapping(UInputMappingContext* Context, UInputAction* Action, FName KeyName, bool bSwizzleYX = false, bool bNegate = false, float DeadZone = 0.f, float Scale = 1.f);

	/** Removes every mapping from a context (setup re-runs). */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Input")
	static void ClearKeyMappings(UInputMappingContext* Context);
};
