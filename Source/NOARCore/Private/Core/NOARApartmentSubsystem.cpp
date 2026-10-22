#include "Core/NOARApartmentSubsystem.h"
#include "Core/NOARGameInstance.h"
#include "Configurator/NOARConfiguratorSubsystem.h"
#include "Lighting/NOARLightingSubsystem.h"
#include "Lighting/NOARDayNightManager.h"
#include "Character/NOARCharacter.h"
#include "Data/NOARDataAssets.h"
#include "NOARCore.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void UNOARApartmentSubsystem::BeginApartment(UNOARApartmentData* InApartment)
{
	Apartment = InApartment;
	if (!Apartment)
	{
		UE_LOG(LogNOAR, Warning, TEXT("ApartmentSubsystem: no DA_Apartment on the GameMode"));
		return;
	}

	if (UNOARConfiguratorSubsystem* Cfg = GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>())
	{
		Cfg->SetOptionsTable(Apartment->FinishOptions);
		Cfg->PreloadAllMaterials();
	}

	// restore after one frame: lights / surfaces register on their own BeginPlay
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		RestoreSavedConfiguration();
		OnApartmentReady.Broadcast();
	});
}

TArray<UNOARRoomData*> UNOARApartmentSubsystem::GetRooms() const
{
	TArray<UNOARRoomData*> Out;
	if (Apartment)
	{
		for (UNOARRoomData* R : Apartment->Rooms)
		{
			if (R) Out.Add(R);
		}
		Out.Sort([](const UNOARRoomData& A, const UNOARRoomData& B) { return A.SortOrder < B.SortOrder; });
	}
	return Out;
}

ANOARCharacter* UNOARApartmentSubsystem::GetCharacter() const
{
	return Cast<ANOARCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
}

void UNOARApartmentSubsystem::TeleportToRoom(FName RoomID)
{
	UNOARRoomData* Room = Apartment ? Apartment->FindRoom(RoomID) : nullptr;
	ANOARCharacter* C = GetCharacter();
	if (!Room || !C)
	{
		return;
	}
	// The fade is driven by the HUD (OnRoomEntered -> fade in); teleport happens now
	C->TeleportTo(Room->TeleportTransform);
	if (Room->RecommendedCamera == ENOARCameraMode::FirstPerson && C->GetCameraMode() != ENOARCameraMode::FirstPerson)
	{
		// only a hint; never switch without the user (section 7)
	}
	NotifyRoomEntered(RoomID);
}

void UNOARApartmentSubsystem::NotifyRoomEntered(FName RoomID)
{
	UNOARRoomData* Room = Apartment ? Apartment->FindRoom(RoomID) : nullptr;
	if (Room && Room != CurrentRoom)
	{
		CurrentRoom = Room;
		OnRoomEntered.Broadcast(Room);
		SaveNow(true);
	}
}

FNOARClientConfiguration UNOARApartmentSubsystem::BuildConfiguration() const
{
	FNOARClientConfiguration C;
	C.ApartmentID = Apartment ? Apartment->ApartmentID : NAME_None;
	if (const UNOARConfiguratorSubsystem* Cfg = GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>())
	{
		C.Finishes = Cfg->GetCurrentConfiguration();
	}
	if (const UNOARLightingSubsystem* L = GetWorld()->GetSubsystem<UNOARLightingSubsystem>())
	{
		C.LightGroups = L->GetGroupStates();
	}
	for (TActorIterator<ANOARDayNightManager> It(GetWorld()); It; ++It)
	{
		C.LightingPreset = It->GetCurrentPreset();
		break;
	}
	if (const ANOARCharacter* Ch = GetCharacter())
	{
		C.CameraMode = Ch->GetCameraMode();
		C.LastPlayerTransform = Ch->GetActorTransform();
	}
	C.LastRoom = CurrentRoom ? CurrentRoom->RoomID : NAME_None;
	return C;
}

void UNOARApartmentSubsystem::SaveNow(bool bDeferred)
{
	if (!Apartment || !bRestored)
	{
		return; // never overwrite a save with the pre-restore state
	}
	UNOARGameInstance* GI = Cast<UNOARGameInstance>(GetWorld()->GetGameInstance());
	if (!GI)
	{
		return;
	}
	const FNOARClientConfiguration C = BuildConfiguration();
	if (bDeferred) GI->SaveConfigurationDeferred(C); else GI->SaveConfiguration(C);
}

void UNOARApartmentSubsystem::RestoreSavedConfiguration()
{
	UNOARGameInstance* GI = Cast<UNOARGameInstance>(GetWorld()->GetGameInstance());
	FNOARClientConfiguration Saved;
	const bool bHas = GI && Apartment && GI->LoadConfiguration(Apartment->ApartmentID, Saved);

	UNOARConfiguratorSubsystem* Cfg = GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>();
	UNOARLightingSubsystem* Lights = GetWorld()->GetSubsystem<UNOARLightingSubsystem>();
	ANOARDayNightManager* DayNight = nullptr;
	for (TActorIterator<ANOARDayNightManager> It(GetWorld()); It; ++It) { DayNight = *It; break; }

	if (bHas)
	{
		if (Cfg)      Cfg->ApplyConfiguration(Saved.Finishes);
		if (DayNight) DayNight->SetPreset(Saved.LightingPreset, true);
		if (Lights)   Lights->ApplyGroupStates(Saved.LightGroups, false);
		if (ANOARCharacter* C = GetCharacter())
		{
			C->SetCameraMode(Saved.CameraMode);
			// position is NOT restored: the apartment always starts at the vestibule (section 10) - the room list
			// remembers LastRoom for the HUD only
		}
		if (!Saved.LastRoom.IsNone() && Apartment)
		{
			CurrentRoom = Apartment->FindRoom(Saved.LastRoom);
		}
		UE_LOG(LogNOAR, Log, TEXT("Configuration restored for %s"), *Apartment->ApartmentID.ToString());
	}
	else if (Cfg)
	{
		Cfg->ResetToDefaults();
	}
	bRestored = true;
}

void UNOARApartmentSubsystem::ResetDesign()
{
	if (UNOARConfiguratorSubsystem* Cfg = GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>())
	{
		Cfg->ResetToDefaults();
	}
	for (TActorIterator<ANOARDayNightManager> It(GetWorld()); It; ++It)
	{
		It->SetPreset(ENOARLightingPreset::Day, false);
		break;
	}
	SaveNow(false);
}

void UNOARApartmentSubsystem::Deinitialize()
{
	// final save on level exit / quit
	if (bRestored && Apartment)
	{
		if (UNOARGameInstance* GI = Cast<UNOARGameInstance>(GetWorld()->GetGameInstance()))
		{
			GI->SaveConfiguration(BuildConfiguration());
		}
	}
	Super::Deinitialize();
}
