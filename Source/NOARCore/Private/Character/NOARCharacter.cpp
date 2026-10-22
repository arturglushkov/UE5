#include "Character/NOARCharacter.h"
#include "Interaction/NOARInteractionComponent.h"
#include "NOARSettings.h"
#include "NOARCore.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NavigationSystem.h"

ANOARCharacter::ANOARCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(30.f, 90.f);

	// Section 7: no jump / crouch, orient to movement, calm acceleration
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 360.f, 0.f);
	Move->MaxWalkSpeed = 160.f;
	Move->MaxAcceleration = 600.f;
	Move->BrakingDecelerationWalking = 900.f;
	Move->GroundFriction = 8.f;
	Move->BrakingFrictionFactor = 1.5f;
	Move->MaxStepHeight = 15.f;
	Move->JumpZVelocity = 0.f;
	Move->NavAgentProps.bCanJump = false;
	Move->NavAgentProps.bCanCrouch = false;
	Move->bUseSeparateBrakingFriction = true;
	Move->BrakingFriction = 6.f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 320.f;
	SpringArm->SocketOffset = FVector(0.f, 35.f, 55.f);
	SpringArm->TargetOffset = FVector(0.f, 0.f, 40.f);
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 8.f;
	SpringArm->CameraLagMaxDistance = 60.f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 12.f;
	SpringArm->bDoCollisionTest = true;
	SpringArm->ProbeSize = 14.f;
	SpringArm->ProbeChannel = ECC_Camera;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 75.f;
	Camera->PostProcessBlendWeight = 0.f;

	Interaction = CreateDefaultSubobject<UNOARInteractionComponent>(TEXT("Interaction"));

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANOARCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplySettings();
	Interaction->OnMoveToRequested.AddDynamic(this, &ANOARCharacter::OnMoveToRequested);
	Interaction->OnWalkToRequested.AddDynamic(this, &ANOARCharacter::OnWalkToRequested);
	// no character mesh assigned yet (Third Person pack not added): start in first person so the view is not an empty orbit
	SetCameraMode(GetMesh() && GetMesh()->GetSkeletalMeshAsset() ? ENOARCameraMode::ThirdPerson : ENOARCameraMode::FirstPerson);
}

void ANOARCharacter::ApplySettings()
{
	const UNOARSettings* S = GetDefault<UNOARSettings>();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = bFastWalk ? S->FastWalkSpeed : (CameraMode == ENOARCameraMode::FirstPerson ? S->FirstPersonWalkSpeed : S->WalkSpeed);
	Move->MaxAcceleration = S->MaxAcceleration;
	Move->BrakingDecelerationWalking = S->BrakingDeceleration;
	Move->RotationRate = FRotator(0.f, S->RotationRateYaw, 0.f);
	SpringArm->SocketOffset = S->ThirdPersonSocketOffset;
	SpringArm->CameraLagSpeed = S->CameraLagSpeed;
	SpringArm->CameraRotationLagSpeed = S->CameraRotationLagSpeed;
	TargetArm = S->ThirdPersonArmLength;
	TargetFOV = S->ThirdPersonFOV;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMin = S->PitchMin;
			PC->PlayerCameraManager->ViewPitchMax = S->PitchMax;
		}
	}
}

void ANOARCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (WalkContext)
			{
				Sub->AddMappingContext(WalkContext, 0);
			}
		}
	}
	ApplySettings();
}

void ANOARCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogNOAR, Error, TEXT("Enhanced Input component missing - check DefaultInput.ini"));
		return;
	}
	if (IA_Move)         EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ANOARCharacter::Input_Move);
	if (IA_Look)         EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ANOARCharacter::Input_Look);
	if (IA_LookHold)
	{
		EIC->BindAction(IA_LookHold, ETriggerEvent::Started, this, &ANOARCharacter::Input_LookHoldStart);
		EIC->BindAction(IA_LookHold, ETriggerEvent::Completed, this, &ANOARCharacter::Input_LookHoldEnd);
		EIC->BindAction(IA_LookHold, ETriggerEvent::Canceled, this, &ANOARCharacter::Input_LookHoldEnd);
	}
	if (IA_FastWalk)
	{
		EIC->BindAction(IA_FastWalk, ETriggerEvent::Started, this, &ANOARCharacter::Input_FastWalkStart);
		EIC->BindAction(IA_FastWalk, ETriggerEvent::Completed, this, &ANOARCharacter::Input_FastWalkEnd);
	}
	if (IA_Interact)     EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ANOARCharacter::Input_Interact);
	if (IA_ToggleCamera) EIC->BindAction(IA_ToggleCamera, ETriggerEvent::Started, this, &ANOARCharacter::Input_ToggleCamera);
	if (IA_Zoom)         EIC->BindAction(IA_Zoom, ETriggerEvent::Triggered, this, &ANOARCharacter::Input_Zoom);
	if (IA_KeyLook)      EIC->BindAction(IA_KeyLook, ETriggerEvent::Triggered, this, &ANOARCharacter::Input_KeyLook);
}

