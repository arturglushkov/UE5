#include "Interaction/NOARMechanisms.h"
#include "NOARGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"

// ============================ Door ============================

ANOARInteractiveDoor::ANOARInteractiveDoor()
{
	TypeTag = NOARTags::Interact_Type_Door;
	OpenDuration = 0.9f;
	CloseDuration = 1.1f;

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(MovingMesh);
	HandleMesh->SetMobility(EComponentMobility::Movable);
	HandleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANOARInteractiveDoor::ApplyAlpha_Implementation(float Alpha)
{
	float Sign = (HingeSide == ENOARHingeSide::Left) ? -1.f : 1.f;
	if (bOpensInward)
	{
		Sign *= -1.f;
	}
	const float Yaw = FMath::Lerp(ClosedAngle, OpenAngle * Sign, Alpha);
	Pivot->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
}

// ============================ Drawer ============================

ANOARInteractiveDrawer::ANOARInteractiveDrawer()
{
	TypeTag = NOARTags::Interact_Type_Drawer;
	OpenDuration = 0.7f;
	CloseDuration = 0.9f;
}

void ANOARInteractiveDrawer::BeginPlay()
{
	ResolvedTravel = TravelDistance;
	if (ResolvedTravel <= 0.f && MovingMesh && MovingMesh->GetStaticMesh())
	{
		const FBox Box = MovingMesh->GetStaticMesh()->GetBoundingBox();
		ResolvedTravel = FMath::Max(5.f, (Box.Max.X - Box.Min.X) - 2.f);
	}
	Super::BeginPlay();
}

void ANOARInteractiveDrawer::ApplyAlpha_Implementation(float Alpha)
{
	Pivot->SetRelativeLocation(TravelDirection.GetSafeNormal() * (ResolvedTravel * Alpha));
}

// ============================ Flap ============================

ANOARInteractiveFlap::ANOARInteractiveFlap()
{
	TypeTag = NOARTags::Interact_Type_Flap;
	OpenDuration = 1.2f;
	CloseDuration = 1.4f;

	SecondaryPivot = CreateDefaultSubobject<USceneComponent>(TEXT("SecondaryPivot"));
	SecondaryPivot->SetupAttachment(Pivot);

	SecondaryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SecondaryMesh"));
	SecondaryMesh->SetupAttachment(SecondaryPivot);
	SecondaryMesh->SetMobility(EComponentMobility::Movable);
	SecondaryMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SecondaryMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	SecondaryMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	SecondaryMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ArmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmMesh"));
	ArmMesh->SetupAttachment(Root);
	ArmMesh->SetMobility(EComponentMobility::Movable);
	ArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANOARInteractiveFlap::ApplyAlpha_Implementation(float Alpha)
{
	switch (Mode)
	{
	case ENOARFlapMode::LiftUp:
		Pivot->SetRelativeRotation(FRotator(OpenAngle * Alpha, 0.f, 0.f)); // positive pitch lifts the lower edge forward/up
		break;

	case ENOARFlapMode::DropDown:
		Pivot->SetRelativeRotation(FRotator(-OpenAngle * Alpha, 0.f, 0.f));
		break;

	case ENOARFlapMode::BiFold:
		Pivot->SetRelativeRotation(FRotator(OpenAngle * Alpha, 0.f, 0.f));
		SecondaryPivot->SetRelativeRotation(FRotator(-OpenAngle * LowerLeafFactor * Alpha, 0.f, 0.f));
		break;

	case ENOARFlapMode::Swing:
	case ENOARFlapMode::Lift:
	{
		const float Z = ZOffsetCurve ? ZOffsetCurve->GetFloatValue(Alpha) : ZTravel * Alpha;
		const float X = XOffsetCurve ? XOffsetCurve->GetFloatValue(Alpha) : XTravel * FMath::Sin(Alpha * PI);
		const float Pitch = (Mode == ENOARFlapMode::Swing) ? OpenAngle * Alpha : OpenAngle * 0.1f * Alpha; // HL stays almost parallel
		Pivot->SetRelativeLocation(FVector(X, 0.f, Z));
		Pivot->SetRelativeRotation(FRotator(Pitch, 0.f, 0.f));
		break;
	}
	}

	// decorative arm follows the front's lower edge
	if (ArmMesh && ArmMesh->GetStaticMesh() && MovingMesh)
	{
		const FVector Target = MovingMesh->GetComponentLocation();
		const FVector From = ArmMesh->GetComponentLocation();
		ArmMesh->SetWorldRotation((Target - From).Rotation());
	}
}

// ============================ Sliding door ============================

ANOARInteractiveSlidingDoor::ANOARInteractiveSlidingDoor()
{
	TypeTag = NOARTags::Interact_Type_Slide;
	OpenDuration = 1.2f;
	CloseDuration = 1.3f;
}

void ANOARInteractiveSlidingDoor::BeginPlay()
{
	ResolvedTravel = TravelDistance;
	if (ResolvedTravel <= 0.f && MovingMesh && MovingMesh->GetStaticMesh())
	{
		const FBox Box = MovingMesh->GetStaticMesh()->GetBoundingBox();
		ResolvedTravel = FMath::Max(5.f, (Box.Max.Y - Box.Min.Y) - 2.f);
	}
	Super::BeginPlay();
}

void ANOARInteractiveSlidingDoor::ApplyAlpha_Implementation(float Alpha)
{
	Pivot->SetRelativeLocation(TravelDirection.GetSafeNormal() * (ResolvedTravel * Alpha));
}

void ANOARInteractiveSlidingDoor::Interact_Implementation(AActor* Instigator)
{
	Super::Interact_Implementation(Instigator);
	if (ANOARInteractiveSlidingDoor* Pair = PairedLeaf.Get())
	{
		if (Pair->GetState() != GetState())
		{
			Pair->SetOpen(GetState() == ENOARInteractState::Opening || GetState() == ENOARInteractState::Open, false);
		}
	}
}

// ============================ Pocket ============================

ANOARInteractivePocket::ANOARInteractivePocket()
{
	TypeTag = NOARTags::Interact_Type_Pocket;
	OpenDuration = 1.2f;
	CloseDuration = 1.3f;

	// re-parent: Root -> Pivot (carriage) -> DoorPivot (hinge) -> MovingMesh
	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(Pivot);
	MovingMesh->SetupAttachment(DoorPivot);
}

void ANOARInteractivePocket::BeginPlay()
{
	ResolvedSlide = SlideDistance;
	if (ResolvedSlide <= 0.f && MovingMesh && MovingMesh->GetStaticMesh())
	{
		const FBox Box = MovingMesh->GetStaticMesh()->GetBoundingBox();
		ResolvedSlide = FMath::Max(5.f, (Box.Max.Y - Box.Min.Y) - 1.f);
	}
	Super::BeginPlay();
}

void ANOARInteractivePocket::ApplyAlpha_Implementation(float Alpha)
{
	if (bSlideOnly)
	{
		Pivot->SetRelativeLocation(SlideDirection.GetSafeNormal() * (ResolvedSlide * Alpha));
		return;
	}
	// phase 1: swing to SwingAngle; phase 2: slide into the pocket
	const float SwingA = FMath::Clamp(Alpha / SwingPhase, 0.f, 1.f);
	const float SlideA = FMath::Clamp((Alpha - SwingPhase) / (1.f - SwingPhase), 0.f, 1.f);
	const float Sign = (HingeSide == ENOARHingeSide::Left) ? -1.f : 1.f;
	DoorPivot->SetRelativeRotation(FRotator(0.f, SwingAngle * Sign * SwingA, 0.f));
	Pivot->SetRelativeLocation(SlideDirection.GetSafeNormal() * (ResolvedSlide * SlideA));
}
