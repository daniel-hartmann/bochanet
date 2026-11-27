#include "Actors/BochaBall.h"

#include "BochaCollision.h"
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
	Sphere->SetCollisionProfileName(BochaCollision::BallProfile);
	// Balls roll around; the referee's NavMesh must not be cut by them.
	Sphere->SetCanEverAffectNavigation(false);

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

TMulticastDelegate<void(ABochaBall*)> ABochaBall::OnBallBeginPlay;
TMulticastDelegate<void(ABochaBall*)> ABochaBall::OnBallEndPlay;

void ABochaBall::BeginPlay()
{
	Super::BeginPlay();
	OnBallBeginPlay.Broadcast(this);

	// Physics runs on the server: only it reacts to collisions between balls.
	if (HasAuthority())
	{
		Sphere->SetNotifyRigidBodyCollision(true);
		Sphere->OnComponentHit.AddUniqueDynamic(this, &ABochaBall::OnSphereHit);
	}
}

void ABochaBall::OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || !Sphere->IsSimulatingPhysics())
	{
		return;
	}

	// The physics engine already resolved the contact (with almost no bounce); the velocities are replaced
	// by the bounce computed from the velocities before the collision (PreStepVelocity). Only the
	// horizontal part: the floor is not a bounce.
	if (ABochaBall* OtherBall = Cast<ABochaBall>(OtherActor))
	{
		// Both balls get this event; the one with the lower id handles the pair.
		if (!OtherBall->GetSphere()->IsSimulatingPhysics() || GetUniqueID() > OtherBall->GetUniqueID())
		{
			return;
		}
		FVector Normal = OtherBall->GetActorLocation() - GetActorLocation();
		Normal.Z = 0.0;
		if (!Normal.Normalize())
		{
			return;
		}
		// Approach speed along the line between the centers; a repeated event for the same contact finds
		// the balls already separating and does nothing.
		const double Approach = FVector::DotProduct(PreStepVelocity - OtherBall->PreStepVelocity, Normal);
		if (Approach <= 0.0)
		{
			return;
		}
		const double InvMass = 1.0 / FMath::Max(Sphere->GetMass(), UE_KINDA_SMALL_NUMBER);
		const double OtherInvMass = 1.0 / FMath::Max(OtherBall->GetSphere()->GetMass(), UE_KINDA_SMALL_NUMBER);
		// Impulse of a collision with restitution e: J = (1 + e) * approach / (1/mA + 1/mB).
		const double Impulse = (1.0 + BallRestitution) * Approach / (InvMass + OtherInvMass);
		const FVector Velocity = Sphere->GetPhysicsLinearVelocity();
		const FVector OtherVelocity = OtherBall->GetSphere()->GetPhysicsLinearVelocity();
		SetBounceVelocity(FVector(PreStepVelocity.X, PreStepVelocity.Y, Velocity.Z) - Normal * (Impulse * InvMass));
		OtherBall->SetBounceVelocity(FVector(OtherBall->PreStepVelocity.X, OtherBall->PreStepVelocity.Y, OtherVelocity.Z)
			+ Normal * (Impulse * OtherInvMass));
		return;
	}

	// A board: the normal points from the board towards the ball and is horizontal (the floor's is vertical).
	FVector Normal = Hit.ImpactNormal;
	if (FMath::Abs(Normal.Z) > 0.5)
	{
		return;
	}
	Normal.Z = 0.0;
	if (!Normal.Normalize())
	{
		return;
	}
	const FVector Before(PreStepVelocity.X, PreStepVelocity.Y, 0.0);
	const double IntoWall = FVector::DotProduct(Before, Normal);
	if (IntoWall >= 0.0)
	{
		return;
	}
	// Reflect the part into the board, scaled by the restitution; the part along the board is kept.
	const FVector After = Before - Normal * ((1.0 + WallRestitution) * IntoWall);
	SetBounceVelocity(FVector(After.X, After.Y, Sphere->GetPhysicsLinearVelocity().Z));
}

void ABochaBall::SetBounceVelocity(const FVector& NewVelocity)
{
	Sphere->WakeAllRigidBodies();
	Sphere->SetPhysicsLinearVelocity(NewVelocity);
	PreStepVelocity = NewVelocity;
	RollAfterHitTicks = 2;
}

void ABochaBall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnBallEndPlay.Broadcast(this);
	Super::EndPlay(EndPlayReason);
}

void ABochaBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickRolling(DeltaSeconds);

	// The velocity this frame's physics step starts from, for the bounces (see OnSphereHit).
	const bool bMoving = HasAuthority() && Sphere->IsSimulatingPhysics() && Sphere->IsAnyRigidBodyAwake();
	PreStepVelocity = bMoving ? Sphere->GetPhysicsLinearVelocity() : FVector::ZeroVector;
}

void ABochaBall::TickRolling(float DeltaSeconds)
{
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

	// After a collision: roll without slipping at the new velocity (w = (Up x v) / r), as in Launch.
	if (RollAfterHitTicks > 0)
	{
		--RollAfterHitTicks;
		Sphere->SetPhysicsAngularVelocityInRadians(FVector::CrossProduct(FVector::UpVector, Horizontal) / GetRadius());
	}

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
		BochaCollision::GroundTrace, Params);
}

float ABochaBall::GetRadius() const
{
	return Sphere->Bounds.SphereRadius;
}
