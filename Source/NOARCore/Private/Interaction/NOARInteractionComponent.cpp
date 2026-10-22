#include "Interaction/NOARInteractionComponent.h"
#include "Interaction/NOARInteractable.h"
#include "NOARSettings.h"
#include "NOARCore.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

UNOARInteractionComponent::UNOARInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNOARInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	const UNOARSettings* S = GetDefault<UNOARSettings>();
	GetWorld()->GetTimerManager().SetTimer(TraceTimer, this, &UNOARInteractionComponent::TraceTick, FMath::Max(0.016f, S->TraceInterval), true);
}

void UNOARInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(TraceTimer);
	}
	Super::EndPlay(EndPlayReason);
}

float UNOARInteractionComponent::GetMaxDistance() const
{
	return MaxInteractionDistanceOverride > 0.f ? MaxInteractionDistanceOverride : GetDefault<UNOARSettings>()->MaxInteractionDistance;
}

void UNOARInteractionComponent::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
	if (!bEnabled)
	{
		SetFocus(nullptr, false, ENOARInteractDenyReason::OK);
	}
}

bool UNOARInteractionComponent::DoTrace(FHitResult& OutHit) const
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC)
	{
		return false;
	}

	const float Length = GetDefault<UNOARSettings>()->TraceLength;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NOARInteract), false, GetOwner());

	if (InputMode == ENOARInputMode::Mouse3P)
	{
		FVector Origin, Dir;
		if (!PC->DeprojectMousePositionToWorld(Origin, Dir))
		{
			return false;
		}
		return GetWorld()->LineTraceSingleByChannel(OutHit, Origin, Origin + Dir * Length, InteractableChannel, Params);
	}

	// First person / gamepad: screen centre
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	return GetWorld()->LineTraceSingleByChannel(OutHit, CamLoc, CamLoc + CamRot.Vector() * Length, InteractableChannel, Params);
}

bool UNOARInteractionComponent::IsOccluded(AActor* Target, const FVector& Point) const
{
	// Second trace from the chest to the interaction point on Visibility: a drawer behind a closed door is occluded.
	const AActor* Owner = GetOwner();
	FVector Chest = Owner->GetActorLocation() + FVector(0, 0, 40.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NOAROcclusion), false, Owner);
	Params.AddIgnoredActor(Target);
	// the target's parent carcass (attach parent) is not an occluder either
	if (AActor* Parent = Target->GetAttachParentActor())
	{
		Params.AddIgnoredActor(Parent);
	}
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Chest, Point, ECC_Visibility, Params))
	{
		// Anything hit before reaching the point that is not the target itself occludes it
		return Hit.GetActor() != Target && (Hit.Location - Point).SizeSquared() > FMath::Square(8.f);
	}
	return false;
}

void UNOARInteractionComponent::TraceTick()
{
	if (!bEnabled)
	{
		return;
	}

	FHitResult Hit;
	AActor* Candidate = nullptr;
	if (DoTrace(Hit))
	{
		AActor* HitActor = Hit.GetActor();
		// walk up to the first actor implementing the interface (meshes can be children of the interactive actor)
		while (HitActor && !HitActor->Implements<UNOARInteractable>())
		{
			HitActor = HitActor->GetAttachParentActor();
		}
		Candidate = HitActor;
	}

	if (!Candidate)
	{
		SetFocus(nullptr, false, ENOARInteractDenyReason::OK);
		return;
	}

	const FVector Point = INOARInteractable::Execute_GetInteractionPoint(Candidate);
	const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Point);
	const bool bInRange = Dist <= GetMaxDistance();

	ENOARInteractDenyReason Reason = ENOARInteractDenyReason::OK;
	if (bInRange)
	{
		if (IsOccluded(Candidate, Point))
		{
			Reason = ENOARInteractDenyReason::Occluded;
		}
		else
		{
			INOARInteractable::Execute_CanInteract(Candidate, GetOwner(), Reason);
		}
	}
	else
	{
		Reason = ENOARInteractDenyReason::TooFar;
	}

	SetFocus(Candidate, bInRange, Reason);
}

