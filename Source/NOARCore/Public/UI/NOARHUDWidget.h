#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NOARTypes.h"
#include "NOARHUDWidget.generated.h"

class UCanvasPanel;
class UOverlay;
class UBorder;
class UTextBlock;
class UHorizontalBox;
class UNOARPanelBase;
class UNOARIdButton;
class UNOARRoomData;
class ANOARPlayerController;
class UNOARProductPanel;

/**
 * HUD + panel host (Standard v1.1, section 11). Created by ANOARPlayerController (HUDClass) and fully built in code:
 *   top-right toolbar (Finishes / Rooms / Light / Photo / Menu), interaction prompt that follows the focused part,
 *   bottom-left key hints, room toast, and one instance of every panel class routed through OnUIRequest.
 * Subclass in Blueprint (WBP_NOAR_HUD) only to replace panel classes or restyle; logic stays here.
 */
UCLASS()
class NOARCORE_API UNOARHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UNOARHUDWidget(const FObjectInitializer& OI);

	UPROPERTY(EditDefaultsOnly, Category = "NOAR|Panels") TSubclassOf<UNOARPanelBase> FinishesPanelClass;
	UPROPERTY(EditDefaultsOnly, Category = "NOAR|Panels") TSubclassOf<UNOARPanelBase> RoomsPanelClass;
	UPROPERTY(EditDefaultsOnly, Category = "NOAR|Panels") TSubclassOf<UNOARPanelBase> LightingPanelClass;
	UPROPERTY(EditDefaultsOnly, Category = "NOAR|Panels") TSubclassOf<UNOARPanelBase> MenuPanelClass;
	UPROPERTY(EditDefaultsOnly, Category = "NOAR|Panels") TSubclassOf<UNOARPanelBase> ProductPanelClass;
	UPROPERTY(EditDefaultsOnly, Category = "NOAR|Panels") TSubclassOf<UNOARPanelBase> PhotoOverlayClass;

	/** Opens the panel, or closes it if already open. Names: Menu, Finishes, Rooms, Lighting, Product, Photo, PhotoClosed */
	UFUNCTION(BlueprintCallable, Category = "NOAR|UI")
	void TogglePanel(FName Panel);

	UFUNCTION(BlueprintCallable, Category = "NOAR|UI")
	void CloseAllPanels();

	UFUNCTION(BlueprintPure, Category = "NOAR|UI")
	bool IsAnyPanelOpen() const;

	UFUNCTION(BlueprintCallable, Category = "NOAR|UI")
	void ShowToast(const FText& Text, float Seconds = 2.f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION() void HandleUIRequest(FName Panel);
	UFUNCTION() void HandleToolbar(FName Id, FGameplayTag Tag);
	UFUNCTION() void HandlePanelClosed(FName Panel);
	UFUNCTION() void HandleFocusChanged(AActor* FocusedActor, const FText& PromptIn, FVector WorldPoint, ENOARInteractDenyReason Reason);
	UFUNCTION() void HandleRoomEntered(UNOARRoomData* Room);
	UFUNCTION() void HandleCameraMode(ENOARCameraMode Mode);

	UNOARPanelBase* CreatePanel(TSubclassOf<UNOARPanelBase> Class, FName Name);
	UNOARPanelBase* FindPanel(FName Name) const;
	void BindToWorld();
	void SetChromeVisible(bool bVisible);
	void UpdateHints();
	ANOARPlayerController* GetNOARController() const;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TObjectPtr<UOverlay> PanelHost;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> Toolbar;
	UPROPERTY(Transient) TObjectPtr<UBorder> Prompt;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptKey;
	UPROPERTY(Transient) TObjectPtr<UBorder> Hints;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HintsText;
	UPROPERTY(Transient) TObjectPtr<UBorder> Toast;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ToastText;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UNOARPanelBase>> Panels;
	UPROPERTY(Transient) TObjectPtr<UNOARPanelBase> OpenPanel;

	bool bBuilt = false;
	bool bBoundToWorld = false;
	bool bPhoto = false;
	bool bHasFocus = false;
	FVector FocusWorldPoint = FVector::ZeroVector;
	float PromptTarget = 0.f;
	float ToastTimer = 0.f;
	ENOARCameraMode CameraMode = ENOARCameraMode::ThirdPerson;
};
