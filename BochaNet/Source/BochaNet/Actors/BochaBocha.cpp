#include "Actors/BochaBocha.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ABochaBocha::ABochaBocha()
{
	Sphere->SetRelativeScale3D(FVector(0.107));
	Sphere->BodyInstance.SetMassOverride(1.05f);
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
