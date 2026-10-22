#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NOARGameMode.generated.h"

class UNOARApartmentData;
class ANOARDayNightManager;

/** BP_NOAR_GameMode: spawns the character at the apartment's start transform; exposes the level's managers. */
UCLASS(Blueprintable)
class NOARCORE_API ANOARGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANOARGameMode();

	/** The apartment this level belongs to (set in the level's GameMode override / BP subclass) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR")
	TObjectPtr<UNOARApartmentData> Apartment;

	UFUNCTION(BlueprintPure, Category = "NOAR")
	ANOARDayNightManager* GetDayNightManager() const;

protected:
	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
