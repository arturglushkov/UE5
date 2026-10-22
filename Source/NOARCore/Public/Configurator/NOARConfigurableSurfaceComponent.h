#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NOARConfigurableSurfaceComponent.generated.h"

/**
 * BPC_ConfigurableSurface (section 10): add to any actor whose mesh material slot the client can change.
 * SlotTag = Configurable.* ; MaterialSlotName = slot on the mesh ("FRONT", "TOP", "PANEL"...); empty = all slots.
 * The configurator subsystem builds its registry from these on BeginPlay.
 */
UCLASS(ClassGroup = (NOAR), meta = (BlueprintSpawnableComponent))
class NOARCORE_API UNOARConfigurableSurfaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNOARConfigurableSurfaceComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Configurator", meta = (Categories = "Configurable"))
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Configurator")
	FName MaterialSlotName = TEXT("FRONT");

	/** Apply to every UMeshComponent on the owner (default) or only the named component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Configurator")
	FName MeshComponentName = NAME_None;

	UFUNCTION(BlueprintCallable, Category = "NOAR|Configurator")
	void ApplyMaterial(UMaterialInterface* Material);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