// ---------------- input ----------------

void ANOARCharacter::Input_Move(const FInputActionValue& Value)
{
	if (bInputFrozen)
	{
		return;
	}
	if (bPendingMove)
	{
		bPendingMove = false; // manual input cancels click-to-walk
		if (AController* C = GetController()) C->StopMovement();
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), Axis.X);
}

void ANOARCharacter::Input_Look(const FInputActionValue& Value)
{
	if (bInputFrozen)
	{
		return;
	}
	if (CameraMode == ENOARCameraMode::ThirdPerson && !bLookHeld)
	{
		return; // 3P: look only with RMB held (cursor stays free for interaction)
	}
	const UNOARSettings* S = GetDefault<UNOARSettings>();
	const FVector2D Raw = Value.Get<FVector2D>() * S->LookSensitivity;
	// exponential smoothing keeps the reference's responsiveness without the jerk
	SmoothedLook = FMath::Vector2DInterpTo(SmoothedLook, Raw, GetWorld()->GetDeltaSeconds(), 15.f);
	AddControllerYawInput(SmoothedLook.X);
	AddControllerPitchInput(-SmoothedLook.Y);
}

void ANOARCharacter::Input_LookHoldStart(const FInputActionValue&) { bLookHeld = true; }
void ANOARCharacter::Input_LookHoldEnd(const FInputActionValue&)   { bLookHeld = false; SmoothedLook = FVector2D::ZeroVector; }

void ANOARCharacter::Input_FastWalkStart(const FInputActionValue&) { bFastWalk = true;  ApplySettings(); }
void ANOARCharacter::Input_FastWalkEnd(const FInputActionValue&)   { bFastWalk = false; ApplySettings(); }

void ANOARCharacter::Input_Interact(const FInputActionValue&)
{
	if (!bInputFrozen)
	{
		Interaction->TryInteract();
	}
}

void ANOARCharacter::Input_ToggleCamera(const FInputActionValue&)
{
	if (!bInputFrozen)
	{
		ToggleCameraMode();
	}
}

void ANOARCharacter::Input_Zoom(const FInputActionValue& Value)
{
	if (CameraMode != ENOARCameraMode::ThirdPerson || bInputFrozen)
	{
		return;
	}
	UserZoom = FMath::Clamp(UserZoom - Value.Get<float>() * 20.f, -150.f, 120.f);
}

// ---------------- camera ----------------

void ANOARCharacter::SetCameraMode(ENOARCameraMode Mode)
{
	if (CameraMode == Mode)
	{
		return;
	}
	CameraMode = Mode;
	const UNOARSettings* S = GetDefault<UNOARSettings>();
	UCharacterMovementComponent* Move = GetCharacterMovement();

	switch (Mode)
	{
	case ENOARCameraMode::FirstPerson:
		Move->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
		SpringArm->bEnableCameraLag = false;
		SpringArm->bDoCollisionTest = false;
		Interaction->InputMode = ENOARInputMode::FirstPerson;
		break;
	case ENOARCameraMode::ThirdPerson:
	default:
		Move->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;
		SpringArm->bEnableCameraLag = true;
		SpringArm->bDoCollisionTest = true;
		Interaction->InputMode = ENOARInputMode::Mouse3P;
		break;
	}
	ApplySettings();
	OnCameraModeChanged.Broadcast(CameraMode);
}

void ANOARCharacter::ToggleCameraMode()
{
	SetCameraMode(CameraMode == ENOARCameraMode::ThirdPerson ? ENOARCameraMode::FirstPerson : ENOARCameraMode::ThirdPerson);
}

void ANOARCharacter::SetCameraZone(bool bEnter, float ArmLength, float FOV)
{
	bInZone = bEnter;
	ZoneArm = ArmLength;
	ZoneFOV = FOV;
}

