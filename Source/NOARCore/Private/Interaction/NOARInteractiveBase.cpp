#include "Interaction/NOARInteractiveBase.h"
#include "Lighting/NOARInteractiveLight.h"
#include "Data/NOARProductData.h"
#include "NOARSettings.h"
#include "NOARCore.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "EngineUtils.h"

ANOARInteractiveBase::ANOARInteractiveBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // ticks only while the timeline runs

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(Root);

	MovingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MovingMesh"));
	MovingMesh->SetupAttachment(Pivot);
	MovingMesh->SetMobility(EComponentMobility::Movable);
	MovingMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MovingMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	MovingMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // Interactable
	MovingMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);        // occludes parts behind it
	MovingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	MovingMesh->SetRenderCustomDepth(false);
	MovingMesh->CustomDepthStencilValue = 1;
}

void ANOARInteractiveBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (SaveKey.IsNone())
	{
#if WITH_EDITOR
		SaveKey = FName(*GetActorLabel());
#else
		SaveKey = GetFName();
#endif
	}
}

void ANOARInteractiveBase::BeginPlay()
{
	Super::BeginPlay();

	FOnTimelineFloat Update;
	Update.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ANOARInteractiveBase, TimelineUpdate));
	FOnTimelineEvent Finished;
	Finished.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ANOARInteractiveBase, TimelineFinished));

	// The timeline always runs 0..1 over 1 s; play rate scales it to Open/CloseDuration. The curve shapes the motion.
	LinearCurve = NewObject<UCurveFloat>(this, TEXT("LinearCurve"));
	LinearCurve->FloatCurve.AddKey(0.f, 0.f);
	LinearCurve->FloatCurve.AddKey(1.f, 1.f);
	Timeline.AddInterpFloat(LinearCurve, Update);
	Timeline.SetTimelineFinishedFunc(Finished);
	Timeline.SetTimelineLength(1.f);
	Timeline.SetLooping(false);

	ApplyAlpha(0.f);
	ApplyPawnCollision();
}

void ANOARInteractiveBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Timeline.TickTimeline(DeltaSeconds);
}

float ANOARInteractiveBase::EvaluateCurve(float T) const
{
	const UCurveFloat* Curve = MotionCurve;
	if (!Curve)
	{
		Curve = GetDefault<UNOARSettings>()->SoftCloseCurve.LoadSynchronous();
	}
	if (Curve)
	{
		return FMath::Clamp(Curve->GetFloatValue(T), 0.f, 1.f);
	}
	// Built-in soft-close: fast start, controlled middle, last 4% over the last 20% of time (section 9)
	if (T < 0.45f)
	{
		return FMath::InterpEaseOut(0.f, 0.78f, T / 0.45f, 1.6f);
	}
	if (T < 0.80f)
	{
		return FMath::Lerp(0.78f, 0.96f, (T - 0.45f) / 0.35f);
	}
	return FMath::InterpEaseOut(0.96f, 1.f, (T - 0.80f) / 0.20f, 2.5f);
}

void ANOARInteractiveBase::TimelineUpdate(float Value)
{
	// Value is linear time 0..1 in the open direction; shape it so both open and close soft-close at their end.
	const bool bOpening = (State == ENOARInteractState::Opening);
	const float Shaped = bOpening ? EvaluateCurve(Value) : 1.f - EvaluateCurve(1.f - Value);
	CurrentAlpha = Shaped;
	ApplyAlpha(CurrentAlpha);
}

void ANOARInteractiveBase::TimelineFinished()
{
	SetActorTickEnabled(false);
	if (State == ENOARInteractState::Opening)
	{
		CurrentAlpha = 1.f;
		ApplyAlpha(1.f);
		SetState(ENOARInteractState::Open);
		if (bAutoClose && AutoCloseDelay > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(AutoCloseTimer, this, &ANOARInteractiveBase::StartClosing, AutoCloseDelay, false);
		}
	}
	else
	{
		CurrentAlpha = 0.f;
		ApplyAlpha(0.f);
		SetState(ENOARInteractState::Closed);
		PlaySoundSafe(SoundSoftCloseEnd);
	}
	ApplyPawnCollision();
	OnSettled(State);
}

void ANOARInteractiveBase::SetState(ENOARInteractState NewState)
{
	if (State == NewState)
	{
		return;
	}
	State = NewState;

	// Cabinet LED follows Open / not Open
	if (ANOARInteractiveLight* Light = LinkedLight.Get())
	{
		Light->SetOn(State == ENOARInteractState::Open || State == ENOARInteractState::Opening);
	}
	OnStateChanged.Broadcast(this, State);
}

void ANOARInteractiveBase::StartOpening()
{
	GetWorld()->GetTimerManager().ClearTimer(AutoCloseTimer);
	SetState(ENOARInteractState::Opening);
	PlaySoundSafe(SoundOpenStart);
	Timeline.SetPlayRate(1.f / FMath::Max(0.05f, OpenDuration));
	SetActorTickEnabled(true);
	ApplyPawnCollision();
	Timeline.Play();
}

