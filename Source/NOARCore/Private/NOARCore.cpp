#include "NOARCore.h"
#include "NOARGameplayTags.h"

DEFINE_LOG_CATEGORY(LogNOAR);

void FNOARCoreModule::StartupModule()
{
	UE_LOG(LogNOAR, Log, TEXT("NOARCore module started (UE %s)"), *FEngineVersion::Current().ToString());
}

void FNOARCoreModule::ShutdownModule()
{
}

IMPLEMENT_PRIMARY_GAME_MODULE(FNOARCoreModule, NOARCore, "NOAR");
