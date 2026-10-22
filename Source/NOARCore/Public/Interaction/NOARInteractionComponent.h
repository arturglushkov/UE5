#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NOARTypes.h"
#include "NOARInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FNOAROnFocusChanged, AActor*, FocusedActor, const FText&, Prompt, FVector, WorldPoint, ENOARInteractDenyReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNOAROnMoveToRequested, FVector, WorldLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNOAROnInteracted, AActor*, Actor);

/**
 * BPC_Interaction (Standard v1.1, section 8). Lives on ANOARCharacter.
 *
 * Every TraceInterval seconds:
 *   trace (cursor in Mouse3P, screen centre otherwise) on channel Interactable
 *   -> distance from capsule to interaction point <= MaxInteractionDistance
 *   -> occlusion trace from chest to interaction point on Visibility
 *   -> CanInteract (locked / dependency / blocked)
 *   -> Focus / Unfocus, OnFocusChanged for the HUD
 * TryInteract(): Interact on the focused actor; out of range in Mouse3P -> OnMoveToRequested (click-to-walk);
 *                out of range in FirstPerson / Gamepad -> prompt "Move closer".
 */
UCLASS(ClassGroup = (NOAR), meta = (BlueprintSpawnableComponent))
class NOARCORE_API UNOARInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNOARInteractionComponent();

	/** Trace channel used for interactables (ECC_GameTraceChannel1 = "Interactable" in DefaultEngine.ini) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	TEnumAsByte<ECollisionChannel> InteractableChannel = ECC_GameTraceChannel1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	ENOARInputMode InputMode = ENOARInputMode::Mouse3P;

	/** Overrides UNOARSettings when > 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	float MaxInteractionDistanceOverride = 0.f;

	UPROPERTY(BlueprintAssignable, Category = "NOAR|Interaction")
	FNOAROnFocusChanged OnFocusChanged;

	UPROPERTY(BlueprintAssignable, Category = "NOAR|Interaction")
	FNOAROnMoveToRequested OnMoveToRequested;

	/** Mouse3P: click on a walkable surface with nothing focused -> walk exactly there */
	UPROPERTY(BlueprintAssignable, Category = "NOAR|Interaction")
	FNOAROnMoveToRequested OnWalkToRequested;

	UPROPERTY(BlueprintAssignable, Category = "NOAR|Interaction")
	FNOAROnInteracted OnInteracted;

	UFUNCTION(BlueprintCallable, Category = "NOAR|Interaction")
	void TryInteract();

	/** Product card request (Alt+click / "i") */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Interaction")
	bool GetFocusedProductInfo(FNOARProductInfo& OutInfo) const;

	UFUNCTION(BlueprintPure, Category = "NOAR|Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "NOAR|Interaction")
	bool IsFocusedInRange() const { return bFocusedInRange; }

	UFUNCTION(BlueprintCallable, Category = "NOAR|Interaction")
	void SetEnabled(bool bEnabled);

	/** Called by the character when a click-to-walk arrives at its target */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Interaction")
	void NotifyArrivedAtPendingTarget();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void TraceTick();
	bool DoTrace(FHitResult& OutHit) const;
	bool IsOccluded(AActor* Target, const FVector& Point) const;
	void SetFocus(AActor* NewActor, bool bInRange, ENOARInteractDenyReason Reason);
	float GetMaxDistance() const;

	FTimerHandle TraceTimer;
	TWeakObjectPtr<AActor> FocusedActor;
	TWeakObjectPtr<AActor> PendingTarget;
	bool bFocusedInRange = false;
	bool bEnabled = true;
	ENOARInteractDenyReason LastReason = ENOARInteractDenyReason::OK;
};
