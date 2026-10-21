#include "Lobby/BochaPortal.h"

#include "DrawDebugHelpers.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Game/BochaMinigameManager.h"
#include "Game/BochaMatch.h"
#include "Game/BochaMinigameManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BochaMinigameCharacter.h"
#include "Player/BochaPlayerController.h"

namespace
{
	ABochaPlayerController* GetLocalBochaController(const AActor* OtherActor)
	{
		const APawn* Pawn = Cast<APawn>(OtherActor);
		return Pawn && Pawn->IsLocallyControlled() ? Cast<ABochaPlayerController>(Pawn->GetController()) : nullptr;
	}
}

ABochaPortal::ABochaPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	MatchClass = ABochaMatch::StaticClass();
}

void ABochaPortal::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (ABochaPlayerController* PlayerController = GetLocalBochaController(OtherActor))
	{
		PlayerController->SetNearbyPortal(this);
	}
}

void ABochaPortal::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (ABochaPlayerController* PlayerController = GetLocalBochaController(OtherActor))
	{
		PlayerController->ClearNearbyPortal();
	}
}

void ABochaPortal::MulticastStartMinigame_Implementation()
{
	if (bMinigameStarted)
	{
		return;
	}
	bMinigameStarted = true;

	if (HasAuthority() && !ABochaMatch::Find(this))
	{
		ABochaMatch* Match = GetWorld()->SpawnActorDeferred<ABochaMatch>(MatchClass, FTransform::Identity);
		Match->SetCourtOrigin(MinigameLevelLocation);
		Match->FinishSpawning(FTransform::Identity);
	}

	bool bSuccess = false;
	MinigameLevelInstance = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		this, MinigameLevel, MinigameLevelLocation, FRotator::ZeroRotator, bSuccess);
	if (MinigameLevelInstance)
	{
		MinigameLevelInstance->OnLevelShown.AddDynamic(this, &ABochaPortal::OnMinigameLevelShown);
	}
}

void ABochaPortal::MulticastEndMinigame_Implementation()
{
	if (!bMinigameStarted)
	{
		return;
	}
	bMinigameStarted = false;

	if (ABochaPlayerController* PlayerController = Cast<ABochaPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (PlayerController->MinigameManager)
		{
			PlayerController->MinigameManager->Destroy();
		}
		if (PlayerController->MinigameCharacter)
		{
			PlayerController->MinigameCharacter->Destroy();
		}
		PlayerController->MinigameManager = nullptr;
		PlayerController->MinigameCharacter = nullptr;
		PlayerController->ResetIgnoreInputFlags();
		PlayerController->SetViewTargetWithBlend(PlayerController->GetPawn(), 0.5f);
	}

	if (MinigameLevelInstance)
	{
		MinigameLevelInstance->SetIsRequestingUnloadAndRemoval(true);
		MinigameLevelInstance = nullptr;
	}
	FlushPersistentDebugLines(GetWorld());

	if (HasAuthority())
	{
		if (ABochaMatch* Match = ABochaMatch::Find(this))
		{
			Match->Destroy();
		}
	}
}

void ABochaPortal::OnMinigameLevelShown()
{
	TArray<AActor*> StartActors;
	UGameplayStatics::GetAllActorsWithTag(this, StartTag, StartActors);
	ABochaPlayerController* PlayerController = Cast<ABochaPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (StartActors.IsEmpty() || !PlayerController)
	{
		return;
	}

	const AActor* Start = StartActors[0];
	ABochaMinigameCharacter* Character = GetWorld()->SpawnActor<ABochaMinigameCharacter>(
		MinigameCharacterClass, Start->GetActorLocation(), Start->GetActorRotation());
	PlayerController->MinigameCharacter = Character;
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);

	ABochaMinigameManager* Manager = GetWorld()->SpawnActorDeferred<ABochaMinigameManager>(MinigameManagerClass, FTransform::Identity);
	Manager->TargetCharacter = Character;
	Manager->FinishSpawning(FTransform::Identity);
	PlayerController->MinigameManager = Manager;

	PlayerController->SetViewTargetWithBlend(Character);
	Character->GameMode = Manager;
}
