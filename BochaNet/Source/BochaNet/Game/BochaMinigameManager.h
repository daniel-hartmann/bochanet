#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaMinigameManager.generated.h"

class ABochaBall;
class ABochaBocha;
class ABochaBolim;
class ACharacter;
class UBochaHUDWidget;

/**
 * Runs one bocha match on the local machine: hands balls to the minigame character and alternates turns.
 * BochaPortal spawns one on every machine when the minigame starts.
 */
UCLASS()
class BOCHANET_API ABochaMinigameManager : public AActor
{
	GENERATED_BODY()

public:
	ABochaMinigameManager();

	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void NotifyBallThrown();

	UFUNCTION(BlueprintCallable, Category = "Bocha")
	void NotifyBolimThrown();

	ABochaBall* GetCurrentBall() const { return CurrentBall; }
	UBochaHUDWidget* GetHUD() const { return HUD; }

	/** Character that holds the balls; falls back to player 0's character when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bocha", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<ACharacter> TargetCharacter;

	/** Ball currently held by the character, waiting to be thrown. */
	UPROPERTY(BlueprintReadOnly, Category = "Bocha")
	TObjectPtr<ABochaBall> CurrentBall;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaBolim> BolimClass;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaBocha> BochaClass;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<UBochaHUDWidget> HUDWidgetClass;

	/** 0 or 1; player 1 plays the blue balls. */
	UPROPERTY(VisibleInstanceOnly, Category = "Bocha")
	int32 CurrentPlayer = 0;

private:
	void SpawnBall(bool bIsBlue);
	void ToggleCurrentPlayer();
	void HoldBall(ABochaBall* Ball);

	UPROPERTY(Transient)
	TObjectPtr<UBochaHUDWidget> HUD;
};
