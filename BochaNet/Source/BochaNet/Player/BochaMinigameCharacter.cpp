#include "Player/BochaMinigameCharacter.h"

#include "Actors/BochaBall.h"
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
	BaseRotation = GetActorRotation();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		FModifyContextOptions Options;
		Options.bIgnoreAllPressedKeysUntilRelease = true;
		InputSubsystem->AddMappingContext(MappingContext, MappingPriority, Options);
	}

	Minimap = CreateWidget<UUserWidget>(PlayerController, MinimapWidgetClass);
	if (Minimap)
	{
		Minimap->AddToViewport();
	}
}

void ABochaMinigameCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Minimap)
	{
		Minimap->RemoveFromParent();
	}
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = PlayerController
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) : nullptr)
	{
		InputSubsystem->RemoveMappingContext(MappingContext);
	}
	Super::EndPlay(EndPlayReason);
}

void ABochaMinigameCharacter::DoMove(float Scale)
{
	// Across the court, whatever the aim.
	AddMovementInput(BaseRotation.RotateVector(FVector::RightVector), Scale);
}

void ABochaMinigameCharacter::DoAim(float Direction)
{
	TurnAim(Direction * AimSpeed * GetWorld()->GetDeltaSeconds());
}

void ABochaMinigameCharacter::DoAimMouse(float MouseDelta)
{
	TurnAim(MouseDelta * MouseAimSensitivity);
}

void ABochaMinigameCharacter::TurnAim(float DeltaYaw)
{
	AimYaw = FMath::Clamp(AimYaw + DeltaYaw, -MaxAimAngle, MaxAimAngle);
	SetActorRotation(BaseRotation + FRotator(0.0, AimYaw, 0.0));
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
	if (!GameMode || !IsValid(GameMode->GetCurrentBall()))
	{
		return;
	}

	const FVector Velocity = GetActorForwardVector() * ThrowStrength;
	SendThrow(Velocity);
}

void ABochaMinigameCharacter::SendThrow(const FVector& Velocity)
{
	ABochaPlayerController* PlayerController = Cast<ABochaPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!PlayerController)
	{
		return;
	}

	ABochaBall* Ball = GameMode->GetCurrentBall();
	PlayerController->ServerThrow(Ball->GetActorLocation(), Velocity);
	Ball->Destroy();
}
