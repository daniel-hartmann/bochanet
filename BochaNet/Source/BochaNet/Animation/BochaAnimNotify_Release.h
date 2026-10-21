#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "BochaAnimNotify_Release.generated.h"

/** Placed on the throw montage at the frame the hand lets go: the ball leaves exactly then. */
UCLASS(meta = (DisplayName = "Bocha: Soltar Bola"))
class BOCHANET_API UBochaAnimNotify_Release : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
