#include "UI/BochaMinimapCapture.h"

#include "Components/SceneCaptureComponent2D.h"
#include "TimerManager.h"

ABochaMinimapCapture::ABochaMinimapCapture()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABochaMinimapCapture::BeginPlay()
{
	Super::BeginPlay();

	// Replaced by UBochaMinimapWidget: rendering the court twice would only cost frame time.
	SceneCapture = FindComponentByClass<USceneCaptureComponent2D>();
	if (SceneCapture)
	{
		SceneCapture->bCaptureEveryFrame = false;
		SceneCapture->bCaptureOnMovement = false;
	}
}

void ABochaMinimapCapture::CaptureMinimap()
{
	SceneCapture->CaptureScene();
}
