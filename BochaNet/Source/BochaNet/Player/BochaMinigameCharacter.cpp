#include "Player/BochaMinigameCharacter.h"

#include "Actors/BochaBall.h"
#include "Blueprint/UserWidget.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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

	// First person, aligned with the character's view axis: the camera sits at eye height just in front
	// of the face, looking slightly down, and turns with the aim (the actor's yaw; nobody possesses this
	// character, so there is no control rotation). Zero-length arm: no collision probe needed, only
	// rotation lag to smooth the aim. The head is hidden so the camera never sees its inside.
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	// 40 cm forward: the chest and shoulders lean forward in the throw and must stay behind the camera.
	SpringArm->SetRelativeLocation(FVector(40.0, 0.0, 70.0));
	SpringArm->SetRelativeRotation(FRotator(-10.0, 0.0, 0.0));
	SpringArm->TargetArmLength = 0.f;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 12.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// Mannequin placement on the capsule (feet on the ground, facing +X), as in the template.
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -89.0), FRotator(0.0, -90.0, 0.0));
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

void ABochaMinigameCharacter::SetTeam(int32 Team)
{
	if (Team == DressedTeam || Team == INDEX_NONE)
	{
		return;
	}
	DressedTeam = Team;

	if (USkeletalMesh* TeamMesh = Team == 1 ? BlueMesh : RedMesh)
	{
		GetMesh()->SetSkeletalMesh(TeamMesh);
		// First person: the camera is where the face is (hiding the neck hides the head too).
		GetMesh()->HideBoneByName(TEXT("neck_01"), EPhysBodyOp::PBO_None);
	}
	if (AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(AnimClass);
	}
	UE_LOG(LogTemp, Display, TEXT("Minigame player dressed for team %d: mesh %s, anim %s."), Team,
		*GetNameSafe(GetMesh()->GetSkeletalMeshAsset()), *GetNameSafe(GetMesh()->GetAnimInstance()));
}

void ABochaMinigameCharacter::DoThrow()
{
	if (bThrowing || !GameMode || !IsValid(GameMode->GetCurrentBall()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	const bool bPlaying = ThrowMontage && AnimInstance && PlayAnimMontage(ThrowMontage) > 0.f;
	UE_LOG(LogTemp, Display, TEXT("Throw: montage %s %s."), *GetNameSafe(ThrowMontage), bPlaying ? TEXT("playing") : TEXT("not playing"));
	if (bPlaying)
	{
		bThrowing = true;
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ABochaMinigameCharacter::OnThrowMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, ThrowMontage);
		return;
	}

	// No animation to wait for.
	bThrowing = true;
	ReleaseBall();
}

void ABochaMinigameCharacter::ReleaseBall()
{
	if (!bThrowing || !GameMode || !IsValid(GameMode->GetCurrentBall()))
	{
		return;
	}
	bThrowing = false;

	const FVector Velocity = GetActorForwardVector() * ThrowStrength;
	SendThrow(Velocity);
}

void ABochaMinigameCharacter::OnThrowMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ReleaseBall();
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
