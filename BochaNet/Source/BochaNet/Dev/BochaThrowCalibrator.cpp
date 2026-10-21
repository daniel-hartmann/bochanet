#include "Dev/BochaThrowCalibrator.h"

#include "Actors/BochaBall.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogBochaCalibrate, Log, All);

namespace
{
	// CanchaMap coordinates (opened directly, no level instance offset).
	const FVector Release(-2470.0, -200.0, 110.0);
	constexpr double FloorZ = 50.0;
	constexpr int32 MinSpeed = 200;
	constexpr int32 MaxSpeed = 1000;
	constexpr int32 SpeedStep = 100;
	constexpr int32 SpeedsPerPass = (MaxSpeed - MinSpeed) / SpeedStep + 1;
	constexpr float MaxTime = 20.f;

#if !UE_BUILD_SHIPPING
	FAutoConsoleCommandWithWorld CalibrateCommand(
		TEXT("Bocha.CalibrateThrows"),
		TEXT("Throws the jack and a bocha at several speeds on CanchaMap, logs where they stop and quits."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			World->SpawnActor<ABochaThrowCalibrator>();
		}));
#endif
}

ABochaThrowCalibrator::ABochaThrowCalibrator()
{
	PrimaryActorTick.bCanEverTick = true;

	// Covers the court floor (X -2600..50, Y -400..0, top at Z 50) with one flat box, 1 mm above it.
	SmoothFloor = CreateDefaultSubobject<UBoxComponent>(TEXT("SmoothFloor"));
	SmoothFloor->SetBoxExtent(FVector(1325.0, 200.0, 10.0));
	SmoothFloor->SetRelativeLocation(FVector(-1275.0, -200.0, 40.1));
	SmoothFloor->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	SmoothFloor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = SmoothFloor;
}

void ABochaThrowCalibrator::BeginPlay()
{
	Super::BeginPlay();

	BallClasses.Add(LoadClass<ABochaBall>(nullptr, TEXT("/Game/Bocha/Actors/ABolim.ABolim_C")));
	BallClasses.Add(LoadClass<ABochaBall>(nullptr, TEXT("/Game/Bocha/Actors/ABocha.ABocha_C")));
	Report = TEXT("floor\tball\tspeed\tdistance\ttime\n");
	LaunchNext();
}

void ABochaThrowCalibrator::LaunchNext()
{
	// Runs: [floor pass][ball][speed]; the second pass uses the smooth slab.
	const int32 RunsPerPass = BallClasses.Num() * SpeedsPerPass;
	if (Run >= 2 * RunsPerPass)
	{
		const FString File = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BochaCalibration.txt"));
		FFileHelper::SaveStringToFile(Report, *File);
		UE_LOG(LogBochaCalibrate, Display, TEXT("Calibration written to %s\n%s"), *File, *Report);
		FPlatformMisc::RequestExit(false);
		SetActorTickEnabled(false);
		return;
	}

	const bool bSmooth = Run >= RunsPerPass;
	SmoothFloor->SetCollisionEnabled(bSmooth ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	const int32 InPass = Run % RunsPerPass;
	UClass* BallClass = BallClasses[InPass / SpeedsPerPass];
	const double Speed = MinSpeed + (InPass % SpeedsPerPass) * SpeedStep;

	Ball = GetWorld()->SpawnActor<ABochaBall>(BallClass, FTransform(Release));
	Ball->SetReplicates(false);
	Ball->Launch(Release, FloorZ, FVector(Speed, 0.0, 0.0));
	BallTime = 0.f;
	StillTime = 0.f;

	if (Run == 0)
	{
		const UPrimitiveComponent* Sphere = Ball->GetSphere();
		Report += FString::Printf(TEXT("# collision: profile %s, object type %s, vs referee %d, vs camera %d, affects nav %d\n"),
			*Sphere->GetCollisionProfileName().ToString(),
			*UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(Sphere->GetCollisionObjectType()).ToString(),
			int32(Sphere->GetCollisionResponseToChannel(ECC_GameTraceChannel2)), int32(Sphere->GetCollisionResponseToChannel(ECC_Camera)),
			Sphere->CanEverAffectNavigation());
	}
}

void ABochaThrowCalibrator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Ball)
	{
		return;
	}

	BallTime += DeltaSeconds;
	StillTime = Ball->GetVelocity().Size() < 2.0 ? StillTime + DeltaSeconds : 0.f;
	if (StillTime < 0.5f && BallTime < MaxTime)
	{
		return;
	}

	const int32 RunsPerPass = BallClasses.Num() * SpeedsPerPass;
	const int32 InPass = Run % RunsPerPass;
	Report += FString::Printf(TEXT("%s\t%s\t%d\t%.0f\t%.1f%s\n"),
		Run >= RunsPerPass ? TEXT("smooth") : TEXT("mesh"),
		InPass / SpeedsPerPass == 0 ? TEXT("jack") : TEXT("bocha"),
		MinSpeed + (InPass % SpeedsPerPass) * SpeedStep,
		Ball->GetActorLocation().X - Release.X, BallTime, BallTime >= MaxTime ? TEXT(" (still moving)") : TEXT(""));
	Ball->Destroy();
	Ball = nullptr;
	++Run;
	LaunchNext();
}
