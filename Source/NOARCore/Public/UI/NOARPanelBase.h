#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "GameplayTagContainer.h"
#include "NOARPanelBase.generated.h"

class UOverlay;
class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UScrollBox;
class UImage;
class ANOARPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNOAROnButtonId, FName, Id, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNOAROnPanelClosed, FName, Panel);

/** UButton that reports an Id/Tag on click so panels can build lists without one UFUNCTION per button. */
UCLASS()
class NOARCORE_API UNOARIdButton : public UButton
{
	GENERATED_BODY()
public:
	UNOARIdButton();

	UPROPERTY(BlueprintReadOnly, Category = "NOAR")
	FName Id;
	UPROPERTY(BlueprintReadOnly, Category = "NOAR")
	FGameplayTag Tag;

	UPROPERTY(BlueprintAssignable, Category = "NOAR")
	FNOAROnButtonId OnPressedId;

	void SetSelected(bool bSelected);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void HandleClicked();
};

/**
 * Base for every NOAR panel (section 11). Builds its own tree in code: dark glass card with title, close button and a
 * content box that subclasses fill in BuildContent(). Fades in/out; modal panels freeze walking through the controller.
 */
UCLASS(Abstract)
class NOARCORE_API UNOARPanelBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UNOARPanelBase(const FObjectInitializer& OI) : Super(OI) {}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Panel")
	FName PanelName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Panel")
	FText Title;

	/** Modal: cursor on, character frozen, UI input context */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Panel")
	bool bModal = true;

	/** Card width in slate units; 0 = fill */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Panel")
	float CardWidth = 520.f;

	/** Right side of the screen (finishes) or centred (menu) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NOAR|Panel")
	bool bDockRight = true;

	UPROPERTY(BlueprintAssignable, Category = "NOAR|Panel")
	FNOAROnPanelClosed OnClosed;

	UFUNCTION(BlueprintCallable, Category = "NOAR|Panel")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "NOAR|Panel")
	void Close();

	UFUNCTION(BlueprintPure, Category = "NOAR|Panel")
	bool IsOpen() const { return bOpen; }

	/** Called on every Open so lists reflect the current state */
	UFUNCTION(BlueprintNativeEvent, Category = "NOAR|Panel")
	void Refresh();
	virtual void Refresh_Implementation() {}

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Subclasses add their widgets to Content here (called once) */
	virtual void BuildContent() {}

	// ---- helpers for subclasses
	UTextBlock* MakeText(const FText& Text, int32 Size = 14, const FLinearColor& Color = FLinearColor::White, const FName& Face = TEXT("Regular"));
	UNOARIdButton* MakeButton(const FText& Label, FName Id, FGameplayTag Tag = FGameplayTag(), bool bAccent = false, int32 FontSize = 14);
	UBorder* MakeSection(const FText& Heading);
	UHorizontalBox* MakeRow();
	UImage* MakeSwatch(const FLinearColor& Color, float Size = 28.f);
	ANOARPlayerController* GetNOARController() const;

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient) TObjectPtr<UOverlay> Root;
	UPROPERTY(Transient) TObjectPtr<UBorder> Card;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Content;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;

	/** Visibility used while open (overlays use HitTestInvisible) */
	ESlateVisibility OpenVisibility = ESlateVisibility::Visible;

	bool bBuilt = false;
	bool bOpen = false;
	float FadeTarget = 0.f;
	float FadeSpeed = 6.f;
};
