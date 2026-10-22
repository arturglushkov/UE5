#include "Configurator/NOARConfigurableSurfaceComponent.h"
#include "Configurator/NOARConfiguratorSubsystem.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"

UNOARConfigurableSurfaceComponent::UNOARConfigurableSurfaceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNOARConfigurableSurfaceComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UNOARConfiguratorSubsystem* Sub = GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>())
	{
		Sub->RegisterSurface(this);
	}
}

void UNOARConfigurableSurfaceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		if (UNOARConfiguratorSubsystem* Sub = W->GetSubsystem<UNOARConfiguratorSubsystem>())
		{
			Sub->UnregisterSurface(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UNOARConfigurableSurfaceComponent::ApplyMaterial(UMaterialInterface* Material)
{
	if (!Material)
	{
		return;
	}
	TArray<UMeshComponent*> Meshes;
	GetOwner()->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* M : Meshes)
	{
		if (!MeshComponentName.IsNone() && M->GetFName() != MeshComponentName)
		{
			continue;
		}
		if (MaterialSlotName.IsNone())
		{
			for (int32 i = 0; i < M->GetNumMaterials(); ++i)
			{
				M->SetMaterial(i, Material);
			}
		}
		else
		{
			const int32 Index = M->GetMaterialIndex(MaterialSlotName);
			if (Index != INDEX_NONE)
			{
				M->SetMaterial(Index, Material);
			}
		}
	}
}
