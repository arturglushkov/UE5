#include "Lighting/NOARDayNightManager.h"
#include "Lighting/NOARLightingSubsystem.h"
#include "Data/NOARDataAssets.h"
#include "NOARCore.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "TimerManager.h"

ANOARDayNightManager::ANOARDayNightManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	From = MakeShared<FNOARSkyState>();
	To = MakeShared<FNOARSkyState>();
}

UNOARLightPreset* ANOARDayNightManager::Find(ENOARLightingPreset P) const
{
	for (UNOARLightPreset* Preset : Presets)
	{
		if (Preset && Preset->Preset == P)
		{
			return Preset;
		}
	}
	return nullptr;
}

void ANOARDayNightManager::BeginPlay()
{
	Super::BeginPlay();

	if (Backdrop)
	{
		if (UMeshComponent* Mesh = Backdrop->FindComponentByClass<UMeshComponent>())
		{
			if (Mesh->GetNumMaterials() > 0)
			{
				BackdropMID = Mesh->CreateAndSetMaterialInstanceDynamic(0);
			}
		}
	}
	SetPreset(StartPreset, true);
}

void ANOARDayNightManager::Capture(UNOARLightPreset* P, FNOARSkyState& Out) const
{
	if (!P)
	{
		return;
	}
	Out.SunLux = P->SunLux;
	Out.SunElevation = P->SunElevation;
	Out.SunAzimuth = P->SunAzimuth;
	Out.SunKelvin = P->SunKelvin;
	Out.SkyIntensity = P->SkyLightIntensity;
	Out.SkyTint = P->SkyLightTint;
	Out.ExposureComp = P->ExposureCompensation;
	Out.WhiteBalance = P->WhiteBalanceKelvin;
	Out.BackdropIndex = P->BackdropIndex;
}

void ANOARDayNightManager::SetPreset(ENOARLightingPreset Preset, bool bInstant)
{
	UNOARLightPreset* P = Find(Preset);
	if (!P)
	{
		UE_LOG(LogNOAR, Warning, TEXT("DayNightManager: preset %d not assigned"), (int32)Preset);
		return;
	}

	// start from the live state so a mid-blend switch does not jump
	if (BlendT < 1.f)
	{
		const float T = BlendT;
		FNOARSkyState Live;
		Live.SunLux = FMath::Lerp(From->SunLux, To->SunLux, T);
		Live.SunElevation = FMath::Lerp(From->SunElevation, To->SunElevation, T);
		Live.SunAzimuth = FMath::Lerp(From->SunAzimuth, To->SunAzimuth, T);
		Live.SunKelvin = FMath::Lerp(From->SunKelvin, To->SunKelvin, T);
		Live.SkyIntensity = FMath::Lerp(From->SkyIntensity, To->SkyIntensity, T);
		Live.SkyTint = FMath::Lerp(From->SkyTint, To->SkyTint, T);
		Live.ExposureComp = FMath::Lerp(From->ExposureComp, To->ExposureComp, T);
		Live.WhiteBalance = FMath::Lerp(From->WhiteBalance, To->WhiteBalance, T);
		Live.BackdropIndex = From->BackdropIndex;
		*From = Live;
	}
	else
	{
		*From = *To;
	}
	Capture(P, *To);
	Current = Preset;

	if (bInstant)
	{
		BlendT = 1.f;
		Apply(*To);
		ApplyInterior();
		SetActorTickEnabled(false);
	}
	else
	{
		BlendT = 0.f;
		SetActorTickEnabled(true);
		GetWorld()->GetTimerManager().ClearTimer(InteriorTimer);
		if (P->InteriorDelay > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(InteriorTimer, this, &ANOARDayNightManager::ApplyInterior, P->InteriorDelay, false);
		}
		else
		{
			ApplyInterior();
		}
	}
	OnPresetChanged.Broadcast(Current);
}

