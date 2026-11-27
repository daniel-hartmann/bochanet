#include "Game/BochaGameMode.h"

#include "Engine/World.h"
#include "Game/BochaGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Player/BochaPlayerState.h"

ABochaGameMode::ABochaGameMode()
{
	GameStateClass = ABochaGameState::StaticClass();
	PlayerStateClass = ABochaPlayerState::StaticClass();
}

void ABochaGameMode::InitGameState()
{
	Super::InitGameState();
	UpdatePlayerCount();
}

void ABochaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UpdatePlayerCount();
}

void ABochaGameMode::Logout(AController* Exiting)
{
	UpdatePlayerCount(Exiting);
	Super::Logout(Exiting);
}

void ABochaGameMode::UpdatePlayerCount(const AController* Exiting)
{
	ABochaGameState* BochaGameState = GetGameState<ABochaGameState>();
	if (!BochaGameState)
	{
		return;
	}
	int32 Connected = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController != Exiting && PlayerController->PlayerState
			&& !PlayerController->PlayerState->IsABot())
		{
			++Connected;
		}
	}
	BochaGameState->SetPlayerCount(Connected, RequiredPlayers);
}
