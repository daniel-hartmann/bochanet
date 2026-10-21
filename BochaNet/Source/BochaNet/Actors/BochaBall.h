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

private:
	void StopRolling();

	bool bHasStopTarget = false;
	double StopTargetX = 0.0;
};
