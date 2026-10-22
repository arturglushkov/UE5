#include "Core/NOARGameMode.h"
#include "Core/NOARGameInstance.h"
#include "Core/NOARPlayerController.h"
#include "Core/NOARApartmentSubsystem.h"
#include "Character/NOARCharacter.h"
#include "Lighting/NOARDayNightManager.h"
#include "Data/NOARDataAssets.h"
#include "NOARCore.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

ANOARGameMode::ANOARGameMode()
{
	DefaultPawnClass = ANOARCharacter::StaticClass();
	PlayerControllerClass = ANOARPlayerController::StaticClass();
}

void ANOARGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// If the GameInstance already selected an apartment (apartment selector), prefer it
	if (UNOARGameInstance* GI = GetGameInstance<UNOARGameInstance>())
	{
		if (GI->SelectedApartment)
		{
			Apartment = GI->SelectedApartment;
		}
		else if (Apartment)
		{
			GI->SelectApartment(Apartment);
		}
	}
}

void ANOARGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (UNOARApartmentSubsystem* Sub = GetWorld()->GetSubsystem<UNOARApartmentSubsystem>())
	{
		Sub->BeginApartment(Apartment);
	}
	if (UNOARGameInstance* GI = GetGameInstance<UNOARGameInstance>())
	{
		GI->AutoDetectQualityIfNeeded();
	}
}

AActor* ANOARGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Spawn at the apartment start transform (private vestibule) unless a PlayerStart is placed explicitly
	if (TActorIterator<APlayerStart> It(GetWorld()); It)
	{
		return *It;
	}
	if (Apartment)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APlayerStart* Start = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Apartment->StartTransform, P);
		return Start;
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

ANOARDayNightManager* ANOARGameMode::GetDayNightManager() const
{
	if (TActorIterator<ANOARDayNightManager> It(GetWorld()); It)
	{
		return *It;
	}
	return nullptr;
}
