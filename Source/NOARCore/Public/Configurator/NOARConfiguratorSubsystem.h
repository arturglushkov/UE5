#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "NOARTypes.h"
#include "NOARConfiguratorSubsystem.generated.h"

class UNOARConfigurableSurfaceComponent;
class UDataTable;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNOAROnConfigChanged, FGameplayTag, Slot, FName, OptionID);

/**
 * BP_MaterialConfigurator (section 10) as a world subsystem.
 * Options come from DT_FinishOptions_<APT> (rows of FNOARFinishOptionRow); surfaces register themselves.
 * ApplyOption: load MI (async-ready, normally preloaded), SetMaterialByName on every registered surface of the slot,
 * remember the choice, broadcast, deferred save through UNOARGameInstance.
 */
UCLASS()
class NOARCORE_API UNOARConfiguratorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "NOAR|Configurator")
	FNOAROnConfigChanged OnConfigChanged;

	void RegisterSurface(UNOARConfigurableSurfaceComponent* Surface);
	void UnregisterSurface(UNOARConfigurableSurfaceComponent* Surface);

	/** Called by the apartment subsystem once the DA_Apartment is known */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Configurator")
	void SetOptionsTable(UDataTable* Table);

	UFUNCTION(BlueprintCallable, Category = "NOAR|Configurator")
	bool ApplyOption(FGameplayTag Slot, FName OptionID, bool bSave = true);

	UFUNCTION(BlueprintCallable, Category = "NOAR|Configurator")
	void ApplyConfiguration(const TMap<FGameplayTag, FName>& Finishes);

	/** RESET DESIGN: every slot back to its bIsDefault row */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Configurator")
	void ResetToDefaults();

	UFUNCTION(BlueprintPure, Category = "NOAR|Configurator")
	TArray<FGameplayTag> GetSlots() const;

	UFUNCTION(BlueprintPure, Category = "NOAR|Configurator")
	TArray<FNOARFinishOptionRow> GetOptionsForSlot(FGameplayTag Slot) const;

	UFUNCTION(BlueprintPure, Category = "NOAR|Configurator")
	FName GetCurrentOption(FGameplayTag Slot) const;

	UFUNCTION(BlueprintPure, Category = "NOAR|Configurator")
	TMap<FGameplayTag, FName> GetCurrentConfiguration() const { return Current; }

	/** Load every option material now (loading screen) so switching is one frame */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Configurator")
	void PreloadAllMaterials();

	/** Sum of Price over current selection (0 when prices hidden / unknown) */
	UFUNCTION(BlueprintPure, Category = "NOAR|Configurator")
	float GetCurrentPriceTotal() const;

private:
	const FNOARFinishOptionRow* FindRow(FGameplayTag Slot, FName OptionID) const;
	void SaveDeferred();

	UPROPERTY()
	TObjectPtr<UDataTable> Options;

	UPROPERTY()
	TArray<TWeakObjectPtr<UNOARConfigurableSurfaceComponent>> Surfaces;

	TMap<FGameplayTag, FName> Current;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> PreloadedMaterials;
};