void UNOARInteractionComponent::SetFocus(AActor* NewActor, bool bInRange, ENOARInteractDenyReason Reason)
{
	AActor* Old = FocusedActor.Get();
	if (Old != NewActor)
	{
		if (Old && Old->Implements<UNOARInteractable>())
		{
			INOARInteractable::Execute_Unfocus(Old, GetOwner());
		}
		if (NewActor)
		{
			INOARInteractable::Execute_Focus(NewActor, GetOwner());
		}
	}
	else if (bInRange == bFocusedInRange && Reason == LastReason)
	{
		return; // nothing changed
	}

	FocusedActor = NewActor;
	bFocusedInRange = bInRange;
	LastReason = Reason;

	FText Prompt;
	FVector Point = FVector::ZeroVector;
	if (NewActor)
	{
		Point = INOARInteractable::Execute_GetInteractionPoint(NewActor);
		switch (Reason)
		{
		case ENOARInteractDenyReason::TooFar:
			Prompt = (InputMode == ENOARInputMode::Mouse3P)
				? INOARInteractable::Execute_GetInteractionText(NewActor)
				: NSLOCTEXT("NOAR", "MoveCloser", "Move closer");
			break;
		case ENOARInteractDenyReason::Locked:     Prompt = NSLOCTEXT("NOAR", "Locked", "Locked"); break;
		case ENOARInteractDenyReason::Dependency: Prompt = NSLOCTEXT("NOAR", "OpenOuterFirst", "Open the door first"); break;
		case ENOARInteractDenyReason::Blocked:    Prompt = NSLOCTEXT("NOAR", "Blocked", "Blocked"); break;
		case ENOARInteractDenyReason::Occluded:   Prompt = FText::GetEmpty(); break;
		default:                                  Prompt = INOARInteractable::Execute_GetInteractionText(NewActor); break;
		}
	}
	OnFocusChanged.Broadcast(NewActor, Prompt, Point, Reason);
}

void UNOARInteractionComponent::TryInteract()
{
	AActor* Target = FocusedActor.Get();
	if (!bEnabled)
	{
		return;
	}
	if (!Target)
	{
		// nothing interactive under the cursor: click on the floor walks there (3P mouse only)
		if (InputMode == ENOARInputMode::Mouse3P)
		{
			const APawn* Pawn = Cast<APawn>(GetOwner());
			APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
			FHitResult Hit;
			if (PC && PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.bBlockingHit && Hit.ImpactNormal.Z > 0.7f)
			{
				PendingTarget = nullptr;
				OnWalkToRequested.Broadcast(Hit.ImpactPoint);
			}
		}
		return;
	}

	if (!bFocusedInRange)
	{
		// Mouse 3P: click-to-walk; the character calls NotifyArrivedAtPendingTarget when it gets there
		if (InputMode == ENOARInputMode::Mouse3P)
		{
			PendingTarget = Target;
			OnMoveToRequested.Broadcast(INOARInteractable::Execute_GetInteractionPoint(Target));
		}
		return;
	}

	if (LastReason != ENOARInteractDenyReason::OK)
	{
		return; // prompt already explains why
	}

	INOARInteractable::Execute_Interact(Target, GetOwner());
	OnInteracted.Broadcast(Target);
}

void UNOARInteractionComponent::NotifyArrivedAtPendingTarget()
{
	AActor* Target = PendingTarget.Get();
	PendingTarget = nullptr;
	if (!Target)
	{
		return;
	}
	const FVector Point = INOARInteractable::Execute_GetInteractionPoint(Target);
	if (FVector::Dist(GetOwner()->GetActorLocation(), Point) <= GetMaxDistance())
	{
		ENOARInteractDenyReason Reason;
		if (INOARInteractable::Execute_CanInteract(Target, GetOwner(), Reason) && !IsOccluded(Target, Point))
		{
			INOARInteractable::Execute_Interact(Target, GetOwner());
			OnInteracted.Broadcast(Target);
		}
	}
}

bool UNOARInteractionComponent::GetFocusedProductInfo(FNOARProductInfo& OutInfo) const
{
	AActor* Target = FocusedActor.Get();
	return Target ? INOARInteractable::Execute_GetProductInfo(Target, OutInfo) : false;
}
