#include "Actors/BochaBocha.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABochaBocha::ABochaBocha()
{
	Sphere->SetRelativeScale3D(FVector(0.107));
	Sphere->BodyInstance.SetMassOverride(1.05f);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedMaterial(TEXT("/Game/Bocha/Materials/M_BallRed.M_BallRed"));
	Sphere->SetMaterial(0, RedMaterial.Object);
}

void ABochaBocha::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABochaBocha, bBlue);
}

void ABochaBocha::BeginPlay()
{
	Super::BeginPlay();

	ApplyColor();
}

void ABochaBocha::OnRep_bBlue()
{
	ApplyColor();
}

void ABochaBocha::ApplyColor()
{
	if (bBlue && BlueMaterial)
	{
		Sphere->SetMaterial(0, BlueMaterial);
	}
}
