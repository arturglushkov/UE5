#pragma once

#include "CoreMinimal.h"
#include "Interaction/NOARInteractiveBase.h"
#include "NOARLightSwitch.generated.h"

/**
 * BP_LightSwitch: rocker tilts 8 degrees over 0.1 s and toggles one or more light groups through the subsystem.
 * MovingMesh = rocker, Pivot on its hinge. Root = plate.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARLightSwitch : public ANOARInteractiveBase
{
	GENERATED_BODY()

public:
	ANOARLightSwitch();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Switch", meta = (Categories = "LightGroup"))
	TArray<FGameplayTag> LightGroupTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Switch")
	float RockerAngle = 8.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	virtual void Interact_Implementation(AActor* Instigator) override;
	virtual FText GetInteractionText_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void ApplyAlpha_Implementation(float Alpha) override;
};
