#include "Actors/BochaBolim.h"

#include "Components/StaticMeshComponent.h"

ABochaBolim::ABochaBolim()
{
	Sphere->SetRelativeScale3D(FVector(0.04));
	Sphere->BodyInstance.SetMassOverride(0.125f);
}
