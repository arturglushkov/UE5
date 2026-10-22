#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "GameplayTagContainer.h"
#include "NOARTypes.h"
#include "Interaction/NOARInteractable.h"
#include "NOARInteractiveBase.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UCurveFloat;
class USoundBase;
class UNOARProductData;
class ANOARInteractiveLight;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNOAROnStateChanged, ANOARInteractiveBase*, Actor, ENOARInteractState, NewState);

/**
 * BP_InteractiveBase (Standard v1.1, section 9).
 *
 * Hierarchy:  Root (static part / reference)  ->  Pivot (SceneComponent at the mechanism axis)  ->  MovingMesh
 * Subclasses only override ApplyAlpha(0..1) to turn timeline alpha into a transform of Pivot.
 *
 * Interact -> CanInteract? -> switch State
 *   Closed  -> Opening (Play)      Open    -> Closing (Reverse)
 *   Opening -> Closing (Reverse)   Closing -> Opening (Play)
 * Timeline Update -> ApplyAlpha ; Finished -> Open/Closed, SoftCloseEnd sound, OnStateChanged (LED, dependencies, save)
 */
UCLASS(Abstract, Blueprintable)
class NOARCORE_API ANOARInteractiveBase : public AActor, public INOARInteractable
{
	GENERATED_BODY()

public:
	ANOARInteractiveBase();

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<USceneComponent> Root;

	/** Placed on the mechanism axis. Meshes under it are exported with the pivot at this point. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UStaticMeshComponent> MovingMesh;

	// ---- Interaction ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	bool bInteractionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction", meta = (Categories = "Interact.Type"))
	FGameplayTag TypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction", meta = (Categories = "Zone"))
	FGameplayTag ZoneTag;

	/** Where the indicator dot is drawn, in Pivot space (handle position). Zero = MovingMesh bounds centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	FVector InteractionPointLocal = FVector::ZeroVector;

	/** Outer parts that must be Open before this one can open (tray inside a drawer, drawer behind a door) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	TArray<TSoftObjectPtr<ANOARInteractiveBase>> RequiredOpen;

	/** Parts whose Open state blocks this one (neighbour door swinging into the drawer path) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	TArray<TSoftObjectPtr<ANOARInteractiveBase>> BlockedBy;

	/** Cabinet LED switched on while this part is Open */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	TSoftObjectPtr<ANOARInteractiveLight> LinkedLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	TObjectPtr<UNOARProductData> Product;

	/** Stable key for SaveGame / sequencer; defaults to the actor label */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Interaction")
	FName SaveKey;

	// ---- Motion ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Motion")
	ENOARKinematicsMode KinematicsMode = ENOARKinematicsMode::Visual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Motion", meta = (ClampMin = 0.1))
	float OpenDuration = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Motion", meta = (ClampMin = 0.1))
	float CloseDuration = 1.1f;

	/** Soft-close curve; empty = UNOARSettings::SoftCloseCurve; still empty = built-in ease */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Motion")
	TObjectPtr<UCurveFloat> MotionCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Motion")
	bool bAutoClose = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Motion", meta = (EditCondition = "bAutoClose"))
	float AutoCloseDelay = 6.f;

	/** Block the pawn while closed (interior doors), ignore it while moving */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Motion")
	bool bBlocksPawnWhenClosed = false;

	// ---- Sound ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Sound")
	TObjectPtr<USoundBase> SoundOpenStart;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Sound")
	TObjectPtr<USoundBase> SoundCloseStart;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Sound")
	TObjectPtr<USoundBase> SoundSoftCloseEnd;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Sound", meta = (ClampMin = 0, ClampMax = 1))
	float SoundVolume = 0.35f;

	// ---- Events ----
	UPROPERTY(BlueprintAssignable, Category = "NOAR|Events")
	FNOAROnStateChanged OnStateChanged;

	// ---- API ----
	UFUNCTION(BlueprintPure, Category = "NOAR|Interaction")
	ENOARInteractState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "NOAR|Interaction")
	bool IsOpen() const { return State == ENOARInteractState::Open; }

	UFUNCTION(BlueprintPure, Category = "NOAR|Interaction")
	float GetAlpha() const { return CurrentAlpha; }

	/** Toggle (same as Interact) */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Interaction")
	void Toggle();

	/** Drive to a state; bInstant skips the timeline (save restore, sequencer snapshot) */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Interaction")
	void SetOpen(bool bOpen, bool bInstant = false);

	/** Direct alpha for manufacturer kinematics / VR hand drag / sequencer scrubbing */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Interaction")
	void SetAlphaDirect(float Alpha);

	/** Highlight on/off: custom depth + stencil (1 focus, 2 locked) on every mesh */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Interaction")
	void SetHighlighted(bool bOn);

	// ---- INOARInteractable ----
	virtual bool CanInteract_Implementation(AActor* Instigator, ENOARInteractDenyReason& Reason) const override;
	virtual void Focus_Implementation(AActor* Instigator) override;
	virtual void Unfocus_Implementation(AActor* Instigator) override;
	virtual void Interact_Implementation(AActor* Instigator) override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual FVector GetInteractionPoint_Implementation() const override;
	virtual bool GetProductInfo_Implementation(FNOARProductInfo& OutInfo) const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Subclasses turn 0..1 into a transform of Pivot (and any secondary parts). */
	UFUNCTION(BlueprintNativeEvent, Category = "NOAR|Motion")
	void ApplyAlpha(float Alpha);
	virtual void ApplyAlpha_Implementation(float Alpha) {}

	/** Blueprint hook after the state settles (Open / Closed) */
	UFUNCTION(BlueprintImplementableEvent, Category = "NOAR|Events")
	void OnSettled(ENOARInteractState NewState);

	UFUNCTION()
	void TimelineUpdate(float Value);
	UFUNCTION()
	void TimelineFinished();

	void SetState(ENOARInteractState NewState);
	void StartOpening();
	void StartClosing();
	void PlaySoundSafe(USoundBase* Sound) const;
	void ApplyPawnCollision() const;
	float EvaluateCurve(float T) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Motion")
	ENOARInteractState State = ENOARInteractState::Closed;

	UPROPERTY()
	FTimeline Timeline;

	UPROPERTY()
	TObjectPtr<UCurveFloat> LinearCurve;
	float CurrentAlpha = 0.f;
	FTimerHandle AutoCloseTimer;
	bool bHighlighted = false;

public:
	virtual void Tick(float DeltaSeconds) override;
};
