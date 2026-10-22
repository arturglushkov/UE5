#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NOARTypes.h"
#include "NOARSettings.generated.h"

class UCurveFloat;

/**
 * Project-wide tunables from the Standard, editable in Project Settings > Game > NOAR.
 * Blueprints read these through UNOARFunctionLibrary::GetNOARSettings() instead of hard-coding numbers.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "NOAR"))
class NOARCORE_API UNOARSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNOARSettings();

	// ----- Interaction (section 8) -----

	/** Max distance from the character capsule to the interaction point, cm. 150 too short, 250 opens across the room. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = 50, ClampMax = 400))
	float MaxInteractionDistance = 200.f;

	/** Indicators are drawn only within this radius, cm */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float IndicatorVisibleDistance = 400.f;

	/** Trace length from camera / cursor, cm */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float TraceLength = 600.f;

	/** Seconds between interaction traces (0.05 = 20 Hz) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float TraceInterval = 0.05f;

	/** Indicators fade out after this many seconds of mouse inactivity (clean frame for screenshots) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float IndicatorIdleFade = 2.f;

	/** Highlight: scene colour multiplier inside the stencil mask (section 8) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float HighlightBrightness = 1.12f;

	// ----- Mechanism timing, seconds (section 9) -----

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float HingedDoorOpen = 0.9f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float HingedDoorClose = 1.1f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float DrawerOpen = 0.7f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float DrawerClose = 0.9f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float FlapOpen = 1.2f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float FlapClose = 1.4f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float InteriorDoorOpen = 1.4f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float InteriorDoorClose = 1.6f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	float PocketDoor = 1.2f;

	/** Soft-close curve (0..1 time, 0..1 alpha): fast start, controlled middle, last 4% over last 20% of time */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Mechanisms")
	TSoftObjectPtr<UCurveFloat> SoftCloseCurve;

	// ----- Character / camera (section 7) -----

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character")
	float WalkSpeed = 160.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character")
	float FastWalkSpeed = 280.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character")
	float FirstPersonWalkSpeed = 140.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character")
	float MaxAcceleration = 600.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character")
	float BrakingDeceleration = 900.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Character")
	float RotationRateYaw = 360.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float ThirdPersonArmLength = 320.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector ThirdPersonSocketOffset = FVector(0.f, 35.f, 55.f);
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float ThirdPersonFOV = 75.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float FirstPersonFOV = 80.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraLagSpeed = 8.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraRotationLagSpeed = 12.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float PitchMin = -60.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float PitchMax = 25.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float LookSensitivity = 0.6f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraModeBlendTime = 0.6f;
	/** Character dither-fades out below this arm length, cm */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CharacterFadeArmLength = 120.f;

	// ----- Presentation -----

	/** Show prices in product cards and configurator */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	bool bShowPrices = false;

	/** Default tier when Synth Benchmark is unavailable */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	ENOARQualityTier DefaultQualityTier = ENOARQualityTier::High;
};
