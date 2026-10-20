#include "UI/BochaMinimapCapture.h"

#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ABochaMinimapCapture::ABochaMinimapCapture()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABochaMinimapCapture::BeginPlay()
{
	Super::BeginPlay();

	SceneCapture = FindComponentByClass<USceneCaptureComponent2D>();
	if (!SceneCapture)
	{
		return;
	}

	if (ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		SceneCapture->HideActorComponents(PlayerCharacter, true);
	}

	GetWorldTimerManager().SetTimer(CaptureTimer, this, &ABochaMinimapCapture::CaptureMinimap, CaptureInterval, true);
}

void ABochaMinimapCapture::CaptureMinimap()
{
	SceneCapture->CaptureScene();
}
