#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BochaGameMode.generated.h"

/**
 * Server-only game mode of the lobby (parent of BP_ThirdPersonGameMode, which sets the pawn and the
 * player controller) and of BochaSoloGameMode: picks ABochaGameState and ABochaPlayerState and counts
 * the players, so the match only starts with RequiredPlayers connected.
 */
UCLASS()
class BOCHANET_API ABochaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABochaGameMode();

	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:
	/** Players needed to start a match at the portal (1 to test alone in PIE: the player throws for both teams). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha", meta = (ClampMin = "1", ClampMax = "2"))
	int32 RequiredPlayers = 2;

private:
	/** Counts the connected players into ABochaGameState, leaving Exiting out. */
	void UpdatePlayerCount(const AController* Exiting = nullptr);
};
