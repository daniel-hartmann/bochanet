#include "Game/BochaMinigameManager.h"

#include "Actors/BochaBocha.h"
#include "Actors/BochaBolim.h"
#include "Components/StaticMeshComponent.h"
#include "Game/BochaMatch.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BochaBallCamera.h"
#include "Player/BochaPlayerController.h"
#include "Player/BochaMinigameCharacter.h"
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

	HUD = CreateWidget<UBochaHUDWidget>(GetWorld(), HUDWidgetClass);
	HUD->AddToViewport();
	if (const ABochaMinigameCharacter* Player = Cast<ABochaMinigameCharacter>(TargetCharacter))
	{
		HUD->SetThrowStrength(Player->GetThrowStrength());
	}

	SetMatch(ABochaMatch::Find(this));
}

void ABochaMinigameManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUD)
	{
		HUD->RemoveFromParent();
	}
	if (IsValid(CurrentBall))
	{
		CurrentBall->Destroy();
	}
	if (BallCamera)
	{
		BallCamera->Destroy();
	}
	if (Match)
	{
		Match->OnStateChanged.RemoveAll(this);
		Match->OnAnnounce.RemoveAll(this);
		Match->OnGameOver.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ABochaMinigameManager::SetMatch(ABochaMatch* NewMatch)
{
	if (!NewMatch || Match == NewMatch)
	{
		return;
	}
	Match = NewMatch;
	Match->OnStateChanged.AddUObject(this, &ABochaMinigameManager::Refresh);
	Match->OnAnnounce.AddWeakLambda(this, [this](const FText& Message)
	{
		if (HUD)
		{
			HUD->ShowMessage(Message);
		}
	});
	Match->OnGameOver.AddUObject(this, &ABochaMinigameManager::ShowGameOver);
	Refresh();
}

void ABochaMinigameManager::Refresh()
{
	if (!Match || !HUD)
	{
		return;
	}

	const APlayerController* LocalController = UGameplayStatics::GetPlayerController(this, 0);
	const APlayerState* LocalPlayer = LocalController ? LocalController->PlayerState : nullptr;
	RefreshHeldBall(Match->CanThrow(LocalPlayer));
	RefreshCamera();

	// The winner's machine claims the victory with a Server RPC; the server confirms it.
	if (!bClaimedVictory && Match->GetWinner() != INDEX_NONE && Match->IsOnTeam(LocalPlayer, Match->GetWinner()))
	{
		if (ABochaPlayerController* BochaController = Cast<ABochaPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			bClaimedVictory = true;
			BochaController->ServerClaimVictory();
		}
	}

	HUD->ShowPlayers(Match->GetCurrentThrower(), Match->GetTeamOf(LocalPlayer), Match->GetPointHolder(),
		Match->GetScore(0), Match->GetScore(1));
	for (int32 Team = 0; Team < ABochaMatch::NumTeams; ++Team)
	{
		HUD->ShowBochasLeft(Team, Match->GetBochasLeft(Team), Match->GetBochasPerPlayer());
	}
	HUD->ShowBolimThrower(Match->IsThrowingBolim() ? Match->GetCurrentThrower() : INDEX_NONE);
}

void ABochaMinigameManager::RefreshCamera()
{
	APlayerController* LocalController = UGameplayStatics::GetPlayerController(this, 0);
	if (!LocalController || !TargetCharacter)
	{
		return;
	}

	ABochaBall* Ball = Match->GetPhase() == EBochaPhase::BallsMoving ? Match->GetThrownBall() : nullptr;
	if (Ball == FollowedBall)
	{
		return;
	}
	FollowedBall = Ball;

	if (Ball)
	{
		if (!BallCamera)
		{
			BallCamera = GetWorld()->SpawnActor<ABochaBallCamera>();
		}
		BallCamera->Follow(Ball);
		LocalController->SetViewTargetWithBlend(BallCamera, CameraBlendTime);
	}
	else
	{
		LocalController->SetViewTargetWithBlend(TargetCharacter, CameraBlendTime);
	}
}

void ABochaMinigameManager::ShowGameOver(int32 WinnerTeam, int32 RedScore, int32 BlueScore)
{
	if (HUD)
	{
		HUD->ShowMessage(FText::Format(NSLOCTEXT("BochaMinigame", "GameOver", "Fim de jogo! {0} venceu por {1} x {2}."),
			ABochaMatch::GetTeamName(WinnerTeam), FMath::Max(RedScore, BlueScore), FMath::Min(RedScore, BlueScore)), 0.f);
	}
}

void ABochaMinigameManager::RefreshHeldBall(bool bMyTurn)
{
	const bool bWantBolim = Match->IsThrowingBolim();
	const bool bWantBlue = Match->GetCurrentThrower() == 1;

	if (IsValid(CurrentBall))
	{
		const ABochaBocha* HeldBocha = Cast<ABochaBocha>(CurrentBall);
		const bool bStillRight = bMyTurn
			&& (bWantBolim ? CurrentBall->IsA<ABochaBolim>() : HeldBocha && HeldBocha->bBlue == bWantBlue);
		if (bStillRight)
		{
			return;
		}
		CurrentBall->Destroy();
	}
	CurrentBall = nullptr;

	if (!bMyTurn)
	{
		return;
	}

	// Held balls exist only on this machine; the thrown ball is spawned by the server.
	if (bWantBolim)
	{
		CurrentBall = GetWorld()->SpawnActor<ABochaBolim>(BolimClass, FTransform::Identity);
	}
	else
	{
		ABochaBocha* Bocha = GetWorld()->SpawnActorDeferred<ABochaBocha>(BochaClass, FTransform::Identity);
		Bocha->bBlue = bWantBlue;
		Bocha->FinishSpawning(FTransform::Identity);
		CurrentBall = Bocha;
	}
	CurrentBall->SetReplicates(false);
	HoldBall(CurrentBall);
}

void ABochaMinigameManager::HoldBall(ABochaBall* Ball)
{
	Ball->GetSphere()->SetSimulatePhysics(false);

	ACharacter* Holder = IsValid(TargetCharacter) ? TargetCharacter.Get() : UGameplayStatics::GetPlayerCharacter(this, 0);
	Ball->AttachToActor(Holder, FAttachmentTransformRules(EAttachmentRule::KeepRelative, true));
	Ball->SetActorRelativeLocation(HeldBallOffset);
}
