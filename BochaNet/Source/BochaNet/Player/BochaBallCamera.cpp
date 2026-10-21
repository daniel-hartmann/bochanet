#include "Player/BochaBallCamera.h"

#include "Actors/BochaBall.h"
#include "Camera/CameraComponent.h"

ABochaBallCamera::ABochaBallCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
}

void ABochaBallCamera::Follow(ABochaBall* Ball)
{
	Target = Ball;
	if (Ball)
	{
		SetActorLocation(GetDesiredLocation());
	}
}

void ABochaBallCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const ABochaBall* Ball = Target.Get();
	if (!Ball)
	{
		return;
	}
	const FVector Location = FMath::VInterpTo(GetActorLocation(), GetDesiredLocation(), DeltaSeconds, FollowSpeed);
	const FVector Focus = Ball->GetActorLocation() + LookAhead;
	SetActorLocationAndRotation(Location, (Focus - Location).Rotation());
}

FVector ABochaBallCamera::GetDesiredLocation() const
{
	return Target->GetActorLocation() + Offset;
}
