#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaMinigameManager.generated.h"

class ABochaBall;
class ABochaBocha;
class ABochaBolim;
class ABochaBallCamera;
class ABochaMatch;
class ACharacter;
class UBochaHUDWidget;

/**
 * Local side of the bocha match on this machine: shows the HUD and puts a ball in the minigame
 * character's hand while the match says it is the local player's turn.
 * BochaPortal spawns one on every machine when the minigame starts.
 */
UCLASS()
class BOCHANET_API ABochaMinigameManager : public AActor
{
	GENERATED_BODY()

public:
	ABochaMinigameManager();

	/** Starts following Match (called by whichever of the two begins play last). */
	void SetMatch(ABochaMatch* NewMatch);

	/** Re-reads the match state: held ball and HUD. */
	void Refresh();

	ABochaMatch* GetMatch() const { return Match; }
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaBolim> BolimClass;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaBocha> BochaClass;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<UBochaHUDWidget> HUDWidgetClass;

	/** Blend time when the view switches to the thrown ball and back (s). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float CameraBlendTime = 0.5f;

private:
	void RefreshHeldBall(bool bMyTurn);
	/** While a thrown ball rolls, the view chases it; then it goes back to the minigame character. */
	void RefreshCamera();
	void ShowGameOver(int32 WinnerTeam, int32 RedScore, int32 BlueScore);
	void HoldBall(ABochaBall* Ball);

	UPROPERTY(Transient)
	TObjectPtr<ABochaMatch> Match;

	UPROPERTY(Transient)
	TObjectPtr<UBochaHUDWidget> HUD;

	UPROPERTY(Transient)
	TObjectPtr<ABochaBallCamera> BallCamera;

	UPROPERTY(Transient)
	TObjectPtr<ABochaBall> FollowedBall;

	bool bClaimedVictory = false;
};
