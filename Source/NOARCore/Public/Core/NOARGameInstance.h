#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "NOARTypes.h"
#include "NOARGameInstance.generated.h"

class UNOARApartmentData;

/** SG_NOAR_ClientConfiguration: one slot per apartment + one settings slot (section 10). */
UCLASS()
class NOARCORE_API UNOARSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FNOARClientConfiguration Configuration;
};

UCLASS()
class NOARCORE_API UNOARSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 SchemaVersion = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	ENOARQualityTier QualityTier = ENOARQualityTier::High;
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	bool bHardwareRayTracing = false;
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float FOV = 75.f;
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float LookSensitivity = 0.6f;
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString Language = TEXT("en");
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	float MasterVolume = 1.f;
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName LastApartment;
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	bool bBenchmarkDone = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNOAROnQualityChanged, ENOARQualityTier, Tier);

/**
 * BP_NOAR_GameInstance (section 10/15): selected apartment, user settings, save/load.
 * Lives across level loads; everything per-level is in subsystems and managers.
 */
UCLASS(Blueprintable)
class NOARCORE_API UNOARGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// ---- apartment ----
	UPROPERTY(BlueprintReadOnly, Category = "NOAR|Apartment")
	TObjectPtr<UNOARApartmentData> SelectedApartment;

	UFUNCTION(BlueprintCallable, Category = "NOAR|Apartment")
	void SelectApartment(UNOARApartmentData* Apartment);

	/** Opens the apartment level through L_Boot's loading screen */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Apartment")
	void OpenSelectedApartment();

	// ---- configuration save (per apartment) ----
	UFUNCTION(BlueprintCallable, Category = "NOAR|Save")
	bool LoadConfiguration(FName ApartmentID, FNOARClientConfiguration& Out) const;

	UFUNCTION(BlueprintCallable, Category = "NOAR|Save")
	void SaveConfiguration(const FNOARClientConfiguration& Config);

	/** Debounced save: many configurator clicks -> one write */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Save")
	void SaveConfigurationDeferred(const FNOARClientConfiguration& Config);

	UFUNCTION(BlueprintCallable, Category = "NOAR|Save")
	void DeleteConfiguration(FName ApartmentID);

	// ---- settings ----
	UFUNCTION(BlueprintPure, Category = "NOAR|Settings")
	UNOARSettingsSave* GetSettings() const { return Settings; }

	UFUNCTION(BlueprintCallable, Category = "NOAR|Settings")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "NOAR|Settings")
	void SetQualityTier(ENOARQualityTier Tier, bool bHardwareRayTracing);

	UFUNCTION(BlueprintPure, Category = "NOAR|Settings")
	ENOARQualityTier GetQualityTier() const;

	/** Runs once: Synth Benchmark -> tier */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Settings")
	void AutoDetectQualityIfNeeded();

	UFUNCTION(BlueprintCallable, Category = "NOAR|Settings")
	void SetLanguage(const FString& Culture);

	UPROPERTY(BlueprintAssignable, Category = "NOAR|Settings")
	FNOAROnQualityChanged OnQualityChanged;

	static FString ConfigSlotName(FName ApartmentID) { return FString::Printf(TEXT("NOAR_%s"), *ApartmentID.ToString()); }
	static const TCHAR* SettingsSlotName() { return TEXT("NOAR_Settings"); }

private:
	UPROPERTY()
	TObjectPtr<UNOARSettingsSave> Settings;

	FNOARClientConfiguration PendingConfig;
	FTimerHandle SaveTimer;
	void FlushDeferredSave();
};
