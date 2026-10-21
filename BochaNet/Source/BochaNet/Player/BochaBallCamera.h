#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaBallCamera.generated.h"

class ABochaBall;
class UCameraComponent;
class USpringArmComponent;

/**
 * Local camera that chases a thrown ball: the actor sits on the ball and a spring arm (with lag
 * and collision) keeps the camera behind and above it, looking down the court.
 */
UCLASS(NotPlaceable, Transient)
class BOCHANET_API ABochaBallCamera : public AActor
{
	GENERATED_BODY()

public:
	ABochaBallCamera();

	virtual void Tick(float DeltaSeconds) override;

	/** Starts chasing Ball. */
	void Follow(ABochaBall* Ball);

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	TWeakObjectPtr<ABochaBall> Target;
};
