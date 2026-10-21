#include "Player/BochaBallCamera.h"

#include "Actors/BochaBall.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ABochaBallCamera::ABochaBallCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	// Move after the ball's physics, so the arm sees where the ball is this frame.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	RootComponent = Pivot;

	// World-fixed angle: always looking down the court (+X), whatever the ball's spin.
	// The arm starts 50 cm above the ball: from the ball's center (5 cm off the floor) the 12 cm
	// collision probe would start inside the floor and pull the camera into it.
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Pivot);
	SpringArm->SetRelativeLocation(FVector(0.0, 0.0, 50.0));
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->SetWorldRotation(FRotator(-25.0, 0.0, 0.0));
	SpringArm->TargetArmLength = 300.f;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.f;
	SpringArm->CameraLagMaxDistance = 400.f;
	// Occlusion: the arm shortens against the boards; the balls ignore the Camera channel.
	SpringArm->bDoCollisionTest = true;
	SpringArm->ProbeSize = 12.f;
	SpringArm->ProbeChannel = ECC_Camera;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ABochaBallCamera::Follow(ABochaBall* Ball)
{
	Target = Ball;
	if (Ball)
	{
		SetActorLocation(Ball->GetActorLocation());
	}
}

void ABochaBallCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (const ABochaBall* Ball = Target.Get())
	{
		SetActorLocation(Ball->GetActorLocation());
	}
}
