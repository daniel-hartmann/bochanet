#include "Actors/BochaBall.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// How far below the ball's bottom something still counts as ground (cm).
	constexpr float GroundTolerance = 2.f;
}

ABochaBall::ABochaBall()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicatingMovement(true);

	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	RootComponent = Sphere;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Sphere->SetStaticMesh(SphereMesh.Object);
	Sphere->SetMaterial(0, BasicMaterial.Object);
	// No damping: it keeps the ball spinning slower than it rolls, so it slides and the floor's full
	// sliding friction brakes it hard. Rolling resistance is applied in Tick instead.
	Sphere->BodyInstance.AngularDamping = 0.f;
	Sphere->BodyInstance.LinearDamping = 0.f;
	// The project cap (3600 deg/s) lets a 4 cm jack roll at only ~1.25 m/s; faster, it slides.
	Sphere->BodyInstance.bOverrideMaxAngularVelocity = true;
	Sphere->BodyInstance.MaxAngularVelocity = 100000.f;
}

void ABochaBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Physics runs on the server; clients get the replicated movement.
	if (!HasAuthority() || !Sphere->IsSimulatingPhysics() || !Sphere->IsAnyRigidBodyAwake())
	{
		return;
	}

	// Rolling resistance only applies on the ground; in the air (or right after the throw,
	// before physics applied the launch speed) the ball is left alone.
	if (!IsOnGround())
	{
		return;
	}

	const FVector Velocity = Sphere->GetPhysicsLinearVelocity();

	const FVector Horizontal(Velocity.X, Velocity.Y, 0.0);
	const double Speed = Horizontal.Size();
	const double Remaining = StopTargetX - GetActorLocation().X;
	if (Speed <= StopSpeed || (bHasStopTarget && Remaining <= 0.0))
	{
		StopRolling();
		return;
	}

	// With a target, the braking that stops the ball right there: v² = 2·a·d.
	const double Deceleration = bHasStopTarget ? Speed * Speed / (2.0 * Remaining) : RollingDeceleration;

	// Slow the spin by the same ratio, or friction with the floor would speed the ball back up.
	const double Ratio = FMath::Max(0.0, Speed - Deceleration * DeltaSeconds) / Speed;
	Sphere->SetPhysicsLinearVelocity(Horizontal * Ratio + FVector(0.0, 0.0, Velocity.Z));
	Sphere->SetPhysicsAngularVelocityInDegrees(Sphere->GetPhysicsAngularVelocityInDegrees() * Ratio);
}

void ABochaBall::Launch(const FVector& Location, double FloorZ, const FVector& Velocity)
{
	const FVector Horizontal(Velocity.X, Velocity.Y, 0.0);
	SetActorLocation(FVector(Location.X, Location.Y, FloorZ + GetRadius() + 0.5));
	Sphere->SetSimulatePhysics(true);
	Sphere->SetPhysicsLinearVelocity(Horizontal);
	// Rolling without slipping: the contact point is at rest, so w = (v / r) * (Up x direction).
	const FVector Spin = FVector::CrossProduct(FVector::UpVector, Horizontal) / GetRadius();
	Sphere->SetPhysicsAngularVelocityInRadians(Spin);
}

void ABochaBall::SetStopTarget(double WorldX)
{
	bHasStopTarget = true;
	StopTargetX = WorldX;
}

void ABochaBall::StopRolling()
{
	bHasStopTarget = false;
	Sphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Sphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Sphere->PutAllRigidBodiesToSleep();
}

bool ABochaBall::IsOnGround() const
{
	const FVector Center = GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BochaBallGround), false, this);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Center, Center - FVector(0.0, 0.0, GetRadius() + GroundTolerance),
		ECC_Visibility, Params);
}

float ABochaBall::GetRadius() const
{
	return Sphere->Bounds.SphereRadius;
}
