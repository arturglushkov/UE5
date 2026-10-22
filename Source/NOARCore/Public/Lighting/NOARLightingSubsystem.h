#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "NOARTypes.h"
#include "NOARLightingSubsystem.generated.h"

class ANOARInteractiveLight;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNOAROnLightGroupChanged, FGameplayTag, Group, bool, bOn, float, Dimmer);

/**
 * BP_LightingManager (Standard v1.1, section 9/15) as a world subsystem: no placement, no GameMode plumbing.
 * Lights register themselves by LightGroupTag; switches, UI and the day/night manager only talk in tags.
 * "LightGroup.Kitchen" toggles every group under it (MatchesTag).
 */
UCLASS()
class NOARCORE_API UNOARLightingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "NOAR|Lighting")
	FNOAROnLightGroupChanged OnGroupChanged;

	void RegisterLight(ANOARInteractiveLight* Light);
	void UnregisterLight(ANOARInteractiveLight* Light);

	/** Toggle every light whose tag matches Group (parent tags match children) */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Lighting")
	void ToggleGroup(FGameplayTag Group);

	UFUNCTION(BlueprintCallable, Category = "NOAR|Lighting")
	void SetGroup(FGameplayTag Group, bool bOn, float Dimmer = 1.f, bool bAnimate = true);

	/** Apply a full preset (day/evening/night or save restore) */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Lighting")
	void ApplyGroupStates(const TArray<FNOARLightGroupState>& States, bool bAnimate = true);

	/** True when at least one light of the group is on */
	UFUNCTION(BlueprintPure, Category = "NOAR|Lighting")
	bool IsGroupOn(FGameplayTag Group) const;

	UFUNCTION(BlueprintPure, Category = "NOAR|Lighting")
	TArray<FNOARLightGroupState> GetGroupStates() const;

	UFUNCTION(BlueprintPure, Category = "NOAR|Lighting")
	TArray<FGameplayTag> GetKnownGroups() const;

	/** All lights of a zone (Zone.Kitchen -> every LightGroup.Kitchen.*) */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Lighting")
	void SetZone(FGameplayTag Zone, bool bOn);

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<ANOARInteractiveLight>> Lights;
};
