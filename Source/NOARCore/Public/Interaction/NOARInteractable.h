#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NOARTypes.h"
#include "NOARInteractable.generated.h"

UINTERFACE(BlueprintType, meta = (DisplayName = "NOAR Interactable"))
class NOARCORE_API UNOARInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * BPI_Interactable (Standard v1.1, section 8).
 * Every clickable thing in the apartment implements this and nothing else is ever referenced directly.
 */
class NOARCORE_API INOARInteractable
{
	GENERATED_BODY()

public:
	/** Asked every trace tick while focused. Reason explains the deny. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NOAR|Interaction")
	bool CanInteract(AActor* Instigator, ENOARInteractDenyReason& Reason) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NOAR|Interaction")
	void Focus(AActor* Instigator);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NOAR|Interaction")
	void Unfocus(AActor* Instigator);

	/** Primary action (toggle). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NOAR|Interaction")
	void Interact(AActor* Instigator);

	/** "Open" / "Close" / "Lights" / "Finishes" - localized */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NOAR|Interaction")
	FText GetInteractionText() const;

	/** World position of the indicator dot (handle, switch rocker) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NOAR|Interaction")
	FVector GetInteractionPoint() const;

	/** Product card data; returns false when the object has none */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NOAR|Interaction")
	bool GetProductInfo(FNOARProductInfo& OutInfo) const;
};