void ANOARInteractiveBase::StartClosing()
{
	GetWorld()->GetTimerManager().ClearTimer(AutoCloseTimer);

	// A person closes the inner part first: close anything that requires us to be open
	for (TActorIterator<ANOARInteractiveBase> It(GetWorld()); It; ++It)
	{
		for (const TSoftObjectPtr<ANOARInteractiveBase>& Req : It->RequiredOpen)
		{
			if (Req.Get() == this && It->IsOpen())
			{
				It->SetOpen(false, false);
			}
		}
	}

	SetState(ENOARInteractState::Closing);
	PlaySoundSafe(SoundCloseStart);
	Timeline.SetPlayRate(1.f / FMath::Max(0.05f, CloseDuration));
	SetActorTickEnabled(true);
	ApplyPawnCollision();
	Timeline.Reverse();
}

void ANOARInteractiveBase::Toggle()
{
	switch (State)
	{
	case ENOARInteractState::Closed:
	case ENOARInteractState::Closing:
		StartOpening();
		break;
	case ENOARInteractState::Open:
	case ENOARInteractState::Opening:
		StartClosing();
		break;
	}
}

void ANOARInteractiveBase::SetOpen(bool bOpen, bool bInstant)
{
	if (bInstant)
	{
		Timeline.Stop();
		SetActorTickEnabled(false);
		CurrentAlpha = bOpen ? 1.f : 0.f;
		Timeline.SetPlaybackPosition(CurrentAlpha, false, false);
		ApplyAlpha(CurrentAlpha);
		SetState(bOpen ? ENOARInteractState::Open : ENOARInteractState::Closed);
		ApplyPawnCollision();
		return;
	}
	if (bOpen && (State == ENOARInteractState::Closed || State == ENOARInteractState::Closing))
	{
		StartOpening();
	}
	else if (!bOpen && (State == ENOARInteractState::Open || State == ENOARInteractState::Opening))
	{
		StartClosing();
	}
}

void ANOARInteractiveBase::SetAlphaDirect(float Alpha)
{
	Timeline.Stop();
	SetActorTickEnabled(false);
	CurrentAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
	Timeline.SetPlaybackPosition(CurrentAlpha, false, false);
	ApplyAlpha(CurrentAlpha);
	if (CurrentAlpha >= 0.999f)      SetState(ENOARInteractState::Open);
	else if (CurrentAlpha <= 0.001f) SetState(ENOARInteractState::Closed);
}

void ANOARInteractiveBase::SetHighlighted(bool bOn)
{
	bHighlighted = bOn;
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);
	const int32 Stencil = bLocked ? 2 : 1;
	for (UStaticMeshComponent* M : Meshes)
	{
		M->SetCustomDepthStencilValue(Stencil);
		M->SetRenderCustomDepth(bOn);
	}
}

void ANOARInteractiveBase::PlaySoundSafe(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetInteractionPoint_Implementation(), SoundVolume);
	}
}

void ANOARInteractiveBase::ApplyPawnCollision() const
{
	if (!MovingMesh)
	{
		return;
	}
	const bool bBlock = bBlocksPawnWhenClosed && State == ENOARInteractState::Closed;
	MovingMesh->SetCollisionResponseToChannel(ECC_Pawn, bBlock ? ECR_Block : ECR_Ignore);
}

// ---------------- INOARInteractable ----------------

bool ANOARInteractiveBase::CanInteract_Implementation(AActor* Instigator, ENOARInteractDenyReason& Reason) const
{
	Reason = ENOARInteractDenyReason::OK;
	if (!bInteractionEnabled)
	{
		Reason = ENOARInteractDenyReason::Locked;
		return false;
	}
	if (bLocked)
	{
		Reason = ENOARInteractDenyReason::Locked;
		return false;
	}
	// opening requires outer parts open; closing is always allowed
	if (State == ENOARInteractState::Closed || State == ENOARInteractState::Closing)
	{
		for (const TSoftObjectPtr<ANOARInteractiveBase>& Req : RequiredOpen)
		{
			const ANOARInteractiveBase* R = Req.Get();
			if (R && !R->IsOpen())
			{
				Reason = ENOARInteractDenyReason::Dependency;
				return false;
			}
		}
		for (const TSoftObjectPtr<ANOARInteractiveBase>& B : BlockedBy)
		{
			const ANOARInteractiveBase* Blocker = B.Get();
			if (Blocker && Blocker->GetState() != ENOARInteractState::Closed)
			{
				Reason = ENOARInteractDenyReason::Blocked;
				return false;
			}
		}
	}
	return true;
}

void ANOARInteractiveBase::Focus_Implementation(AActor* Instigator)
{
	SetHighlighted(true);
}

void ANOARInteractiveBase::Unfocus_Implementation(AActor* Instigator)
{
	SetHighlighted(false);
}

void ANOARInteractiveBase::Interact_Implementation(AActor* Instigator)
{
	Toggle();
}

FText ANOARInteractiveBase::GetInteractionText_Implementation() const
{
	return (State == ENOARInteractState::Open || State == ENOARInteractState::Opening)
		? NSLOCTEXT("NOAR", "Close", "Close")
		: NSLOCTEXT("NOAR", "Open", "Open");
}

FVector ANOARInteractiveBase::GetInteractionPoint_Implementation() const
{
	if (!InteractionPointLocal.IsNearlyZero())
	{
		return Pivot->GetComponentTransform().TransformPosition(InteractionPointLocal);
	}
	if (MovingMesh && MovingMesh->GetStaticMesh())
	{
		return MovingMesh->Bounds.Origin;
	}
	return GetActorLocation();
}

bool ANOARInteractiveBase::GetProductInfo_Implementation(FNOARProductInfo& OutInfo) const
{
	if (Product)
	{
		OutInfo = Product->Info;
		return true;
	}
	return false;
}
