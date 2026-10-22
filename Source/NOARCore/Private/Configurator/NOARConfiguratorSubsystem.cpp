#include "Configurator/NOARConfiguratorSubsystem.h"
#include "Configurator/NOARConfigurableSurfaceComponent.h"
#include "Core/NOARApartmentSubsystem.h"
#include "NOARCore.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"

void UNOARConfiguratorSubsystem::RegisterSurface(UNOARConfigurableSurfaceComponent* Surface)
{
	if (!Surface)
	{
		return;
	}
	Surfaces.AddUnique(Surface);
	// late joiners (streamed actors) get the current selection immediately
	if (const FName* Opt = Current.Find(Surface->SlotTag))
	{
		if (const FNOARFinishOptionRow* Row = FindRow(Surface->SlotTag, *Opt))
		{
			Surface->ApplyMaterial(Row->Material.LoadSynchronous());
		}
	}
}

void UNOARConfiguratorSubsystem::UnregisterSurface(UNOARConfigurableSurfaceComponent* Surface)
{
	Surfaces.RemoveAll([Surface](const TWeakObjectPtr<UNOARConfigurableSurfaceComponent>& P) { return !P.IsValid() || P.Get() == Surface; });
}

void UNOARConfiguratorSubsystem::SetOptionsTable(UDataTable* Table)
{
	Options = Table;
	Current.Reset();
	if (!Options)
	{
		return;
	}
	// defaults
	for (const TPair<FName, uint8*>& Pair : Options->GetRowMap())
	{
		const FNOARFinishOptionRow* Row = reinterpret_cast<const FNOARFinishOptionRow*>(Pair.Value);
		if (Row && Row->bIsDefault && !Current.Contains(Row->Slot))
		{
			Current.Add(Row->Slot, Row->OptionID);
		}
	}
}

const FNOARFinishOptionRow* UNOARConfiguratorSubsystem::FindRow(FGameplayTag Slot, FName OptionID) const
{
	if (!Options)
	{
		return nullptr;
	}
	for (const TPair<FName, uint8*>& Pair : Options->GetRowMap())
	{
		const FNOARFinishOptionRow* Row = reinterpret_cast<const FNOARFinishOptionRow*>(Pair.Value);
		if (Row && Row->Slot == Slot && Row->OptionID == OptionID)
		{
			return Row;
		}
	}
	return nullptr;
}

bool UNOARConfiguratorSubsystem::ApplyOption(FGameplayTag Slot, FName OptionID, bool bSave)
{
	const FNOARFinishOptionRow* Row = FindRow(Slot, OptionID);
	if (!Row)
	{
		UE_LOG(LogNOAR, Warning, TEXT("Configurator: option %s not found for %s"), *OptionID.ToString(), *Slot.ToString());
		return false;
	}
	UMaterialInterface* Mat = Row->Material.LoadSynchronous();
	if (!Mat)
	{
		UE_LOG(LogNOAR, Warning, TEXT("Configurator: material missing for %s"), *OptionID.ToString());
		return false;
	}
	for (const TWeakObjectPtr<UNOARConfigurableSurfaceComponent>& P : Surfaces)
	{
		if (UNOARConfigurableSurfaceComponent* S = P.Get(); S && S->SlotTag == Slot)
		{
			S->ApplyMaterial(Mat);
		}
	}
	Current.Add(Slot, OptionID);
	OnConfigChanged.Broadcast(Slot, OptionID);
	if (bSave)
	{
		SaveDeferred();
	}
	return true;
}

void UNOARConfiguratorSubsystem::ApplyConfiguration(const TMap<FGameplayTag, FName>& Finishes)
{
	for (const TPair<FGameplayTag, FName>& Pair : Finishes)
	{
		ApplyOption(Pair.Key, Pair.Value, false);
	}
}

void UNOARConfiguratorSubsystem::ResetToDefaults()
{
	if (!Options)
	{
		return;
	}
	for (const TPair<FName, uint8*>& Pair : Options->GetRowMap())
	{
		const FNOARFinishOptionRow* Row = reinterpret_cast<const FNOARFinishOptionRow*>(Pair.Value);
		if (Row && Row->bIsDefault)
		{
			ApplyOption(Row->Slot, Row->OptionID, false);
		}
	}
	SaveDeferred();
}

TArray<FGameplayTag> UNOARConfiguratorSubsystem::GetSlots() const
{
	TArray<FGameplayTag> Out;
	if (Options)
	{
		for (const TPair<FName, uint8*>& Pair : Options->GetRowMap())
		{
			const FNOARFinishOptionRow* Row = reinterpret_cast<const FNOARFinishOptionRow*>(Pair.Value);
			if (Row)
			{
				Out.AddUnique(Row->Slot);
			}
		}
	}
	return Out;
}

TArray<FNOARFinishOptionRow> UNOARConfiguratorSubsystem::GetOptionsForSlot(FGameplayTag Slot) const
{
	TArray<FNOARFinishOptionRow> Out;
	if (Options)
	{
		for (const TPair<FName, uint8*>& Pair : Options->GetRowMap())
		{
			const FNOARFinishOptionRow* Row = reinterpret_cast<const FNOARFinishOptionRow*>(Pair.Value);
			if (Row && Row->Slot == Slot)
			{
				Out.Add(*Row);
			}
		}
	}
	return Out;
}

FName UNOARConfiguratorSubsystem::GetCurrentOption(FGameplayTag Slot) const
{
	const FName* F = Current.Find(Slot);
	return F ? *F : NAME_None;
}

void UNOARConfiguratorSubsystem::PreloadAllMaterials()
{
	PreloadedMaterials.Reset();
	if (!Options)
	{
		return;
	}
	for (const TPair<FName, uint8*>& Pair : Options->GetRowMap())
	{
		const FNOARFinishOptionRow* Row = reinterpret_cast<const FNOARFinishOptionRow*>(Pair.Value);
		if (Row)
		{
			if (UMaterialInterface* M = Row->Material.LoadSynchronous())
			{
				PreloadedMaterials.Add(M);
			}
		}
	}
	UE_LOG(LogNOAR, Log, TEXT("Configurator: %d option materials preloaded"), PreloadedMaterials.Num());
}

float UNOARConfiguratorSubsystem::GetCurrentPriceTotal() const
{
	float Total = 0.f;
	for (const TPair<FGameplayTag, FName>& Pair : Current)
	{
		if (const FNOARFinishOptionRow* Row = FindRow(Pair.Key, Pair.Value))
		{
			Total += Row->Price;
		}
	}
	return Total;
}

void UNOARConfiguratorSubsystem::SaveDeferred()
{
	if (UNOARApartmentSubsystem* Apt = GetWorld()->GetSubsystem<UNOARApartmentSubsystem>())
	{
		Apt->SaveNow(true);
	}
}
