#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaPortal.generated.h"

class ABochaMatch;
class ABochaMinigameCharacter;
class ABochaMinigameManager;

/**
 * Lobby object that starts the bocha minigame. Interacting asks the server, which starts it
 * on every machine: each one streams in the court and puts its local player in it.
 * Its collision (InteractionArea in BochaPortal) marks where the player can interact.
 */
UCLASS()
class BOCHANET_API ABochaPortal : public AActor
{
	GENERATED_BODY()

public:
	ABochaPortal();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartMinigame();

	/** Puts every player back in the lobby and lets the portal start a new match. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEndMinigame();

	bool HasMinigameStarted() const { return bMinigameStarted; }

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSoftObjectPtr<UWorld> MinigameLevel;

	/** Where the court is streamed in. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	FVector MinigameLevelLocation = FVector(5000.0, 0.0, 0.0);

	/** Tag of the actor in MinigameLevel where the minigame character is spawned. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	FName StartTag = TEXT("CanchaMapStart");

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaMinigameCharacter> MinigameCharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaMinigameManager> MinigameManagerClass;

	/** Spawned by the server; holds the rules state both players share. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaMatch> MatchClass;

private:
	UFUNCTION()
	void OnMinigameLevelShown();

	UPROPERTY(VisibleInstanceOnly, Category = "Bocha")
	bool bMinigameStarted = false;

	/** The court loaded on this machine. */
	UPROPERTY(Transient)
	TObjectPtr<class ULevelStreamingDynamic> MinigameLevelInstance;
};
