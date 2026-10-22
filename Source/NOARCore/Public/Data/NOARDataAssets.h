#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NOARTypes.h"
#include "NOARDataAssets.generated.h"

class UWorld;
class UDataTable;
class UTexture2D;

/** DA_LightPreset: Day / Evening / Night (Standard v1.1, sections 6 and 10). */
UCLASS(BlueprintType)
class NOARCORE_API UNOARLightPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preset")
	ENOARLightingPreset Preset = ENOARLightingPreset::Day;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preset")
	FText DisplayName;

	/** Sun illuminance, lux (Day 75 000, Evening 8 000, Night 0) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	float SunLux = 75000.f;

	/** Sun elevation / azimuth, degrees */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	float SunElevation = 38.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	float SunAzimuth = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	float SunKelvin = 5500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sky")
	float SkyLightIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sky")
	FLinearColor SkyLightTint = FLinearColor::White;

	/** Index into the DayNightManager's backdrop HDRI list */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sky")
	int32 BackdropIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure")
	float ExposureCompensation = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure")
	float WhiteBalanceKelvin = 6000.f;

	/** Interior light groups state for this preset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior")
	TArray<FNOARLightGroupState> LightGroups;

	/** Delay before interior lights come on when entering the preset (evening: 0.8 s, like someone switched them) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interior")
	float InteriorDelay = 0.f;
};

/** DA_Room: one entry of the room navigation and photo-mode camera presets. */
UCLASS(BlueprintType)
class NOARCORE_API UNOARRoomData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	FName RoomID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room", meta = (Categories = "Zone"))
	FGameplayTag ZoneTag;

	/** Teleport destination (world space of the apartment level) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	FTransform TeleportTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	ENOARCameraMode RecommendedCamera = ENOARCameraMode::ThirdPerson;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	TSoftObjectPtr<UTexture2D> Preview;

	/** Configurator slots shown first when the player is in this room */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room", meta = (Categories = "Configurable"))
	TArray<FGameplayTag> ConfigurableSlots;

	/** Photo-mode camera presets: the designer's own render viewpoints */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	TArray<FTransform> CameraPresets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	int32 SortOrder = 0;
};

/** DA_Product: manufacturer / model / finish / price for the product card (Alt+click). */
UCLASS(BlueprintType)
class NOARCORE_API UNOARProductData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	FNOARProductInfo Info;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	FString Location;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	FString Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	TSoftObjectPtr<UTexture2D> Photo;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Product"), GetFName());
	}
};

/**
 * DA_Apartment: the root of one apartment (Standard v1.1, section 10/14). Primary Asset from v1.0 so that
 * Asset Manager labels / per-apartment chunks can be added later without touching anything else.
 */
UCLASS(BlueprintType)
class NOARCORE_API UNOARApartmentData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	FName ApartmentID = TEXT("DEM");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	FText Address;

	/** "Concept 08.11.2026 / ID drawings 09.30.2026" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	FString DesignVersion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	TArray<TObjectPtr<UNOARRoomData>> Rooms;

	/** Rows of FNOARFinishOptionRow */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	TObjectPtr<UDataTable> FinishOptions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	TArray<TObjectPtr<UNOARLightPreset>> LightPresets;

	/** Where the character spawns (private vestibule facing the living room) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	FTransform StartTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	TSoftObjectPtr<UTexture2D> Preview;

	/** Floor-plan texture for the interactive plan (D1.00) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apartment")
	TSoftObjectPtr<UTexture2D> FloorPlan;

	UFUNCTION(BlueprintPure, Category = "Apartment")
	UNOARRoomData* FindRoom(FName RoomID) const;

	UFUNCTION(BlueprintPure, Category = "Apartment")
	UNOARLightPreset* FindPreset(ENOARLightingPreset Preset) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Apartment"), ApartmentID.IsNone() ? GetFName() : ApartmentID);
	}
};
