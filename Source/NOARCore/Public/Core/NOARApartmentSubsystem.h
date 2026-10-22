#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NOARTypes.h"
#include "NOARApartmentSubsystem.generated.h"

class UNOARApartmentData;
class UNOARRoomData;
class ANOARCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNOAROnRoomEntered, UNOARRoomData*, Room);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNOAROnApartmentReady);

/**
 * BP_ApartmentManager (section 10/15) as a world subsystem: current DA_Apartment, room list, teleport,
 * restore / save of the client configuration (finishes, light groups, preset, last position).
 */
UCLASS()
class NOARCORE_API UNOARApartmentSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "NOAR|Apartment")
	FNOAROnRoomEntered OnRoomEntered;

	UPROPERTY(BlueprintAssignable, Category = "NOAR|Apartment")
	FNOAROnApartmentReady OnApartmentReady;

	/** Called by ANOARGameMode::BeginPlay */
	void BeginApartment(UNOARApartmentData* InApartment);

	UFUNCTION(BlueprintPure, Category = "NOAR|Apartment")
	UNOARApartmentData* GetApartment() const { return Apartment; }

	UFUNCTION(BlueprintPure, Category = "NOAR|Apartment")
	TArray<UNOARRoomData*> GetRooms() const;

	UFUNCTION(BlueprintCallable, Category = "NOAR|Apartment")
	void TeleportToRoom(FName RoomID);

	UFUNCTION(BlueprintPure, Category = "NOAR|Apartment")
	UNOARRoomData* GetCurrentRoom() const { return CurrentRoom; }

	/** Zone volumes call this when the player enters a zone */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Apartment")
	void NotifyRoomEntered(FName RoomID);

	/** Build the configuration from live state and save (deferred by default) */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Apartment")
	void SaveNow(bool bDeferred = true);

	UFUNCTION(BlueprintCallable, Category = "NOAR|Apartment")
	void RestoreSavedConfiguration();

	UFUNCTION(BlueprintCallable, Category = "NOAR|Apartment")
	void ResetDesign();

	virtual void Deinitialize() override;

private:
	FNOARClientConfiguration BuildConfiguration() const;
	ANOARCharacter* GetCharacter() const;

	UPROPERTY()
	TObjectPtr<UNOARApartmentData> Apartment;

	UPROPERTY()
	TObjectPtr<UNOARRoomData> CurrentRoom;

	bool bRestored = false;
};
