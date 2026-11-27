#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaBall.generated.h"

class UStaticMeshComponent;

/** Base for every ball on the court: a replicated physics sphere. */
UCLASS()
class BOCHANET_API ABochaBall : public AActor
{
	GENERATED_BODY()

public:
	ABochaBall();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Every ball, on every machine, as it enters and leaves play (filter by world). */
	static TMulticastDelegate<void(ABochaBall*)> OnBallBeginPlay;
	static TMulticastDelegate<void(ABochaBall*)> OnBallEndPlay;

	UStaticMeshComponent* GetSphere() const { return Sphere; }

	/** Radius in world units. */
	float GetRadius() const;

	/** Something (floor or another ball) is right below the ball. */
	bool IsOnGround() const;

	/**
	 * Server: rolls the ball along the floor from Location (its bottom is put on FloorZ) at
	 * Velocity (horizontal, cm/s), already spinning as it rolls so it does not skid.
	 */
	void Launch(const FVector& Location, double FloorZ, const FVector& Velocity);

	/**
	 * Server: once rolling, brake so the ball stops exactly at court position WorldX (the court
	 * runs along +X). It must be launched fast enough to get there.
	 */
	void SetStopTarget(double WorldX);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bocha")
	TObjectPtr<UStaticMeshComponent> Sphere;

	/**
	 * Rolling resistance of the court carpet: speed lost per second while rolling (cm/s²).
	 * A ball launched at v stops after v² / (2 * RollingDeceleration): 1000 cm/s -> ~24 m.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha|Physics")
	float RollingDeceleration = 210.f;

	/** Below this speed (cm/s) a rolling ball stops dead. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha|Physics")
	float StopSpeed = 5.f;

	/**
	 * Bounce between two balls (coefficient of restitution): 0 = they stop together, 1 = perfectly elastic.
	 * Computed here instead of by the physics engine, which ignores bounces below BounceThresholdVelocity
	 * (2 m/s by default, faster than most hits on the court). The ball with the lower id of the pair uses
	 * its own value.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha|Physics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BallRestitution = 0.9f;

	/** Bounce off the boards around the court (0 = stops dead against it, 1 = perfectly elastic). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha|Physics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallRestitution = 0.7f;

private:
	void StopRolling();

	/** Server: rolling resistance and the braking to a stop target. */
	void TickRolling(float DeltaSeconds);

	/** Server: bounces off another ball or a board (see BallRestitution, WallRestitution), then rolls again. */
	UFUNCTION()
	void OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& Hit);

	bool bHasStopTarget = false;

	/**
	 * After a bounce: for the next ticks the spin is matched to the new velocity. A collision only changes the
	 * linear velocity, so the ball would skid and the floor's sliding friction would stop it in a few centimeters.
	 */
	int32 RollAfterHitTicks = 0;

	/** Velocity at the end of the last tick: the velocity before this frame's collisions. */
	FVector PreStepVelocity = FVector::ZeroVector;

	/** Server: sets the velocity after a bounce (awake, rolling again on the next ticks). */
	void SetBounceVelocity(const FVector& NewVelocity);
	double StopTargetX = 0.0;
};
