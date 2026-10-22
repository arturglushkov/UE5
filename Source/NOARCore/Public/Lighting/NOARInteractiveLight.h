#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "NOARInteractiveLight.generated.h"

class UStaticMeshComponent;
class ULocalLightComponent;
class UMaterialInstanceDynamic;

/**
 * BP_InteractiveLight (Standard v1.1, section 6): one fixture = fixture mesh + emissive strip/diffuser mesh +
 * one or more light components (Rect for LED, Spot for downlights, Point for pendants). Add the light components
 * in the Blueprint subclass; this class finds every ULocalLightComponent on the actor.
 * Physical units: Lumens and Kelvin set the lights; EmissiveNits drives the "Emissive" scalar on the strip material.
 * ON/OFF is a 0.3 s dimmer ramp, never a hard switch.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARInteractiveLight : public AActor
{
	GENERATED_BODY()

public:
	ANOARInteractiveLight();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UStaticMeshComponent> FixtureMesh;

	/** LED tape / diffuser with M_Master_Emissive; parameter "Emissive" is scaled by dimmer */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UStaticMeshComponent> StripMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Light", meta = (Categories = "LightGroup"))
	FGameplayTag LightGroupTag;

	/** Total luminous flux of the fixture (lm). Split evenly across light components. L02 LED: 630 per 120 cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Light", meta = (ClampMin = 0))
	float Lumens = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Light", meta = (ClampMin = 1700, ClampMax = 6500))
	float Kelvin = 3000.f;

	/** Luminance of the visible strip/diffuser, nit (opal LED channel ~ 3000) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Light", meta = (ClampMin = 0))
	float EmissiveNits = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Light")
	FName EmissiveParameterName = TEXT("Emissive");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Light")
	bool bStartOn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Light", meta = (ClampMin = 0.05))
	float RampSeconds = 0.3f;

	UFUNCTION(BlueprintCallable, Category = "NOAR|Light")
	void SetOn(bool bOn, bool bAnimate = true);

	UFUNCTION(BlueprintCallable, Category = "NOAR|Light")
	void SetDimmer(float InDimmer, bool bAnimate = true);

	UFUNCTION(BlueprintPure, Category = "NOAR|Light")
	bool IsOn() const { return bIsOn; }

	UFUNCTION(BlueprintPure, Category = "NOAR|Light")
	float GetDimmer() const { return Dimmer; }

	/** Re-read Lumens/Kelvin into the light components (call after editing in BP) */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "NOAR|Light")
	void ApplyPhysicalUnits();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	void ApplyLevel(float Level);

	UPROPERTY()
	TArray<TObjectPtr<ULocalLightComponent>> LightComponents;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> StripMID;

	bool bIsOn = true;
	float Dimmer = 1.f;
	float CurrentLevel = 1.f;
	float TargetLevel = 1.f;
};
