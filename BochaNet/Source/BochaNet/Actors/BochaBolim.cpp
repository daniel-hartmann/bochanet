#include "Actors/BochaBolim.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABochaBolim::ABochaBolim()
{
	Sphere->SetRelativeScale3D(FVector(0.04));
	Sphere->BodyInstance.SetMassOverride(0.125f);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BolimMaterial(TEXT("/Game/Bocha/Materials/M_BallBolim.M_BallBolim"));
	Sphere->SetMaterial(0, BolimMaterial.Object);
}
