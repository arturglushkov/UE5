#pragma once

#include "CoreMinimal.h"
#include "Interaction/NOARInteractiveBase.h"
#include "NOARMechanisms.generated.h"

class UCurveFloat;

/**
 * Hinged door: CLIP top cabinet fronts, interior doors D.03-D.13, fridge.
 * Pivot sits on the hinge axis (vertical edge), local X points out of the cabinet. Rotation around Z.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARInteractiveDoor : public ANOARInteractiveBase
{
	GENERATED_BODY()

public:
	ANOARInteractiveDoor();

	/** 105 for CLIP top 110, 90 for interior doors, 115 for fridge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Door", meta = (ClampMin = 10, ClampMax = 180))
	float OpenAngle = 105.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Door")
	float ClosedAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Door")
	ENOARHingeSide HingeSide = ENOARHingeSide::Left;

	/** Flip for doors that open inward (interior doors) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Door")
	bool bOpensInward = false;

	/** Optional handle mesh under Pivot (moves with the door) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

protected:
	virtual void ApplyAlpha_Implementation(float Alpha) override;
};

/**
 * Drawer: LEGRABOX / MOVENTO / TANDEM, internal trays, dishwasher racks, oven trays.
 * Pivot at the back-bottom-centre of the drawer; travel along local +X.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARInteractiveDrawer : public ANOARInteractiveBase
{
	GENERATED_BODY()

public:
	ANOARInteractiveDrawer();

	/** cm. Full extension = cabinet depth - 2; TANDEM partial = 0.75 x depth. 0 = auto from mesh bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Drawer", meta = (ClampMin = 0))
	float TravelDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Drawer")
	FVector TravelDirection = FVector(1.f, 0.f, 0.f);

protected:
	virtual void BeginPlay() override;
	virtual void ApplyAlpha_Implementation(float Alpha) override;
	float ResolvedTravel = 0.f;
};

UENUM(BlueprintType)
enum class ENOARFlapMode : uint8
{
	LiftUp		UMETA(DisplayName = "AVENTOS HK / HK top (lift-up)"),
	DropDown	UMETA(DisplayName = "Drop-down (dishwasher, oven, Murphy bed)"),
	BiFold		UMETA(DisplayName = "AVENTOS HF (bi-fold up)"),
	Swing		UMETA(DisplayName = "AVENTOS HS (swing over cabinet)"),
	Lift		UMETA(DisplayName = "AVENTOS HL (parallel lift)")
};

/**
 * Flaps: AVENTOS HK/HF/HS/HL, dishwasher and oven doors, Murphy bed. Rotation around local Y.
 * LiftUp: pivot on the top edge, opens to -OpenAngle. DropDown: pivot on the bottom edge, opens to +OpenAngle.
 * BiFold: Pivot = upper leaf (top edge), LowerLeaf child pivot at the leaf joint, rotates opposite.
 * Swing / Lift: VISUAL mode uses the two curves (ZOffsetCurve, XOffsetCurve, in cm) plus angle; MANUFACTURER mode
 * expects the curves to be digitised from the Blum trajectory drawings.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARInteractiveFlap : public ANOARInteractiveBase
{
	GENERATED_BODY()

public:
	ANOARInteractiveFlap();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Flap")
	ENOARFlapMode Mode = ENOARFlapMode::LiftUp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Flap", meta = (ClampMin = 10, ClampMax = 180))
	float OpenAngle = 90.f;

	/** BiFold: lower leaf relative angle multiplier (HF ~ 1.9) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Flap")
	float LowerLeafFactor = 1.9f;

	/** Swing / Lift: vertical travel of the pivot over alpha, cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Flap")
	TObjectPtr<UCurveFloat> ZOffsetCurve;

	/** Swing / Lift: forward travel of the pivot over alpha, cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Flap")
	TObjectPtr<UCurveFloat> XOffsetCurve;

	/** Fallback travel when no curves are set (HS: up ~ front height, HL: up ~ 0.6 x height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Flap")
	float ZTravel = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Flap")
	float XTravel = 8.f;

	/** BiFold lower leaf / Murphy bed legs */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<USceneComponent> SecondaryPivot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UStaticMeshComponent> SecondaryMesh;

	/** Decorative lift arm that looks at the front (HK) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UStaticMeshComponent> ArmMesh;

protected:
	virtual void ApplyAlpha_Implementation(float Alpha) override;
};

/** Sliding door on a track (D.14 double sliding, wardrobe fronts). Pivot at the top-centre on the track; travel along local Y. */
UCLASS(Blueprintable)
class NOARCORE_API ANOARInteractiveSlidingDoor : public ANOARInteractiveBase
{
	GENERATED_BODY()

public:
	ANOARInteractiveSlidingDoor();

	/** cm; 0 = auto (door width - 2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Sliding", meta = (ClampMin = 0))
	float TravelDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Sliding")
	FVector TravelDirection = FVector(0.f, 1.f, 0.f);

	/** Second leaf (double sliding) moving the opposite way; optional */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Sliding")
	TSoftObjectPtr<ANOARInteractiveSlidingDoor> PairedLeaf;

protected:
	virtual void BeginPlay() override;
	virtual void ApplyAlpha_Implementation(float Alpha) override;
	virtual void Interact_Implementation(AActor* Instigator) override;
	float ResolvedTravel = 0.f;
};

/**
 * Pocket door (D.10, D.12: slides into the wall) and pocket systems (Hawa Concepta / Blum REVEGO: swing 90 then slide in).
 * Pivot = carriage on the track; DoorPivot (child) = hinge for the swing phase.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARInteractivePocket : public ANOARInteractiveBase
{
	GENERATED_BODY()

public:
	ANOARInteractivePocket();

	/** true = wall pocket door (slide only); false = furniture pocket system (swing, then slide) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Pocket")
	bool bSlideOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Pocket", meta = (ClampMin = 0))
	float SlideDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Pocket")
	FVector SlideDirection = FVector(0.f, 1.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Pocket")
	float SwingAngle = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Pocket")
	ENOARHingeSide HingeSide = ENOARHingeSide::Left;

	/** Fraction of alpha spent swinging before the slide starts (pocket systems) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Pocket", meta = (ClampMin = 0.1, ClampMax = 0.9))
	float SwingPhase = 0.45f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<USceneComponent> DoorPivot;

protected:
	virtual void BeginPlay() override;
	virtual void ApplyAlpha_Implementation(float Alpha) override;
	float ResolvedSlide = 0.f;
};
