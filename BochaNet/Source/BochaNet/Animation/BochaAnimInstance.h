#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BochaAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

/**
 * Base of the bocha AnimBlueprints (ABP_BochaPlayer, the referee's): reads the character's movement
 * once per frame and exposes it to the state machines. The variable names match the ones the
 * Mannequin template's AnimGraph (copied into ABP_BochaPlayer) already reads.
 */
UCLASS()
class BOCHANET_API UBochaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Bocha|Animation")
	TObjectPtr<ACharacter> Character;

	UPROPERTY(BlueprintReadOnly, Category = "Bocha|Animation")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Bocha|Animation")
	FVector Velocity = FVector::ZeroVector;

	/** Horizontal speed (cm/s): the blend space's Y axis. */
	UPROPERTY(BlueprintReadOnly, Category = "Bocha|Animation")
	float GroundSpeed = 0.f;

	/** Movement direction relative to where the character faces (degrees): the blend space's X axis. */
	UPROPERTY(BlueprintReadOnly, Category = "Bocha|Animation")
	float Direction = 0.f;

	/** Moving along the ground: Idle <-> Walk/Run. */
	UPROPERTY(BlueprintReadOnly, Category = "Bocha|Animation")
	bool ShouldMove = false;

	/** In the air: Jump / Fall Loop / Land. */
	UPROPERTY(BlueprintReadOnly, Category = "Bocha|Animation")
	bool IsFalling = false;
};
