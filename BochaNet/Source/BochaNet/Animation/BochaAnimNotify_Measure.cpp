#include "Animation/BochaAnimNotify_Measure.h"

#include "Components/SkeletalMeshComponent.h"
#include "Game/BochaReferee.h"

FString UBochaAnimNotify_Measure::GetNotifyName_Implementation() const
{
	return TEXT("Medir");
}

void UBochaAnimNotify_Measure::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (ABochaReferee* Referee = MeshComp ? Cast<ABochaReferee>(MeshComp->GetOwner()) : nullptr)
	{
		Referee->OnMeasureNotify();
	}
}
