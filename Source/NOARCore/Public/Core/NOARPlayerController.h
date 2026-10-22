#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NOARTypes.h"
#include "NOARPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class ANOARCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNOAROnUIRequest, FName, Panel);

/**
 * BP_NOAR_PlayerController (Standard v1.1, section 11): input contexts, cursor, UI requests, photo mode.
 * UI widgets are created by the Blueprint subclass (WBP_*) in response to OnUIRequest; C++ never references UMG classes.
 */
UCLASS(Blueprintable)
class NOARCORE_API ANOARPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANOARPlayerController();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputMappingContext> UIContext;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputMappingContext> PhotoContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_Menu;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_Finishes;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_Rooms;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_DayNight;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_PhotoMode;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Input")
	TObjectPtr<UInputAction> IA_ProductInfo;

	/** Panel names: "Menu", "Finishes", "Rooms", "Photo", "Product", "Settings" */
	UPROPERTY(BlueprintAssignable, Category = "NOAR|UI")
	FNOAROnUIRequest OnUIRequest;

	UFUNCTION(BlueprintCallable, Category = "NOAR|UI")
	void RequestPanel(FName Panel);

	/** Game And UI with free cursor (3P) or Game Only (1P) */
	UFUNCTION(BlueprintCallable, Category = "NOAR|UI")
	void ApplyInputModeForCamera(ENOARCameraMode Mode);

	/** A modal panel is open: cursor on, walking frozen */
	UFUNCTION(BlueprintCallable, Category = "NOAR|UI")
	void SetUIModal(bool bModal);

	UFUNCTION(BlueprintCallable, Category = "NOAR|Photo")
	void SetPhotoMode(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "NOAR|Photo")
	bool IsPhotoMode() const { return bPhotoMode; }

	UFUNCTION(BlueprintPure, Category = "NOAR")
	ANOARCharacter* GetNOARCharacter() const;

	/** Detects gamepad use and switches prompts / interaction mode */
	UFUNCTION(BlueprintCallable, Category = "NOAR|Input")
	void NotifyGamepadUsed(bool bGamepad);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	void Input_Menu(const FInputActionValue&);
	void Input_Finishes(const FInputActionValue&);
	void Input_Rooms(const FInputActionValue&);
	void Input_DayNight(const FInputActionValue&);
	void Input_PhotoMode(const FInputActionValue&);
	void Input_ProductInfo(const FInputActionValue&);

	UFUNCTION()
	void HandleCameraModeChanged(ENOARCameraMode Mode);

	bool bPhotoMode = false;
	bool bModal = false;
	bool bGamepad = false;
	ENOARQualityTier TierBeforePhoto = ENOARQualityTier::High;
};
