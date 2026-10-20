#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BochaMinigameCharacter.generated.h"

class ABochaMinigameManager;
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

	/** Slides the character sideways. */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void DoMove(float Scale);

	/** Changes the throw strength by Delta steps. */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void DoStrength(float Delta);

	/** Throws the held ball, then hands the next one out. */
	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void DoThrow();

	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void SetCanThrow(bool bNewValue) { bCanThrow = bNewValue; }

	/** Set by BochaPortal right after it spawns the manager. */
	UPROPERTY(BlueprintReadWrite, Category = "Bocha")
	TObjectPtr<ABochaMinigameManager> GameMode;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MinimapWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bocha")
	bool bCanThrow = true;

	/** Starts at WBP_HUD's slider default; kept within the HUD's strength range. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float ThrowStrength = 200.f;

private:
	/** Sends the throw to the server and removes the held ball. */
	void SendThrow(const FVector& Impulse, bool bIsBolim);
};
