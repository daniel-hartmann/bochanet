#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"
#include "BochaReferee.generated.h"

class UAnimInstance;
class UAnimMontage;
class UMaterialInterface;
class USkeletalMesh;
struct FAIRequestID;

/**
 * Referee NPC: an AI-controlled character (server) that walks the court's NavMesh to the jack after
 * each bocha, crouches to measure (AM_BochaMeasure) and walks back. Its "Medir" anim notify tells
 * the match the point can be announced. It walks through the balls (BochaReferee collision preset).
 */
UCLASS()
class BOCHANET_API ABochaReferee : public ACharacter
{
	GENERATED_BODY()

public:
	ABochaReferee();

	/** Server: walks to Location along the NavMesh; OnArrived fires when done (or when it cannot). */
	void WalkTo(const FVector& Location);

	/** Server: plays the measuring gesture on every machine; OnMeasured fires at its notify. */
	void Measure();

	/** Called by the "Medir" anim notify on every machine. */
	void OnMeasureNotify();

	FSimpleDelegate OnArrived;
	FSimpleDelegate OnMeasured;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(EditDefaultsOnly, Category = "Referee")
	TObjectPtr<USkeletalMesh> RefereeMesh;

	/** Applied to every material slot, so the referee stands out from the players. */
	UPROPERTY(EditDefaultsOnly, Category = "Referee")
	TObjectPtr<UMaterialInterface> RefereeMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Referee")
	TSubclassOf<UAnimInstance> AnimClass;

	UPROPERTY(EditDefaultsOnly, Category = "Referee")
	TObjectPtr<UAnimMontage> MeasureMontage;

	/** How close to the target counts as arrived (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Referee")
	float AcceptanceRadius = 30.f;

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayMeasure();

	UFUNCTION()
	void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	/** The walk OnArrived waits for; older walks cut short by a new order are ignored. */
	FAIRequestID CurrentWalk;
};
