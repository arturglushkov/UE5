#pragma once

#include "CoreMinimal.h"
#include "UI/NOARPanelBase.h"
#include "NOARTypes.h"
#include "NOARPanels.generated.h"

class UNOARConfiguratorSubsystem;
class UNOARApartmentSubsystem;
class UNOARLightingSubsystem;
class ANOARDayNightManager;
class UWrapBox;

/** Finishes configurator: one section per Configurable.* slot, swatch buttons per option, price total if enabled. */
UCLASS()
class NOARCORE_API UNOARFinishesPanel : public UNOARPanelBase
{
	GENERATED_BODY()
public:
	UNOARFinishesPanel(const FObjectInitializer& OI);
	virtual void Refresh_Implementation() override;

protected:
	virtual void BuildContent() override;

	UFUNCTION()
	void HandleOption(FName Id, FGameplayTag Tag);
	UFUNCTION()
	void HandleReset(FName Id, FGameplayTag Tag);
	UFUNCTION()
	void HandleConfigChanged(FGameplayTag ChangedSlot, FName OptionID);

	void RebuildList();
	FLinearColor SwatchColorFor(const FNOARFinishOptionRow& Row) const;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> PriceText;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UNOARIdButton>> OptionButtons;   // key: Slot|OptionID
	bool bListBuilt = false;
};

/** Rooms: one button per UNOARRoomData, teleports with a short fade. */
UCLASS()
class NOARCORE_API UNOARRoomsPanel : public UNOARPanelBase
{
	GENERATED_BODY()
public:
	UNOARRoomsPanel(const FObjectInitializer& OI);
	virtual void Refresh_Implementation() override;
protected:
	UFUNCTION()
	void HandleRoom(FName Id, FGameplayTag Tag);
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> List;
};

/** Day / Evening / Night presets + light group toggles of the current zone. */
UCLASS()
class NOARCORE_API UNOARLightingPanel : public UNOARPanelBase
{
	GENERATED_BODY()
public:
	UNOARLightingPanel(const FObjectInitializer& OI);
	virtual void Refresh_Implementation() override;
protected:
	virtual void BuildContent() override;
	UFUNCTION()
	void HandlePreset(FName Id, FGameplayTag Tag);
	UFUNCTION()
	void HandleGroup(FName Id, FGameplayTag Tag);
	UFUNCTION()
	void HandleGroupChanged(FGameplayTag Group, bool bOn, float Dimmer);
	ANOARDayNightManager* FindDayNight() const;

	UPROPERTY(Transient) TMap<FName, TObjectPtr<UNOARIdButton>> PresetButtons;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> GroupList;
	UPROPERTY(Transient) TMap<FGameplayTag, TObjectPtr<UImage>> GroupDots;
};

/** Main menu: resume, camera, quality tier, reset design, quit. */
UCLASS()
class NOARCORE_API UNOARMenuPanel : public UNOARPanelBase
{
	GENERATED_BODY()
public:
	UNOARMenuPanel(const FObjectInitializer& OI);
	virtual void Refresh_Implementation() override;
protected:
	virtual void BuildContent() override;
	UFUNCTION()
	void HandleAction(FName Id, FGameplayTag Tag);
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UNOARIdButton>> QualityButtons;
	UPROPERTY(Transient) TObjectPtr<UNOARIdButton> CameraButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CameraLabel;
};

/** Product card for the focused interactable (I key / Alt+click). */
UCLASS()
class NOARCORE_API UNOARProductPanel : public UNOARPanelBase
{
	GENERATED_BODY()
public:
	UNOARProductPanel(const FObjectInitializer& OI);
	virtual void Refresh_Implementation() override;
	void SetProduct(const FNOARProductInfo& Info);
protected:
	virtual void BuildContent() override;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Rows;
	FNOARProductInfo Current;
};

/** Photo mode overlay: non-modal, bottom-left hint, hides the HUD. */
UCLASS()
class NOARCORE_API UNOARPhotoOverlay : public UNOARPanelBase
{
	GENERATED_BODY()
public:
	UNOARPhotoOverlay(const FObjectInitializer& OI);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
