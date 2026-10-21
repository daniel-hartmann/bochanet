#include "Player/BochaPlayerController.h"

#include "Actors/BochaBocha.h"
#include "Actors/BochaBolim.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
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
}

void ABochaPlayerController::Interact()
{
	if (IsValid(NearbyPortal))
	{
		ServerRequestStart(NearbyPortal);
	}
}

void ABochaPlayerController::BochaMove(const FInputActionValue& Value)
{
	if (IsValid(MinigameCharacter))
	{
		MinigameCharacter->DoMove(Value.Get<FVector2D>().X);
	}
}

void ABochaPlayerController::BochaStrength(const FInputActionValue& Value)
{
	if (IsValid(MinigameCharacter))
	{
		MinigameCharacter->DoStrength(Value.Get<FVector2D>().Y);
	}
}

void ABochaPlayerController::BochaThrow()
{
	if (IsValid(MinigameCharacter))
	{
		MinigameCharacter->DoThrow();
	}
}

void ABochaPlayerController::ServerRequestStart_Implementation(ABochaPortal* Portal)
{
	if (Portal && !Portal->HasMinigameStarted())
	{
		Portal->MulticastStartMinigame();
	}
}

void ABochaPlayerController::ServerThrow_Implementation(FVector BallLocation, FVector Impulse, bool bIsBolim)
{
	const FTransform SpawnTransform(BallLocation);
	ABochaBall* Ball = nullptr;
	if (bIsBolim)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ball = GetWorld()->SpawnActor<ABochaBolim>(BolimClass, SpawnTransform, Params);
	}
	else
	{
		ABochaBocha* Bocha = GetWorld()->SpawnActorDeferred<ABochaBocha>(BochaClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		// The host plays the red balls and the client the blue ones, same rule as the held ball.
		Bocha->bBlue = !IsLocalPlayerController();
		Bocha->FinishSpawning(SpawnTransform);
		Ball = Bocha;
	}

	Ball->GetSphere()->SetSimulatePhysics(true);
	Ball->GetSphere()->AddImpulse(Impulse);
}
