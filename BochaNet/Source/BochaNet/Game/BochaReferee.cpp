#include "Game/BochaReferee.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BochaCollision.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogBochaReferee, Log, All);

ABochaReferee::ABochaReferee()
{
	bReplicates = true;
	SetReplicatingMovement(true);

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Walks through the balls without moving them, and does not cut holes in its own NavMesh.
	GetCapsuleComponent()->SetCollisionProfileName(BochaCollision::RefereeProfile);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -89.0), FRotator(0.0, -90.0, 0.0));

	// A jog (it crosses the whole court to reach the jack) that turns towards where it goes.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0, 360.0, 0.0);
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
}

void ABochaReferee::BeginPlay()
{
	Super::BeginPlay();

	// Every machine dresses its copy (mesh and AnimBlueprint are not replicated).
	if (RefereeMesh)
	{
		GetMesh()->SetSkeletalMesh(RefereeMesh);
	}
	if (RefereeMaterial)
	{
		for (int32 Slot = 0; Slot < GetMesh()->GetNumMaterials(); ++Slot)
		{
			GetMesh()->SetMaterial(Slot, RefereeMaterial);
		}
	}
	if (AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(AnimClass);
	}
}

void ABochaReferee::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AAIController* AIController = Cast<AAIController>(NewController))
	{
		AIController->ReceiveMoveCompleted.AddUniqueDynamic(this, &ABochaReferee::OnMoveCompleted);
	}
}

void ABochaReferee::WalkTo(const FVector& Location)
{
	AAIController* AIController = Cast<AAIController>(GetController());
	const EPathFollowingRequestResult::Type Request = AIController
		? AIController->MoveToLocation(Location, AcceptanceRadius, true, true, true, false)
		: EPathFollowingRequestResult::Failed;
	UE_LOG(LogBochaReferee, Display, TEXT("Walk to %s: %s."), *Location.ToCompactString(),
		Request == EPathFollowingRequestResult::RequestSuccessful ? TEXT("path found")
		: Request == EPathFollowingRequestResult::AlreadyAtGoal ? TEXT("already there")
		: TEXT("no path (NavMesh missing?)"));

	if (Request == EPathFollowingRequestResult::RequestSuccessful)
	{
		CurrentWalk = AIController->GetCurrentMoveRequestID();
	}
	else
	{
		CurrentWalk = FAIRequestID::InvalidRequest;
		OnArrived.ExecuteIfBound();
	}
}

void ABochaReferee::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (RequestID != CurrentWalk)
	{
		return;
	}
	CurrentWalk = FAIRequestID::InvalidRequest;
	UE_LOG(LogBochaReferee, Display, TEXT("Walk finished: %s."), *UEnum::GetValueAsString(Result));
	OnArrived.ExecuteIfBound();
}

void ABochaReferee::Measure()
{
	if (MeasureMontage && GetMesh()->GetAnimInstance())
	{
		MulticastPlayMeasure();
	}
	else
	{
		OnMeasured.ExecuteIfBound();
	}
}

void ABochaReferee::MulticastPlayMeasure_Implementation()
{
	PlayAnimMontage(MeasureMontage);
}

void ABochaReferee::OnMeasureNotify()
{
	if (HasAuthority())
	{
		OnMeasured.ExecuteIfBound();
	}
}
