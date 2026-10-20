#include "Player/BochaMinigameCharacter.h"

#include "Actors/BochaBolim.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/BochaMinigameManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BochaPlayerController.h"
#include "UI/BochaHUDWidget.h"

namespace
{
	// Above IMC_Default so the bocha keys win while the minigame runs.
	constexpr int32 MappingPriority = 10;
	constexpr float StrengthPerInput = 10.f;
	// Upward impulse as a fraction of the throw strength.
	constexpr float ArcFactor = 0.32f;
}

ABochaMinigameCharacter::ABochaMinigameCharacter()
{
	bReplicates = false;
	SetReplicatingMovement(false);

	// Nobody possesses this character, but it still has to walk.
	GetCharacterMovement()->bRunPhysicsWithNoController = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetCapsuleComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->SetRelativeLocation(FVector(330.0, 0.0, -10.0));
}

void ABochaMinigameCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		FModifyContextOptions Options;
		Options.bIgnoreAllPressedKeysUntilRelease = true;
		InputSubsystem->AddMappingContext(MappingContext, MappingPriority, Options);
	}

	if (UUserWidget* Minimap = CreateWidget<UUserWidget>(PlayerController, MinimapWidgetClass))
	{
		Minimap->AddToViewport();
	}
}

void ABochaMinigameCharacter::DoMove(float Scale)
{
	AddMovementInput(GetCapsuleComponent()->GetRightVector(), Scale);
}

void ABochaMinigameCharacter::DoStrength(float Delta)
{
	ThrowStrength = FMath::Clamp(ThrowStrength + Delta * StrengthPerInput,
		UBochaHUDWidget::MinThrowStrength, UBochaHUDWidget::MaxThrowStrength);

	if (UBochaHUDWidget* HUD = GameMode ? GameMode->GetHUD() : nullptr)
	{
		HUD->SetThrowStrength(ThrowStrength);
	}
}

void ABochaMinigameCharacter::DoThrow()
{
	if (!bCanThrow || !GameMode)
	{
		return;
	}
	SetCanThrow(false);

	// Component-wise product, as in the original Blueprint: only the forward vector's X is used.
	const FVector Impulse = GetActorForwardVector() * FVector(ThrowStrength, 0.0, 0.0)
		+ FVector(0.0, 0.0, ThrowStrength * ArcFactor);
	const bool bIsBolim = GameMode->GetCurrentBall()->IsA<ABochaBolim>();

	SendThrow(Impulse, bIsBolim);

	if (bIsBolim)
	{
		GameMode->NotifyBolimThrown();
	}
	else
	{
		GameMode->NotifyBallThrown();
	}
	SetCanThrow(true);
}

void ABochaMinigameCharacter::SendThrow(const FVector& Impulse, bool bIsBolim)
{
	ABochaPlayerController* PlayerController = Cast<ABochaPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!PlayerController)
	{
		return;
	}

	ABochaBall* Ball = GameMode->GetCurrentBall();
	PlayerController->ServerThrow(Ball->GetActorLocation(), Impulse, bIsBolim);
	Ball->Destroy();
}
