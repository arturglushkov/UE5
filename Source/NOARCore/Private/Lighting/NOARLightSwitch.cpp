#include "Lighting/NOARLightSwitch.h"
#include "Lighting/NOARLightingSubsystem.h"
#include "NOARGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ANOARLightSwitch::ANOARLightSwitch()
{
	TypeTag = NOARTags::Interact_Type_Switch;
	OpenDuration = 0.1f;
	CloseDuration = 0.1f;
	SoundVolume = 0.25f;

	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(Root);
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlateMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlateMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

void ANOARLightSwitch::BeginPlay()
{
	Super::BeginPlay();
	// rocker position mirrors the real state of the first group
	if (UNOARLightingSubsystem* Sub = GetWorld()->GetSubsystem<UNOARLightingSubsystem>())
	{
		if (LightGroupTags.Num() > 0)
		{
			SetOpen(Sub->IsGroupOn(LightGroupTags[0]), true);
		}
	}
}

void ANOARLightSwitch::ApplyAlpha_Implementation(float Alpha)
{
	Pivot->SetRelativeRotation(FRotator(FMath::Lerp(-RockerAngle, RockerAngle, Alpha), 0.f, 0.f));
}

void ANOARLightSwitch::Interact_Implementation(AActor* Instigator)
{
	Super::Interact_Implementation(Instigator); // rocker animation
	if (UNOARLightingSubsystem* Sub = GetWorld()->GetSubsystem<UNOARLightingSubsystem>())
	{
		const bool bTurnOn = (GetState() == ENOARInteractState::Opening || GetState() == ENOARInteractState::Open);
		for (const FGameplayTag& G : LightGroupTags)
		{
			Sub->SetGroup(G, bTurnOn, 1.f, true);
		}
	}
}

FText ANOARLightSwitch::GetInteractionText_Implementation() const
{
	return NSLOCTEXT("NOAR", "Lights", "Lights");
}
