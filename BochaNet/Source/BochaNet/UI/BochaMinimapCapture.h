#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaMinimapCapture.generated.h"

class USceneCaptureComponent2D;

/**
 * Old camera minimap (render target shown by WBP_Minimap). Replaced by UBochaMinimapWidget, so it
 * no longer captures; the instance placed in CanchaMap can be deleted together with WBP_Minimap
 * and RT_Minimap.
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
