#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaThrowCalibrator.generated.h"

class ABochaBall;
class UBoxComponent;

/**
 * Development tool: throws the jack and a bocha at a range of launch speeds on Lvl_Cancha and logs
 * where they stop, with and without a smooth collision slab over the floor. Started by the console
 * command Bocha.CalibrateThrows; writes Saved/BochaCalibration.txt and quits when done.
 */
UCLASS(NotPlaceable, Transient)
class BOCHANET_API ABochaThrowCalibrator : public AActor
{
	GENERATED_BODY()

public:
	ABochaThrowCalibrator();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void LaunchNext();

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> SmoothFloor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UClass>> BallClasses;

	UPROPERTY(Transient)
	TObjectPtr<ABochaBall> Ball;

	int32 Run = 0;
	FString Report;
	float BallTime = 0.f;
	float StillTime = 0.f;
};
