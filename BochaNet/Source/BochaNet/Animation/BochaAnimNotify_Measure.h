#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "BochaAnimNotify_Measure.generated.h"

/** Placed on the referee's measuring montage at the moment it looks at the balls: the point is announced then. */
UCLASS(meta = (DisplayName = "Bocha: Medir"))
class BOCHANET_API UBochaAnimNotify_Measure : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
