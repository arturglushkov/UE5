#include "Lighting/NOARLightingSubsystem.h"
#include "Lighting/NOARInteractiveLight.h"

void UNOARLightingSubsystem::RegisterLight(ANOARInteractiveLight* Light)
{
	if (Light)
	{
		Lights.AddUnique(Light);
	}
}

void UNOARLightingSubsystem::UnregisterLight(ANOARInteractiveLight* Light)
{
	Lights.RemoveAll([Light](const TWeakObjectPtr<ANOARInteractiveLight>& P) { return !P.IsValid() || P.Get() == Light; });
}

void UNOARLightingSubsystem::ToggleGroup(FGameplayTag Group)
{
	SetGroup(Group, !IsGroupOn(Group), 1.f, true);
}

void UNOARLightingSubsystem::SetGroup(FGameplayTag Group, bool bOn, float Dimmer, bool bAnimate)
{
	for (const TWeakObjectPtr<ANOARInteractiveLight>& P : Lights)
	{
		ANOARInteractiveLight* L = P.Get();
		if (L && L->LightGroupTag.MatchesTag(Group))
		{
			L->SetDimmer(Dimmer, false);
			L->SetOn(bOn, bAnimate);
		}
	}
	OnGroupChanged.Broadcast(Group, bOn, Dimmer);
}

void UNOARLightingSubsystem::ApplyGroupStates(const TArray<FNOARLightGroupState>& States, bool bAnimate)
{
	for (const FNOARLightGroupState& S : States)
	{
		SetGroup(S.Group, S.bOn, S.Dimmer, bAnimate);
	}
}

bool UNOARLightingSubsystem::IsGroupOn(FGameplayTag Group) const
{
	for (const TWeakObjectPtr<ANOARInteractiveLight>& P : Lights)
	{
		const ANOARInteractiveLight* L = P.Get();
		if (L && L->LightGroupTag.MatchesTag(Group) && L->IsOn())
		{
			return true;
		}
	}
	return false;
}

TArray<FGameplayTag> UNOARLightingSubsystem::GetKnownGroups() const
{
	TArray<FGameplayTag> Out;
	for (const TWeakObjectPtr<ANOARInteractiveLight>& P : Lights)
	{
		if (const ANOARInteractiveLight* L = P.Get())
		{
			Out.AddUnique(L->LightGroupTag);
		}
	}
	return Out;
}

TArray<FNOARLightGroupState> UNOARLightingSubsystem::GetGroupStates() const
{
	TArray<FNOARLightGroupState> Out;
	for (const FGameplayTag& G : GetKnownGroups())
	{
		FNOARLightGroupState S;
		S.Group = G;
		S.bOn = IsGroupOn(G);
		S.Dimmer = 1.f;
		for (const TWeakObjectPtr<ANOARInteractiveLight>& P : Lights)
		{
			if (const ANOARInteractiveLight* L = P.Get(); L && L->LightGroupTag == G)
			{
				S.Dimmer = L->GetDimmer();
				break;
			}
		}
		Out.Add(S);
	}
	return Out;
}

void UNOARLightingSubsystem::SetZone(FGameplayTag Zone, bool bOn)
{
	// Zone.Kitchen -> LightGroup.Kitchen (same leaf name)
	const FString Leaf = Zone.ToString().Replace(TEXT("Zone."), TEXT(""));
	const FGameplayTag Group = FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("LightGroup.%s"), *Leaf)), false);
	if (Group.IsValid())
	{
		SetGroup(Group, bOn, 1.f, true);
	}
}
