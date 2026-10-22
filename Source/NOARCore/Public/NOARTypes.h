#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NOARTypes.generated.h"

class UMaterialInterface;
class UTexture2D;

// ---------------------------------------------------------------------------
// Enums shared by every Blueprint. Defined in C++ so renaming/reordering never
// breaks Blueprint references (Standard v1.1, section 15).
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ENOARInteractState : uint8
{
	Closed		UMETA(DisplayName = "Closed"),
	Opening		UMETA(DisplayName = "Opening"),
	Open		UMETA(DisplayName = "Open"),
	Closing		UMETA(DisplayName = "Closing")
};

UENUM(BlueprintType)
enum class ENOARInteractDenyReason : uint8
{
	OK			UMETA(DisplayName = "OK"),
	Locked		UMETA(DisplayName = "Locked"),
	Blocked		UMETA(DisplayName = "Blocked by moving part"),
	TooFar		UMETA(DisplayName = "Too far"),
	Dependency	UMETA(DisplayName = "Open outer part first"),
	Occluded	UMETA(DisplayName = "Occluded")
};

UENUM(BlueprintType)
enum class ENOARHingeSide : uint8
{
	Left	UMETA(DisplayName = "Left"),
	Right	UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class ENOARMechanism : uint8
{
	HingedDoor		UMETA(DisplayName = "Hinged door (CLIP top / interior / fridge)"),
	Drawer			UMETA(DisplayName = "Drawer (LEGRABOX / MOVENTO / TANDEM)"),
	FlapUp			UMETA(DisplayName = "Lift-up (AVENTOS HK / HK top)"),
	FlapBiFold		UMETA(DisplayName = "Bi-fold up (AVENTOS HF)"),
	FlapSwing		UMETA(DisplayName = "Swing-up over cabinet (AVENTOS HS)"),
	FlapLift		UMETA(DisplayName = "Parallel lift (AVENTOS HL)"),
	FlapDown		UMETA(DisplayName = "Drop-down (dishwasher / oven)"),
	Sliding			UMETA(DisplayName = "Sliding on track"),
	Pocket			UMETA(DisplayName = "Pocket door / pocket system"),
	BiFoldSide		UMETA(DisplayName = "Bi-fold sideways"),
	MurphyBed		UMETA(DisplayName = "Murphy bed")
};

/** VISUAL = single axis / simple curves (95% of cabinets). MANUFACTURER = multi-link trajectory from Blum/Hawa data (hero only). */
UENUM(BlueprintType)
enum class ENOARKinematicsMode : uint8
{
	Visual			UMETA(DisplayName = "Visual kinematics"),
	Manufacturer	UMETA(DisplayName = "Manufacturer kinematics")
};

UENUM(BlueprintType)
enum class ENOARCameraMode : uint8
{
	ThirdPerson		UMETA(DisplayName = "Third person"),
	FirstPerson		UMETA(DisplayName = "First person"),
	Photo			UMETA(DisplayName = "Photo mode")
};

UENUM(BlueprintType)
enum class ENOARInputMode : uint8
{
	Mouse3P		UMETA(DisplayName = "Mouse, third person (click-to-walk allowed)"),
	FirstPerson	UMETA(DisplayName = "First person (Move closer)"),
	Gamepad		UMETA(DisplayName = "Gamepad (Move closer)")
};

UENUM(BlueprintType)
enum class ENOARLightingPreset : uint8
{
	Day		UMETA(DisplayName = "Day"),
	Evening	UMETA(DisplayName = "Evening"),
	Night	UMETA(DisplayName = "Night")
};

/** NOAR quality tiers -> engine scalability levels 1..4 (DefaultScalability.ini). */
UENUM(BlueprintType)
enum class ENOARQualityTier : uint8
{
	Unset	= 0	UMETA(Hidden),
	Low		= 1	UMETA(DisplayName = "NOAR_LOW"),
	Medium	= 2	UMETA(DisplayName = "NOAR_MEDIUM"),
	High	= 3	UMETA(DisplayName = "NOAR_HIGH"),
	Ultra	= 4	UMETA(DisplayName = "NOAR_ULTRA")
};

// ---------------------------------------------------------------------------
// Data table rows (CSV in Data/<Apartment>/). Column names must match exactly.
// ---------------------------------------------------------------------------

/** One finish option a client can pick. Mirrors DA_MaterialOption; CSV row for bulk import. */
USTRUCT(BlueprintType)
struct NOARCORE_API FNOARFinishOptionRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Configurable.* slot this option belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish", meta = (Categories = "Configurable"))
	FGameplayTag Slot;

	/** Stable ID saved in SaveGame (never rename) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	FName OptionID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	FText DisplayName;

	/** Spec code from the ID drawings: M01, LAM01, SS01, GL02 ... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	FString SpecCode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	TSoftObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	bool bIsDefault = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	FString Manufacturer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	FString SKU;

	/** Price per unit, 0 = not shown */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	float Price = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	FString PriceUnit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	FText Description;
};

/** Product card shown on Alt+click (section 10). */
USTRUCT(BlueprintType)
struct NOARCORE_API FNOARProductInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
	FString SpecCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
	FString Manufacturer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
	FString Model;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
	FString Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
	FString Finish;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
	FString Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
	float Price = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Product")
	FString URL;
};

/** Light preset row: one per LightGroup per preset (DA_LightPreset is built from these). */
USTRUCT(BlueprintType)
struct NOARCORE_API FNOARLightGroupState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (Categories = "LightGroup"))
	FGameplayTag Group;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	bool bOn = true;

	/** 0..1 multiplier on the fixture's physical lumens (dimmer) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (ClampMin = 0, ClampMax = 1))
	float Dimmer = 1.f;
};

/** Everything SG_NOAR_ClientConfiguration persists for one apartment (section 10). */
USTRUCT(BlueprintType)
struct NOARCORE_API FNOARClientConfiguration
{
	GENERATED_BODY()

	/** Bump when the layout of this struct changes; mismatch = reset to defaults instead of crash */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SchemaVersion = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName ApartmentID;

	/** Configurable.* slot -> OptionID */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TMap<FGameplayTag, FName> Finishes;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FNOARLightGroupState> LightGroups;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	ENOARLightingPreset LightingPreset = ENOARLightingPreset::Day;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	ENOARCameraMode CameraMode = ENOARCameraMode::ThirdPerson;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FTransform LastPlayerTransform;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName LastRoom;
};