void ANOARCharacter::UpdateCameraBlend(float DeltaSeconds)
{
	const UNOARSettings* S = GetDefault<UNOARSettings>();
	float WantArm, WantFOV;
	FVector WantSocket;

	if (CameraMode == ENOARCameraMode::FirstPerson)
	{
		WantArm = 0.f;
		WantFOV = S->FirstPersonFOV;
		WantSocket = FVector(0.f, 0.f, 75.f); // eye height ~165 cm: capsule centre 90 + 75
	}
	else
	{
		WantArm = (bInZone ? ZoneArm : S->ThirdPersonArmLength) + UserZoom;
		WantFOV = bInZone ? ZoneFOV : S->ThirdPersonFOV;
		WantSocket = S->ThirdPersonSocketOffset;
	}

	// blend time 0.6 s, ease in/out -> interp speed ~ 1/0.6 * 2.5
	const float Speed = 2.5f / FMath::Max(0.1f, S->CameraModeBlendTime);
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, WantArm, DeltaSeconds, Speed);
	SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, WantSocket, DeltaSeconds, Speed);
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, WantFOV, DeltaSeconds, Speed));
}

void ANOARCharacter::UpdateCharacterFade()
{
	// character dithers out when the camera is close (1P or wall collision); no popping
	const float Threshold = GetDefault<UNOARSettings>()->CharacterFadeArmLength;
	const float Dist = FVector::Dist(Camera->GetComponentLocation(), GetActorLocation() + FVector(0, 0, 40.f));
	const bool bVisible = Dist > Threshold * 0.6f && CameraMode != ENOARCameraMode::FirstPerson;
	GetMesh()->SetVisibility(bVisible, true);
}

// ---------------- movement helpers ----------------

void ANOARCharacter::OnMoveToRequested(FVector WorldLocation)
{
	MoveToAndInteract(WorldLocation);
}

void ANOARCharacter::OnWalkToRequested(FVector WorldLocation)
{
	if (bInputFrozen)
	{
		return;
	}
	FVector Dest = WorldLocation;
	if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation Projected;
		if (Nav->ProjectPointToNavigation(Dest, Projected, FVector(60.f, 60.f, 150.f)))
		{
			Dest = Projected.Location;
		}
	}
	bPendingMove = false;   // plain walk: nothing to interact with on arrival
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), Dest);
}

void ANOARCharacter::Input_KeyLook(const FInputActionValue& Value)
{
	if (bInputFrozen)
	{
		return;
	}
	AController* C = GetController();
	if (!C)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Dt = GetWorld()->GetDeltaSeconds();
	const UNOARSettings* S = GetDefault<UNOARSettings>();
	FRotator R = C->GetControlRotation();
	R.Yaw += Axis.X * KeyLookRate * Dt;
	R.Pitch = FMath::Clamp(FRotator::NormalizeAxis(R.Pitch + Axis.Y * KeyLookRate * 0.6f * Dt), S->PitchMin, S->PitchMax);
	C->SetControlRotation(R);
}

void ANOARCharacter::MoveToAndInteract(FVector WorldLocation)
{
	if (bInputFrozen)
	{
		return;
	}
	// stop ~90 cm short of the point so the character does not stand inside the cabinet
	const FVector ToPoint = (WorldLocation - GetActorLocation()).GetSafeNormal2D();
	FVector Dest = WorldLocation - ToPoint * 90.f;
	if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation Projected;
		if (Nav->ProjectPointToNavigation(Dest, Projected, FVector(50.f, 50.f, 120.f)))
		{
			Dest = Projected.Location;
		}
	}
	PendingMoveTarget = Dest;
	bPendingMove = true;
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), Dest);
}

void ANOARCharacter::SetInputFrozen(bool bFrozen)
{
	bInputFrozen = bFrozen;
	if (bFrozen)
	{
		GetCharacterMovement()->StopMovementImmediately();
		bPendingMove = false;
	}
	Interaction->SetEnabled(!bFrozen);
}

void ANOARCharacter::TeleportToTransform(const FTransform& Transform)
{
	bPendingMove = false;
	SetActorLocationAndRotation(Transform.GetLocation(), Transform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = GetController())
	{
		C->SetControlRotation(Transform.Rotator());
	}
	SpringArm->bEnableCameraLag = false; // reset lag for one frame
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
	{
		SpringArm->bEnableCameraLag = (CameraMode == ENOARCameraMode::ThirdPerson);
	});
}

void ANOARCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCameraBlend(DeltaSeconds);
	UpdateCharacterFade();

	if (bPendingMove && FVector::DistSquared2D(GetActorLocation(), PendingMoveTarget) < FMath::Square(45.f))
	{
		bPendingMove = false;
		Interaction->NotifyArrivedAtPendingTarget();
	}
}
