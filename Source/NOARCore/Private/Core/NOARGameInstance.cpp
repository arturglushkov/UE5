#include "Core/NOARGameInstance.h"
#include "Data/NOARDataAssets.h"
#include "NOARFunctionLibrary.h"
#include "NOARSettings.h"
#include "NOARCore.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Internationalization.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"

void UNOARGameInstance::Init()
{
	Super::Init();

	Settings = Cast<UNOARSettingsSave>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName(), 0));
	if (!Settings || Settings->SchemaVersion != 1)
	{
		Settings = Cast<UNOARSettingsSave>(UGameplayStatics::CreateSaveGameObject(UNOARSettingsSave::StaticClass()));
		Settings->QualityTier = GetDefault<UNOARSettings>()->DefaultQualityTier;
	}
	if (!Settings->Language.IsEmpty())
	{
		FInternationalization::Get().SetCurrentCulture(Settings->Language);
	}
}

// ---------------- apartment ----------------

void UNOARGameInstance::SelectApartment(UNOARApartmentData* Apartment)
{
	SelectedApartment = Apartment;
	if (Apartment)
	{
		Settings->LastApartment = Apartment->ApartmentID;
		SaveSettings();
	}
}

void UNOARGameInstance::OpenSelectedApartment()
{
	if (!SelectedApartment || SelectedApartment->Level.IsNull())
	{
		UE_LOG(LogNOAR, Error, TEXT("OpenSelectedApartment: no apartment / level"));
		return;
	}
	// Soft world path -> level name
	const FString LongName = SelectedApartment->Level.GetLongPackageName();
	UGameplayStatics::OpenLevel(this, FName(*LongName));
}

// ---------------- configuration ----------------

bool UNOARGameInstance::LoadConfiguration(FName ApartmentID, FNOARClientConfiguration& Out) const
{
	const FString Slot = ConfigSlotName(ApartmentID);
	if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		return false;
	}
	const UNOARSaveGame* SG = Cast<UNOARSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!SG || SG->Configuration.SchemaVersion != FNOARClientConfiguration().SchemaVersion)
	{
		UE_LOG(LogNOAR, Warning, TEXT("Save %s has old schema - ignoring (defaults will be used)"), *Slot);
		return false;
	}
	Out = SG->Configuration;
	return true;
}

void UNOARGameInstance::SaveConfiguration(const FNOARClientConfiguration& Config)
{
	UNOARSaveGame* SG = Cast<UNOARSaveGame>(UGameplayStatics::CreateSaveGameObject(UNOARSaveGame::StaticClass()));
	SG->Configuration = Config;
	UGameplayStatics::AsyncSaveGameToSlot(SG, ConfigSlotName(Config.ApartmentID), 0);
}

void UNOARGameInstance::SaveConfigurationDeferred(const FNOARClientConfiguration& Config)
{
	PendingConfig = Config;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(SaveTimer, this, &UNOARGameInstance::FlushDeferredSave, 1.0f, false);
	}
	else
	{
		FlushDeferredSave();
	}
}

void UNOARGameInstance::FlushDeferredSave()
{
	SaveConfiguration(PendingConfig);
}

void UNOARGameInstance::DeleteConfiguration(FName ApartmentID)
{
	UGameplayStatics::DeleteGameInSlot(ConfigSlotName(ApartmentID), 0);
}

// ---------------- settings ----------------

void UNOARGameInstance::SaveSettings()
{
	if (Settings)
	{
		UGameplayStatics::SaveGameToSlot(Settings, SettingsSlotName(), 0);
	}
}

ENOARQualityTier UNOARGameInstance::GetQualityTier() const
{
	return Settings ? Settings->QualityTier : ENOARQualityTier::High;
}

void UNOARGameInstance::SetQualityTier(ENOARQualityTier Tier, bool bHardwareRayTracing)
{
	Settings->QualityTier = Tier;
	Settings->bHardwareRayTracing = bHardwareRayTracing;
	UNOARFunctionLibrary::ApplyQualityTier(Tier, bHardwareRayTracing);
	SaveSettings();
	OnQualityChanged.Broadcast(Tier);
}

void UNOARGameInstance::AutoDetectQualityIfNeeded()
{
	if (Settings->bBenchmarkDone)
	{
		UNOARFunctionLibrary::ApplyQualityTier(Settings->QualityTier, Settings->bHardwareRayTracing);
		return;
	}
	// UGameUserSettings wraps the synth benchmark with an exported API (5.x)
	float GPUIndex = 300.f;
	if (UGameUserSettings* GUS = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		GUS->RunHardwareBenchmark(10, 1.f, 1.f);
		GPUIndex = GUS->GetLastGPUBenchmarkResult();
	}
	ENOARQualityTier Tier = UNOARFunctionLibrary::TierFromGPUIndex(GPUIndex);
	if (UNOARFunctionLibrary::IsMac() && static_cast<int32>(Tier) > static_cast<int32>(ENOARQualityTier::Medium))
	{
		Tier = ENOARQualityTier::Medium; // Software Lumen baseline on Mac; HIGH is opt-in from Settings
	}
	Settings->bBenchmarkDone = true;
	UE_LOG(LogNOAR, Log, TEXT("Synth benchmark GPU index %.0f -> tier %d"), GPUIndex, (int32)Tier);
	SetQualityTier(Tier, false);
}

void UNOARGameInstance::SetLanguage(const FString& Culture)
{
	Settings->Language = Culture;
	FInternationalization::Get().SetCurrentCulture(Culture);
	SaveSettings();
}
