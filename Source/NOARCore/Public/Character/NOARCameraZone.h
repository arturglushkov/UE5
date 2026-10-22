#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NOARCameraZone.generated.h"

class UBoxComponent;

/** BPV_CameraZone: corridors, WIC, bathrooms - shorter arm and wider FOV while the character is inside (section 7). */
UCLASS(Blueprintable)
class NOARCORE_API ANOARCameraZone : public AActor
{
	GENERATED_BODY()

public:
	ANOARCameraZone();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NOAR|Components")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Camera")
	float ArmLength = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Camera")
	float FOV = 80.f;

	/** Show the "V - first person" hint when entering (WIC, bathrooms) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NOAR|Camera")
	bool bSuggestFirstPerson = false;

protected:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);
};
