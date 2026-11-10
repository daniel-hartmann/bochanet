#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BochaSoloGameMode.generated.h"

class ABochaMatch;
class ABochaMinigameCharacter;
class ABochaMinigameManager;

/**
 * Game mode of Lvl_Cancha opened on its own (not streamed in by the portal): starts a match where the
 * single player throws for both teams. Set for the map in DefaultEngine.ini (GameModeMapPrefixes).
 */
UCLASS()
class BOCHANET_API ABochaSoloGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABochaSoloGameMode();

	virtual void StartPlay() override;

protected:
	/** The player only exists as the minigame character, spawned by StartPlay. */
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override { return false; }

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaMatch> MatchClass;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaMinigameCharacter> MinigameCharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TSubclassOf<ABochaMinigameManager> MinigameManagerClass;

	/** Tag of the actor where the minigame character is spawned. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	FName StartTag = TEXT("CanchaMapStart");
};
