#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NOARTypes.h"
#include "NOARDayNightManager.generated.h"

class ADirectionalLight;
class ASkyLight;
class APostProcessVolume;
class AActor;
class UNOARLightPreset;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNOAROnPresetChanged, ENOARLightingPreset, Preset);

/**
 * BP_DayNightManager (Standard v1.1, section 10). One per apartment level.
 * Blends sun / sky / backdrop / exposure over BlendSeconds, then applies the preset's interior light groups
 * through UNOARLightingSubsystem (with the preset's InteriorDelay). No Lumen rebuild is needed.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARDayNightManager : public AActor
{
	GENERATED_BODY()

public:
	ANOARDayNightManager();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "NOAR|Scene")
	TObjectPtr<ADirectionalLight> Sun;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "NOAR|Scene")
	TObjectPtr<ASkyLight> SkyLight;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "NOAR|Scene")
	TObjectPtr<APostProcessVolume> PostProcess;

	/** HDRI backdrop actor; its material has "Blend" (0..1) and two texture slots "HDRI_A" / "HDRI_B" */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "NOAR|Scene")
	TObjectPtr<AActor> Backdrop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Scene")
	TArray<TObjectPtr<class UTexture>> BackdropHDRIs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Presets")
	TArray<TObjectPtr<UNOARLightPreset>> Presets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Presets")
	ENOARLightingPreset StartPreset = ENOARLightingPreset::Day;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Presets", meta = (ClampMin = 0.1))
	float BlendSeconds = 2.5f;

	UPROPERTY(BlueprintAssignable, Category = "NOAR|Events")
	FNOAROnPresetChanged OnPresetChanged;

	UFUNCTION(BlueprintCallable, Category = "NOAR|DayNight")
	void SetPreset(ENOARLightingPreset Preset, bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "NOAR|DayNight")
	void CyclePreset();

	UFUNCTION(BlueprintPure, Category = "NOAR|DayNight")
	ENOARLightingPreset GetCurrentPreset() const { return Current; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UNOARLightPreset* Find(ENOARLightingPreset P) const;
	void Capture(UNOARLightPreset* P, struct FNOARSkyState& Out) const;
	void Apply(const struct FNOARSkyState& S);
	void ApplyInterior();

	ENOARLightingPreset Current = ENOARLightingPreset::Day;
	float BlendT = 1.f;
	FTimerHandle InteriorTimer;

	TSharedPtr<struct FNOARSkyState> From;
	TSharedPtr<struct FNOARSkyState> To;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BackdropMID;
};

/** Everything that blends; kept out of UHT (plain struct). */
struct FNOARSkyState
{
	float SunLux = 0.f;
	float SunElevation = 0.f;
	float SunAzimuth = 0.f;
	float SunKelvin = 5500.f;
	float SkyIntensity = 1.f;
	FLinearColor SkyTint = FLinearColor::White;
	float ExposureComp = 0.f;
	float WhiteBalance = 6000.f;
	int32 BackdropIndex = 0;
};
