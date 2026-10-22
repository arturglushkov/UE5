#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NOARTypes.h"
#include "NOARCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UNOARInteractionComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNOAROnCameraModeChanged, ENOARCameraMode, Mode);

/**
 * BP_NOAR_Character (Standard v1.1, section 7). Third Person Character with the videogame removed.
 * One camera on a spring arm: 3P = arm 320 cm; 1P = arm blends to 0 at eye height, character fades out.
 * Values come from UNOARSettings; the Blueprint subclass only assigns the mesh, ABP and input assets.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANOARCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UNOARInteractionComponent> Interaction;

	// ---- Enhanced Input assets (assign in BP) ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputMappingContext> WalkContext;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_Look;
	/** Held RMB in 3P enables look; ignored in 1P */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_LookHold;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_FastWalk;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_Interact;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_ToggleCamera;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_Zoom;
	/** Arrow keys turn / tilt the view in both camera modes (trackpad-friendly, no RMB needed) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_KeyLook;
	/** Degrees per second for IA_KeyLook */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	float KeyLookRate = 90.f;

	UPROPERTY(BlueprintAssignable, Category = "NOAR|Events")
	FNOAROnCameraModeChanged OnCameraModeChanged;

	UFUNCTION(BlueprintCallable, Category = "NOAR|Camera")
	void SetCameraMode(ENOARCameraMode Mode);

	UFUNCTION(BlueprintPure, Category = "NOAR|Camera")
	ENOARCameraMode GetCameraMode() const { return CameraMode; }

	UFUNCTION(BlueprintCallable, Category = "NOAR|Camera")
	void ToggleCameraMode();

	/** Camera zones push a temporary arm length / FOV; call with bEnter=false to release */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Camera")
	void SetCameraZone(bool bEnter, float ArmLength, float FOV);

	/** Click-to-walk (Mouse3P): walks to the point, then tells the interaction component it arrived */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Movement")
	void MoveToAndInteract(FVector WorldLocation);

	/** Freeze movement/look (photo mode, menus) */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Movement")
	void SetInputFrozen(bool bFrozen);

	/** Teleport that also resets camera lag */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Movement")
	void TeleportToTransform(const FTransform& Transform);

	UFUNCTION(BlueprintCallable, Category = "NOAR|Settings")
	void ApplySettings();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_LookHoldStart(const FInputActionValue& Value);
	void Input_LookHoldEnd(const FInputActionValue& Value);
	void Input_FastWalkStart(const FInputActionValue& Value);
	void Input_FastWalkEnd(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_ToggleCamera(const FInputActionValue& Value);
	void Input_Zoom(const FInputActionValue& Value);
	void Input_KeyLook(const FInputActionValue& Value);

	UFUNCTION()
	void OnWalkToRequested(FVector WorldLocation);

	UFUNCTION()
	void OnMoveToRequested(FVector WorldLocation);

	void UpdateCameraBlend(float DeltaSeconds);
	void UpdateCharacterFade();

	ENOARCameraMode CameraMode = ENOARCameraMode::ThirdPerson;
	bool bLookHeld = false;
	bool bInputFrozen = false;
	bool bFastWalk = false;

	float TargetArm = 320.f;
	float TargetFOV = 75.f;
	float ZoneArm = 0.f;
	float ZoneFOV = 0.f;
	bool bInZone = false;
	float UserZoom = 0.f; // wheel offset on arm length in 3P, cm

	FVector2D SmoothedLook = FVector2D::ZeroVector;
	FVector PendingMoveTarget = FVector::ZeroVector;
	bool bPendingMove = false;
};
