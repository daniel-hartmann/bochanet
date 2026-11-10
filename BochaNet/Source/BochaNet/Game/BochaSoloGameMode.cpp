#include "Game/BochaSoloGameMode.h"

#include "Game/BochaMatch.h"
#include "Game/BochaMinigameManager.h"
#include "Lobby/BochaPortal.h"
#include "Player/BochaMinigameCharacter.h"
#include "UObject/ConstructorHelpers.h"

ABochaSoloGameMode::ABochaSoloGameMode()
{
	DefaultPawnClass = nullptr;

	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBP(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	static ConstructorHelpers::FClassFinder<ABochaMatch> MatchBP(TEXT("/Game/Bocha/Blueprints/BP_BochaMatch"));
	static ConstructorHelpers::FClassFinder<ABochaMinigameCharacter> CharacterBP(TEXT("/Game/Bocha/Blueprints/BP_BochaPlayerCharacter"));
	static ConstructorHelpers::FClassFinder<ABochaMinigameManager> ManagerBP(TEXT("/Game/Bocha/Blueprints/BP_BochaMinigameManager"));
	PlayerControllerClass = PlayerControllerBP.Class;
	MatchClass = MatchBP.Class;
	MinigameCharacterClass = CharacterBP.Class;
	MinigameManagerClass = ManagerBP.Class;
}

void ABochaSoloGameMode::StartPlay()
{
	Super::StartPlay();

	// The court sits at the origin of this map. With one player, the match gives them both teams.
	GetWorld()->SpawnActor<ABochaMatch>(MatchClass);
	ABochaPortal::SetUpLocalPlayer(GetWorld(), StartTag, MinigameCharacterClass, MinigameManagerClass);
}