void ANOARDayNightManager::CyclePreset()
{
	switch (Current)
	{
	case ENOARLightingPreset::Day:     SetPreset(ENOARLightingPreset::Evening); break;
	case ENOARLightingPreset::Evening: SetPreset(ENOARLightingPreset::Night);   break;
	default:                           SetPreset(ENOARLightingPreset::Day);     break;
	}
}

void ANOARDayNightManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BlendT = FMath::Min(1.f, BlendT + DeltaSeconds / FMath::Max(0.1f, BlendSeconds));
	const float T = FMath::InterpEaseInOut(0.f, 1.f, BlendT, 2.f);

	FNOARSkyState S;
	S.SunLux = FMath::Lerp(From->SunLux, To->SunLux, T);
	S.SunElevation = FMath::Lerp(From->SunElevation, To->SunElevation, T);
	S.SunAzimuth = FMath::Lerp(From->SunAzimuth, To->SunAzimuth, T);
	S.SunKelvin = FMath::Lerp(From->SunKelvin, To->SunKelvin, T);
	S.SkyIntensity = FMath::Lerp(From->SkyIntensity, To->SkyIntensity, T);
	S.SkyTint = FMath::Lerp(From->SkyTint, To->SkyTint, T);
	S.ExposureComp = FMath::Lerp(From->ExposureComp, To->ExposureComp, T);
	S.WhiteBalance = FMath::Lerp(From->WhiteBalance, To->WhiteBalance, T);
	S.BackdropIndex = To->BackdropIndex;
	Apply(S);

	if (BackdropMID)
	{
		BackdropMID->SetScalarParameterValue(TEXT("Blend"), T);
	}
	if (BlendT >= 1.f)
	{
		SetActorTickEnabled(false);
		if (BackdropMID && BackdropHDRIs.IsValidIndex(To->BackdropIndex))
		{
			// settle: A = B, blend back to 0 so the next transition starts clean
			BackdropMID->SetTextureParameterValue(TEXT("HDRI_A"), BackdropHDRIs[To->BackdropIndex]);
			BackdropMID->SetScalarParameterValue(TEXT("Blend"), 0.f);
		}
	}
}

void ANOARDayNightManager::Apply(const FNOARSkyState& S)
{
	if (Sun)
	{
		if (UDirectionalLightComponent* C = Sun->GetComponent())
		{
			C->SetIntensity(S.SunLux);
			C->SetUseTemperature(true);
			C->SetTemperature(S.SunKelvin);
			C->SetVisibility(S.SunLux > 1.f);
		}
		Sun->SetActorRotation(FRotator(-S.SunElevation, S.SunAzimuth, 0.f));
	}
	if (SkyLight)
	{
		if (USkyLightComponent* C = SkyLight->GetLightComponent())
		{
			C->SetIntensity(S.SkyIntensity);
			C->SetLightColor(S.SkyTint);
		}
	}
	if (PostProcess)
	{
		PostProcess->Settings.bOverride_AutoExposureBias = true;
		PostProcess->Settings.AutoExposureBias = S.ExposureComp;
		PostProcess->Settings.bOverride_WhiteTemp = true;
		PostProcess->Settings.WhiteTemp = S.WhiteBalance;
	}
	if (BackdropMID && BlendT <= 0.f + KINDA_SMALL_NUMBER)
	{
		if (BackdropHDRIs.IsValidIndex(From->BackdropIndex))
		{
			BackdropMID->SetTextureParameterValue(TEXT("HDRI_A"), BackdropHDRIs[From->BackdropIndex]);
		}
		if (BackdropHDRIs.IsValidIndex(To->BackdropIndex))
		{
			BackdropMID->SetTextureParameterValue(TEXT("HDRI_B"), BackdropHDRIs[To->BackdropIndex]);
		}
	}
}

void ANOARDayNightManager::ApplyInterior()
{
	UNOARLightPreset* P = Find(Current);
	UNOARLightingSubsystem* Sub = GetWorld()->GetSubsystem<UNOARLightingSubsystem>();
	if (P && Sub)
	{
		Sub->ApplyGroupStates(P->LightGroups, true);
	}
}
