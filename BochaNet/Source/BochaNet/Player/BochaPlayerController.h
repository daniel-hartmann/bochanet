#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BochaPlayerController.generated.h"

class ABochaBocha;
class ABochaBolim;
class ABochaMinigameCharacter;
class ABochaMinigameManager;
class ABochaPortal;
class UInputAction;
class UInputMappingContext;
class UUserWidget;
struct FInputActionValue;

/**
 * Lobby player controller (third person template input) that also drives the bocha minigame:
 * forwards IA_Bocha* to the minigame character and carries the server RPCs.
 */
UCLASS()
class BOCHANET_API ABochaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Asks the server to start the minigame on every machine. */
	UFUNCTION(Server, Reliable)
	void ServerRequestStart(ABochaPortal* Portal);

	/**
	 * Spawns the thrown ball on the server, where it replicates to everyone, launched at Velocity
	 * (cm/s). Ignored (and the client told so) when the match says it is not this player's turn.
	 */
	UFUNCTION(Server, Reliable)
	void ServerThrow(FVector BallLocation, FVector Velocity);

	/** The winner's machine claims the victory; the server checks the score before announcing it. */
	UFUNCTION(Server, Reliable)
	void ServerClaimVictory();

	/** The server refused a throw: put the held ball back as the match says. */
	UFUNCTION(Client, Reliable)
	void ClientThrowRejected();

	void SetNearbyPortal(ABochaPortal* Portal) { NearbyPortal = Portal; }
	void ClearNearbyPortal() { NearbyPortal = nullptr; }

	/** Set by BochaPortal when the minigame starts on this machine. */
	UPROPERTY(BlueprintReadOnly, Category = "Bocha")
	TObjectPtr<ABochaMinigameCharacter> MinigameCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Bocha")
	TObjectPtr<ABochaMinigameManager> MinigameManager;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Skipped with touch controls, which would fight with it. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> MouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> TouchControlsWidgetClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Bocha")
	TObjectPtr<UInputAction> BochaMoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Bocha")
	TObjectPtr<UInputAction> BochaStrengthAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Bocha")
	TObjectPtr<UInputAction> BochaThrowAction;

	/** Mouse X: turns the aim by how much the mouse moved. */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Bocha")
	TObjectPtr<UInputAction> BochaAimMouseAction;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaBolim> BolimClass;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaBocha> BochaClass;

private:
	void InitializeLocalInput();
	bool ShouldUseTouchControls() const;

	void Interact();
	void BochaMove(const FInputActionValue& Value);
	void BochaStrength(const FInputActionValue& Value);
	void BochaThrow();
	void BochaAimMouse(const FInputActionValue& Value);

	/** Portal whose interaction area the possessed pawn is standing in. */
	UPROPERTY(Transient)
	TObjectPtr<ABochaPortal> NearbyPortal;
};
