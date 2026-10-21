#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BochaMinigameCharacter.generated.h"

class ABochaMinigameManager;
class UAnimInstance;
class UAnimMontage;
class USkeletalMesh;
class UCameraComponent;
class UInputMappingContext;
class USpringArmComponent;
class UUserWidget;

/**
 * The player on the bocha court. Not possessed: the player controller forwards the
 * IA_Bocha* actions to DoMove / DoStrength / DoThrow.
 */
UCLASS()
class BOCHANET_API ABochaMinigameCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABochaMinigameCharacter();

	/** Slides the character sideways, across the court. */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void DoMove(float Scale);

	/** Turns the character (and the throw) left (< 0) or right (> 0) at AimSpeed, within MaxAimAngle. */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void DoAim(float Direction);

	/** Turns the aim by a mouse movement, within MaxAimAngle. */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void DoAimMouse(float MouseDelta);

	/** Changes the throw strength by Delta steps. */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void DoStrength(float Delta);

	/**
	 * Starts the throw of the held ball, if the match gave one (only on the local player's turn):
	 * plays ThrowMontage, whose "Soltar Bola" notify calls ReleaseBall.
	 */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void DoThrow();

	/** Lets go of the held ball, in the direction and with the strength of this moment. */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void ReleaseBall();

	/** Dresses the character for its team: Manny for red (0), Quinn for blue (1). */
	void SetTeam(int32 Team);

	float GetThrowStrength() const { return ThrowStrength; }

	/** Set by BochaPortal right after it spawns the manager. */
	UPROPERTY(BlueprintReadWrite, Category = "Bocha")
	TObjectPtr<ABochaMinigameManager> GameMode;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MinimapWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<USkeletalMesh> RedMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<USkeletalMesh> BlueMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> AnimClass;

	/** Throw gesture; without it (or without a mesh) the ball leaves right away. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> ThrowMontage;

	/** Launch speed (cm/s), within the HUD's strength range. 750 rolls a ball ~13 m. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float ThrowStrength = 750.f;

	/** Largest aim angle to either side (degrees). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float MaxAimAngle = 30.f;

	/** Aim turn rate with Q/E (degrees per second). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float AimSpeed = 30.f;

	/** Aim degrees per unit of mouse movement. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float MouseAimSensitivity = 0.1f;

private:
	/** Sends the throw to the server and removes the held ball. */
	void SendThrow(const FVector& Velocity);

	void TurnAim(float DeltaYaw);

	/** Safety net: if the montage ends before its notify (interrupted), the ball still leaves. */
	UFUNCTION()
	void OnThrowMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	bool bThrowing = false;
	int32 DressedTeam = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Minimap;

	/** Spawn rotation: straight down the court. */
	FRotator BaseRotation;
	float AimYaw = 0.f;
};
