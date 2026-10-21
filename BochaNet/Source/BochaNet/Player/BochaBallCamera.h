#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaBallCamera.generated.h"

class ABochaBall;
class UCameraComponent;

/** Local camera that chases a thrown ball from behind and above, looking down the court. */
UCLASS(NotPlaceable, Transient)
class BOCHANET_API ABochaBallCamera : public AActor
{
	GENERATED_BODY()

public:
	ABochaBallCamera();

	virtual void Tick(float DeltaSeconds) override;

	/** Starts chasing Ball, jumping straight behind it. */
	void Follow(ABochaBall* Ball);

protected:
	/** Camera position relative to the ball (court axes: X down the court). */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector Offset = FVector(-260.0, 0.0, 150.0);

	/** The camera looks at this point ahead of the ball. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector LookAhead = FVector(150.0, 0.0, 0.0);

	/** How quickly the camera catches up with the ball. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float FollowSpeed = 5.f;

private:
	FVector GetDesiredLocation() const;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	TWeakObjectPtr<ABochaBall> Target;
};
