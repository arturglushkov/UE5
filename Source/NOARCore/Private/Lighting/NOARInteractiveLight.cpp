#include "Lighting/NOARInteractiveLight.h"
#include "Lighting/NOARLightingSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

ANOARInteractiveLight::ANOARInteractiveLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FixtureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FixtureMesh"));
	FixtureMesh->SetupAttachment(Root);
	FixtureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StripMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StripMesh"));
	StripMesh->SetupAttachment(Root);
	StripMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StripMesh->SetCastShadow(false);
}

void ANOARInteractiveLight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPhysicalUnits();
}

void ANOARInteractiveLight::ApplyPhysicalUnits()
{
	LightComponents.Reset();
	TArray<ULocalLightComponent*> Found;
	GetComponents<ULocalLightComponent>(Found);
	for (ULocalLightComponent* L : Found)
	{
		LightComponents.Add(L);
	}
	const int32 N = FMath::Max(1, LightComponents.Num());
	for (ULocalLightComponent* L : LightComponents)
	{
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(Lumens / N);
		L->SetUseTemperature(true);
		L->SetTemperature(Kelvin);
	}
}

void ANOARInteractiveLight::BeginPlay()
{
	Super::BeginPlay();
	ApplyPhysicalUnits();

	if (StripMesh && StripMesh->GetStaticMesh() && StripMesh->GetNumMaterials() > 0)
	{
		StripMID = StripMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	bIsOn = bStartOn;
	CurrentLevel = TargetLevel = bIsOn ? Dimmer : 0.f;
	ApplyLevel(CurrentLevel);

	if (UNOARLightingSubsystem* Sub = GetWorld()->GetSubsystem<UNOARLightingSubsystem>())
	{
		Sub->RegisterLight(this);
	}
}

void ANOARInteractiveLight::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		if (UNOARLightingSubsystem* Sub = W->GetSubsystem<UNOARLightingSubsystem>())
		{
			Sub->UnregisterLight(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ANOARInteractiveLight::SetOn(bool bOn, bool bAnimate)
{
	bIsOn = bOn;
	TargetLevel = bOn ? Dimmer : 0.f;
	if (!bAnimate)
	{
		CurrentLevel = TargetLevel;
		ApplyLevel(CurrentLevel);
		SetActorTickEnabled(false);
	}
	else
	{
		SetActorTickEnabled(true);
	}
}

void ANOARInteractiveLight::SetDimmer(float InDimmer, bool bAnimate)
{
	Dimmer = FMath::Clamp(InDimmer, 0.f, 1.f);
	SetOn(bIsOn, bAnimate);
}

void ANOARInteractiveLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Step = DeltaSeconds / FMath::Max(0.05f, RampSeconds);
	CurrentLevel = FMath::FInterpConstantTo(CurrentLevel, TargetLevel, 1.f, Step);
	ApplyLevel(CurrentLevel);
	if (FMath::IsNearlyEqual(CurrentLevel, TargetLevel, 0.001f))
	{
		CurrentLevel = TargetLevel;
		ApplyLevel(CurrentLevel);
		SetActorTickEnabled(false);
	}
}

void ANOARInteractiveLight::ApplyLevel(float Level)
{
	const int32 N = FMath::Max(1, LightComponents.Num());
	for (ULocalLightComponent* L : LightComponents)
	{
		if (L)
		{
			L->SetIntensity(Lumens / N * Level);
			L->SetVisibility(Level > 0.001f);
		}
	}
	if (StripMID)
	{
		// EmissiveNits -> material emissive scalar. Material is authored so that Emissive=1 == 200 nit at EV100 10.
		StripMID->SetScalarParameterValue(EmissiveParameterName, (EmissiveNits / 200.f) * Level);
	}
}
