#include "Player/BochaPlayerController.h"

#include "Actors/BochaBocha.h"
#include "Actors/BochaBolim.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Game/BochaMatch.h"
#include "Game/BochaMinigameManager.h"
#include "Kismet/GameplayStatics.h"
#include "UI/BochaHUDWidget.h"
#include "Lobby/BochaPortal.h"
#include "Player/BochaMinigameCharacter.h"

void ABochaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// The local player is not always ready on the first frame.
	GetWorldTimerManager().SetTimerForNextTick(this, &ABochaPlayerController::InitializeLocalInput);
}

void ABochaPlayerController::InitializeLocalInput()
{
	// Only add input mappings and touch UI to local players.
	if (!IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!InputSubsystem)
	{
		return;
	}
	InputSubsystem->AddMappingContext(DefaultMappingContext, 0);

	if (ShouldUseTouchControls())
	{
		if (UUserWidget* TouchControls = CreateWidget<UUserWidget>(this, TouchControlsWidgetClass))
		{
			TouchControls->AddToPlayerScreen(0);
		}
	}
	else
	{
		InputSubsystem->AddMappingContext(MouseLookMappingContext, 0);
	}
}

bool ABochaPlayerController::ShouldUseTouchControls() const
{
	const FString Platform = UGameplayStatics::GetPlatformName();
	return Platform.Equals(TEXT("iOS"), ESearchCase::IgnoreCase)
		|| Platform.Equals(TEXT("Android"), ESearchCase::IgnoreCase)
		|| bForceTouchControls;
}

void ABochaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(InputComponent);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Triggered, this, &ABochaPlayerController::Interact);
	EnhancedInput->BindAction(BochaMoveAction, ETriggerEvent::Triggered, this, &ABochaPlayerController::BochaMove);
	EnhancedInput->BindAction(BochaStrengthAction, ETriggerEvent::Triggered, this, &ABochaPlayerController::BochaStrength);
	EnhancedInput->BindAction(BochaThrowAction, ETriggerEvent::Started, this, &ABochaPlayerController::BochaThrow);
	EnhancedInput->BindAction(BochaAimMouseAction, ETriggerEvent::Triggered, this, &ABochaPlayerController::BochaAimMouse);
	EnhancedInput->BindAction(BochaPauseAction, ETriggerEvent::Started, this, &ABochaPlayerController::BochaPause);
}

void ABochaPlayerController::Interact()
{
	if (IsValid(NearbyPortal))
	{
		ServerRequestStart(NearbyPortal);
	}
}

ABochaMinigameCharacter* ABochaPlayerController::GetPlayableCharacter() const
{
	const bool bPaused = MinigameManager && MinigameManager->IsPaused();
	return IsValid(MinigameCharacter) && !bPaused ? MinigameCharacter.Get() : nullptr;
}

void ABochaPlayerController::BochaPause()
{
	if (MinigameManager)
	{
		MinigameManager->TogglePause();
	}
}

void ABochaPlayerController::BochaMove(const FInputActionValue& Value)
{
	ABochaMinigameCharacter* Player = GetPlayableCharacter();
	if (!Player)
	{
		return;
	}
	// X: A/D slide sideways. Y: Q/E aim.
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.X != 0.0)
	{
		Player->DoMove(Axis.X);
	}
	if (Axis.Y != 0.0)
	{
		Player->DoAim(Axis.Y);
	}
}

void ABochaPlayerController::BochaStrength(const FInputActionValue& Value)
{
	if (ABochaMinigameCharacter* Player = GetPlayableCharacter())
	{
		Player->DoStrength(Value.Get<FVector2D>().Y);
	}
}

void ABochaPlayerController::BochaAimMouse(const FInputActionValue& Value)
{
	if (ABochaMinigameCharacter* Player = GetPlayableCharacter())
	{
		Player->DoAimMouse(Value.Get<float>());
	}
}

void ABochaPlayerController::BochaThrow()
{
	if (ABochaMinigameCharacter* Player = GetPlayableCharacter())
	{
		Player->DoThrow();
	}
}

void ABochaPlayerController::ServerRequestStart_Implementation(ABochaPortal* Portal)
{
	if (Portal && !Portal->HasMinigameStarted())
	{
		Portal->MulticastStartMinigame();
	}
}

void ABochaPlayerController::ServerThrow_Implementation(FVector BallLocation, FVector Velocity)
{
	ABochaMatch* Match = ABochaMatch::Find(this);
	if (!Match || !Match->CanThrow(PlayerState) || !Match->IsBehindThrowLine(BallLocation))
	{
		ClientThrowRejected();
		return;
	}

	const FTransform SpawnTransform(BallLocation);
	ABochaBall* Ball = nullptr;
	FVector LaunchVelocity = Velocity;
	if (Match->IsThrowingBolim())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ball = GetWorld()->SpawnActor<ABochaBolim>(BolimClass, SpawnTransform, Params);
		// The strength picks where the jack stops; it always ends up valid.
		const float Strength = (Velocity.Size2D() - UBochaHUDWidget::MinThrowStrength)
			/ (UBochaHUDWidget::MaxThrowStrength - UBochaHUDWidget::MinThrowStrength);
		LaunchVelocity = Match->LaunchBolim(Ball, Strength);
	}
	else
	{
		ABochaBocha* Bocha = GetWorld()->SpawnActorDeferred<ABochaBocha>(BochaClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		// Team 1 plays blue, same rule as the held ball.
		Bocha->bBlue = Match->GetCurrentThrower() == 1;
		Bocha->FinishSpawning(SpawnTransform);
		Ball = Bocha;
	}

	// Rolled along the floor: a ball dropped from the hand loses most of its speed on landing.
	Ball->Launch(BallLocation, Match->GetFloorZ(), LaunchVelocity);
	Match->RegisterThrow(Ball);
}

void ABochaPlayerController::ServerClaimVictory_Implementation()
{
	if (ABochaMatch* Match = ABochaMatch::Find(this))
	{
		Match->HandleVictoryClaim(PlayerState);
	}
}

void ABochaPlayerController::ClientThrowRejected_Implementation()
{
	if (MinigameManager)
	{
		MinigameManager->Refresh();
	}
}
