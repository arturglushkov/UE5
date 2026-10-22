#include "Character/NOARCameraZone.h"
#include "Character/NOARCharacter.h"
#include "Components/BoxComponent.h"

ANOARCameraZone::ANOARCameraZone()
{
	PrimaryActorTick.bCanEverTick = false;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);
	Box->SetBoxExtent(FVector(150.f, 150.f, 150.f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Box->SetGenerateOverlapEvents(true);
	Box->OnComponentBeginOverlap.AddDynamic(this, &ANOARCameraZone::OnBeginOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &ANOARCameraZone::OnEndOverlap);
}

void ANOARCameraZone::OnBeginOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (ANOARCharacter* C = Cast<ANOARCharacter>(Other))
	{
		C->SetCameraZone(true, ArmLength, FOV);
	}
}

void ANOARCameraZone::OnEndOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (ANOARCharacter* C = Cast<ANOARCharacter>(Other))
	{
		C->SetCameraZone(false, 0.f, 0.f);
	}
}
