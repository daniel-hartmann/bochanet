#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaMinimapCapture.generated.h"

class USceneCaptureComponent2D;

/**
 * Renders the court into the minimap render target at a fixed interval, hiding the local player.
 * The scene capture component (and its render target) is set up on the instance in CanchaMap.
 */
UCLASS()
class BOCHANET_API ABochaMinimapCapture : public AActor
{
	GENERATED_BODY()

public:
	ABochaMinimapCapture();

protected:
	virtual void BeginPlay() override;

	/** Seconds between captures. */
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float CaptureInterval = 0.15f;

private:
	void CaptureMinimap();

	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	FTimerHandle CaptureTimer;
};
