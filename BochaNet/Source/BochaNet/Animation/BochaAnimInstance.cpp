#include "Animation/BochaAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UBochaAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(GetOwningActor());
	MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
}

void UBochaAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!MovementComponent)
	{
		return;
	}

	Velocity = MovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();
	// Speed only: the template also required acceleration, but an AI following a NavMesh path (and
	// every remote copy of a character) moves with zero acceleration and would slide in Idle.
	ShouldMove = GroundSpeed > 3.f;
	IsFalling = MovementComponent->IsFalling();

	// Facing the movement there is no sideways step to blend; otherwise strafe within +-45 degrees.
	const float RawDirection = UKismetAnimationLibrary::CalculateDirection(Velocity, Character->GetActorRotation());
	Direction = MovementComponent->bOrientRotationToMovement ? FMath::Clamp(RawDirection, -45.f, 45.f) : RawDirection;
}
