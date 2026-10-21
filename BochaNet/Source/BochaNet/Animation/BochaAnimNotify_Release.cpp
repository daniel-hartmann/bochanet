#include "Animation/BochaAnimNotify_Release.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/BochaMinigameCharacter.h"

FString UBochaAnimNotify_Release::GetNotifyName_Implementation() const
{
	return TEXT("Soltar Bola");
}

void UBochaAnimNotify_Release::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (ABochaMinigameCharacter* Player = MeshComp ? Cast<ABochaMinigameCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Player->ReleaseBall();
	}
}
