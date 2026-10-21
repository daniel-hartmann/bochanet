#include "Game/BochaMinigameManager.h"

#include "Actors/BochaBocha.h"
#include "Actors/BochaBolim.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "UI/BochaHUDWidget.h"

namespace
{
	// Where a held ball sits relative to the character.
	const FVector HeldBallOffset(100.0, 0.0, -30.0);
}

ABochaMinigameManager::ABochaMinigameManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABochaMinigameManager::BeginPlay()
{
	Super::BeginPlay();

	// The match starts with the jack.
	ABochaBolim* Bolim = GetWorld()->SpawnActor<ABochaBolim>(BolimClass, FTransform::Identity);
	Bolim->SetReplicates(false);
	CurrentBall = Bolim;
	HoldBall(Bolim);

	HUD = CreateWidget<UBochaHUDWidget>(GetWorld(), HUDWidgetClass);
	HUD->AddToViewport();
}

void ABochaMinigameManager::NotifyBallThrown()
{
	SpawnBall();
}

void ABochaMinigameManager::NotifyBolimThrown()
{
	// TODO: wait for the jack to stop before handing out the first ball.
	SpawnBall();
}

void ABochaMinigameManager::SpawnBall()
{
	ABochaBocha* Bocha = GetWorld()->SpawnActorDeferred<ABochaBocha>(BochaClass, FTransform::Identity);
	// The host plays the red balls and the client the blue ones, same rule as ServerThrow.
	Bocha->bBlue = GetNetMode() == NM_Client;
	Bocha->FinishSpawning(FTransform::Identity);
	Bocha->SetReplicates(false);
	CurrentBall = Bocha;
	HoldBall(Bocha);
}

void ABochaMinigameManager::HoldBall(ABochaBall* Ball)
{
	Ball->GetSphere()->SetSimulatePhysics(false);

	ACharacter* Holder = IsValid(TargetCharacter) ? TargetCharacter.Get() : UGameplayStatics::GetPlayerCharacter(this, 0);
	Ball->AttachToActor(Holder, FAttachmentTransformRules(EAttachmentRule::KeepRelative, true));
	Ball->SetActorRelativeLocation(HeldBallOffset);
}
