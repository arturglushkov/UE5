#include "Core/NOARPlayerController.h"
#include "UI/NOARHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Core/NOARGameInstance.h"
#include "Character/NOARCharacter.h"
#include "Interaction/NOARInteractionComponent.h"
#include "Lighting/NOARDayNightManager.h"
#include "NOARFunctionLibrary.h"
#include "NOARCore.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"

ANOARPlayerController::ANOARPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	HUDClass = UNOARHUDWidget::StaticClass();
}

void ANOARPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (HUDClass && IsLocalController())
	{
		HUD = CreateWidget<UNOARHUDWidget>(this, HUDClass);
		if (HUD) HUD->AddToViewport(0);
	}
	ApplyInputModeForCamera(ENOARCameraMode::ThirdPerson);
}

void ANOARPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (ANOARCharacter* C = Cast<ANOARCharacter>(InPawn))
	{
		C->OnCameraModeChanged.AddUniqueDynamic(this, &ANOARPlayerController::HandleCameraModeChanged);
		ApplyInputModeForCamera(C->GetCameraMode());
	}
}

void ANOARPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		return;
	}
	if (IA_Menu)        EIC->BindAction(IA_Menu, ETriggerEvent::Started, this, &ANOARPlayerController::Input_Menu);
	if (IA_Finishes)    EIC->BindAction(IA_Finishes, ETriggerEvent::Started, this, &ANOARPlayerController::Input_Finishes);
	if (IA_Rooms)       EIC->BindAction(IA_Rooms, ETriggerEvent::Started, this, &ANOARPlayerController::Input_Rooms);
	if (IA_DayNight)    EIC->BindAction(IA_DayNight, ETriggerEvent::Started, this, &ANOARPlayerController::Input_DayNight);
	if (IA_PhotoMode)   EIC->BindAction(IA_PhotoMode, ETriggerEvent::Started, this, &ANOARPlayerController::Input_PhotoMode);
	if (IA_ProductInfo) EIC->BindAction(IA_ProductInfo, ETriggerEvent::Started, this, &ANOARPlayerController::Input_ProductInfo);
}

ANOARCharacter* ANOARPlayerController::GetNOARCharacter() const
{
	return Cast<ANOARCharacter>(GetPawn());
}

void ANOARPlayerController::RequestPanel(FName Panel)
{
	OnUIRequest.Broadcast(Panel);
}

void ANOARPlayerController::ApplyInputModeForCamera(ENOARCameraMode Mode)
{
	if (bModal || bPhotoMode)
	{
		return;
	}
	if (Mode == ENOARCameraMode::FirstPerson || bGamepad)
	{
		FInputModeGameOnly M;
		SetInputMode(M);
		bShowMouseCursor = false;
	}
	else
	{
		FInputModeGameAndUI M;
		M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		M.SetHideCursorDuringCapture(true); // hidden while RMB look is held
		SetInputMode(M);
		bShowMouseCursor = true;
	}
}

void ANOARPlayerController::HandleCameraModeChanged(ENOARCameraMode Mode)
{
	ApplyInputModeForCamera(Mode);
}

void ANOARPlayerController::SetUIModal(bool bInModal)
{
	bModal = bInModal;
	UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (bModal)
	{
		FInputModeGameAndUI M;
		M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(M);
		bShowMouseCursor = true;
		if (Sub && UIContext) Sub->AddMappingContext(UIContext, 10);
		if (ANOARCharacter* C = GetNOARCharacter()) C->SetInputFrozen(true);
	}
	else
	{
		if (Sub && UIContext) Sub->RemoveMappingContext(UIContext);
		if (ANOARCharacter* C = GetNOARCharacter())
		{
			C->SetInputFrozen(false);
			ApplyInputModeForCamera(C->GetCameraMode());
		}
	}
}

void ANOARPlayerController::SetPhotoMode(bool bEnabled)
{
	if (bPhotoMode == bEnabled)
	{
		return;
	}
	bPhotoMode = bEnabled;
	UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	UNOARGameInstance* GI = GetGameInstance<UNOARGameInstance>();

	if (bEnabled)
	{
		TierBeforePhoto = GI ? GI->GetQualityTier() : ENOARQualityTier::High;
		UNOARFunctionLibrary::ApplyPhotoModeOverrides(true, TierBeforePhoto);
		if (Sub && PhotoContext) Sub->AddMappingContext(PhotoContext, 20);
		if (ANOARCharacter* C = GetNOARCharacter()) C->SetInputFrozen(true);
		FInputModeGameAndUI M;
		M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(M);
		bShowMouseCursor = true;
		RequestPanel(TEXT("Photo"));
	}
	else
	{
		UNOARFunctionLibrary::ApplyPhotoModeOverrides(false, TierBeforePhoto);
		if (Sub && PhotoContext) Sub->RemoveMappingContext(PhotoContext);
		if (ANOARCharacter* C = GetNOARCharacter())
		{
			C->SetInputFrozen(false);
			ApplyInputModeForCamera(C->GetCameraMode());
		}
		RequestPanel(TEXT("PhotoClosed"));
	}
}

void ANOARPlayerController::NotifyGamepadUsed(bool bInGamepad)
{
	if (bGamepad == bInGamepad)
	{
		return;
	}
	bGamepad = bInGamepad;
	if (ANOARCharacter* C = GetNOARCharacter())
	{
		C->Interaction->InputMode = bGamepad ? ENOARInputMode::Gamepad
			: (C->GetCameraMode() == ENOARCameraMode::FirstPerson ? ENOARInputMode::FirstPerson : ENOARInputMode::Mouse3P);
		ApplyInputModeForCamera(C->GetCameraMode());
	}
}

// ---------------- input ----------------

void ANOARPlayerController::Input_Menu(const FInputActionValue&)
{
	if (bPhotoMode) { SetPhotoMode(false); return; }
	RequestPanel(TEXT("Menu"));
}

void ANOARPlayerController::Input_Finishes(const FInputActionValue&)   { RequestPanel(TEXT("Finishes")); }
void ANOARPlayerController::Input_Rooms(const FInputActionValue&)      { RequestPanel(TEXT("Rooms")); }
void ANOARPlayerController::Input_PhotoMode(const FInputActionValue&)  { SetPhotoMode(!bPhotoMode); }

void ANOARPlayerController::Input_DayNight(const FInputActionValue&)
{
	if (HUD)
	{
		RequestPanel(TEXT("Lighting"));
		return;
	}
	if (TActorIterator<ANOARDayNightManager> It(GetWorld()); It)
	{
		It->CyclePreset();
	}
}

void ANOARPlayerController::Input_ProductInfo(const FInputActionValue&)
{
	if (ANOARCharacter* C = GetNOARCharacter())
	{
		FNOARProductInfo Info;
		if (C->Interaction->GetFocusedProductInfo(Info))
		{
			RequestPanel(TEXT("Product"));
		}
	}
}
