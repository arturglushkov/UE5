#include "NOARFunctionLibrary.h"
#include "NOARSettings.h"
#include "NOARCore.h"
#include "Misc/EngineVersion.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "RHI.h"
#include "RenderUtils.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"

const UNOARSettings* UNOARFunctionLibrary::GetNOARSettings()
{
	return GetDefault<UNOARSettings>();
}

bool UNOARFunctionLibrary::IsShippingBuild()
{
#if UE_BUILD_SHIPPING
	return true;
#else
	return false;
#endif
}

bool UNOARFunctionLibrary::IsDebugMenuAllowed()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return true;
#endif
}

FString UNOARFunctionLibrary::GetEngineVersionString()
{
	return FEngineVersion::Current().ToString(EVersionComponent::Patch);
}

static void SetScalabilityLevelAll(int32 Level)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetViewDistanceQuality(Level);
		Settings->SetAntiAliasingQuality(Level);
		Settings->SetShadowQuality(Level);
		Settings->SetGlobalIlluminationQuality(Level);
		Settings->SetReflectionQuality(Level);
		Settings->SetPostProcessingQuality(Level);
		Settings->SetTextureQuality(Level);
		Settings->SetVisualEffectQuality(Level);
		Settings->SetFoliageQuality(Level);
		Settings->SetShadingQuality(Level);
		Settings->ApplySettings(false);
	}
}

void UNOARFunctionLibrary::ApplyQualityTier(ENOARQualityTier Tier, bool bHardwareRayTracing)
{
	const int32 Level = FMath::Clamp(static_cast<int32>(Tier), 1, 4);
	SetScalabilityLevelAll(Level);

	// Resolution: LOW/MEDIUM upscale through TSR, HIGH/ULTRA native (section 13)
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const float ResQuality = (Tier == ENOARQualityTier::Low) ? 58.f : (Tier == ENOARQualityTier::Medium) ? 67.f : 100.f;
		Settings->SetResolutionScaleValueEx(ResQuality);
		Settings->SetFrameRateLimit(Level >= 3 ? 60.f : 30.f);
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}

	// HWRT only on ULTRA and only when the RHI has it (Windows RTX; experimental Apple Silicon is not offered in the UI)
	const bool bUseHWRT = bHardwareRayTracing && Tier == ENOARQualityTier::Ultra && IsHardwareRayTracingAvailable() && !IsMac();
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing")))
	{
		CVar->Set(bUseHWRT ? 1 : 0, ECVF_SetByGameSetting);
	}
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Reflections.HardwareRayTracing")))
	{
		CVar->Set(bUseHWRT ? 1 : 0, ECVF_SetByGameSetting);
	}
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing.LightingMode")))
	{
		CVar->Set(bUseHWRT ? 2 : 0, ECVF_SetByGameSetting); // 2 = hit lighting for exact mirrors
	}

	UE_LOG(LogNOAR, Log, TEXT("Quality tier applied: level %d, HWRT=%d"), Level, bUseHWRT ? 1 : 0);
}

void UNOARFunctionLibrary::ApplyPhotoModeOverrides(bool bEnable, ENOARQualityTier RestoreTier)
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	if (bEnable)
	{
		Settings->SetReflectionQuality(4);
		Settings->SetShadowQuality(4);
		Settings->SetGlobalIlluminationQuality(FMath::Max(3, static_cast<int32>(RestoreTier)));
		Settings->SetResolutionScaleValueEx(100.f);
	}
	else
	{
		const int32 Level = static_cast<int32>(RestoreTier);
		Settings->SetReflectionQuality(Level);
		Settings->SetShadowQuality(Level);
		Settings->SetGlobalIlluminationQuality(Level);
		const float ResQuality = (RestoreTier == ENOARQualityTier::Low) ? 58.f : (RestoreTier == ENOARQualityTier::Medium) ? 67.f : 100.f;
		Settings->SetResolutionScaleValueEx(ResQuality);
	}
	Settings->ApplySettings(false);

	// Motion blur off while composing a still
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))
	{
		CVar->Set(bEnable ? 0 : 4, ECVF_SetByGameSetting);
	}
}

bool UNOARFunctionLibrary::IsHardwareRayTracingAvailable()
{
	return GRHISupportsRayTracing && IsRayTracingEnabled();
}

bool UNOARFunctionLibrary::IsMac()
{
#if PLATFORM_MAC
	return true;
#else
	return false;
#endif
}

ENOARQualityTier UNOARFunctionLibrary::TierFromGPUIndex(float GPUIndex)
{
	if (GPUIndex < 150.f) return ENOARQualityTier::Low;
	if (GPUIndex < 300.f) return ENOARQualityTier::Medium;
	if (GPUIndex < 550.f) return ENOARQualityTier::High;
	return ENOARQualityTier::Ultra;
}

void UNOARFunctionLibrary::AddKeyMapping(UInputMappingContext* Context, UInputAction* Action, FName KeyName, bool bSwizzleYX, bool bNegate, float DeadZone, float Scale)
{
	if (!Context || !Action)
	{
		return;
	}
	const FKey Key(KeyName);
	if (!Key.IsValid())
	{
		UE_LOG(LogNOAR, Warning, TEXT("AddKeyMapping: unknown key %s"), *KeyName.ToString());
		return;
	}
	Context->Modify();
	FEnhancedActionKeyMapping& M = Context->MapKey(Action, Key);
	if (DeadZone > 0.f)
	{
		UInputModifierDeadZone* DZ = NewObject<UInputModifierDeadZone>(Context);
		DZ->LowerThreshold = DeadZone;
		M.Modifiers.Add(DZ);
	}
	if (bSwizzleYX)
	{
		UInputModifierSwizzleAxis* Sw = NewObject<UInputModifierSwizzleAxis>(Context);
		Sw->Order = EInputAxisSwizzle::YXZ;
		M.Modifiers.Add(Sw);
	}
	if (bNegate)
	{
		M.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	}
	if (!FMath::IsNearlyEqual(Scale, 1.f))
	{
		UInputModifierScalar* Sc = NewObject<UInputModifierScalar>(Context);
		Sc->Scalar = FVector(Scale, Scale, 1.f);
		M.Modifiers.Add(Sc);
	}
	Context->MarkPackageDirty();
}

void UNOARFunctionLibrary::ClearKeyMappings(UInputMappingContext* Context)
{
	if (Context)
	{
		Context->Modify();
		Context->UnmapAll();
		Context->MarkPackageDirty();
	}
}
