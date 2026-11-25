#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
#if !UE_BUILD_SHIPPING
	// For -ExecCmds, which runs everything on the first frame: "Bocha.DelayedExec 5 HighResShot 1280x720".
	FAutoConsoleCommandWithWorldAndArgs DelayedExecCommand(
		TEXT("Bocha.DelayedExec"),
		TEXT("Bocha.DelayedExec <seconds> <command>: runs the console command after a delay."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() < 2 || !World)
			{
				return;
			}
			const float Delay = FCString::Atof(*Args[0]);
			const FString Command = FString::Join(TArrayView<const FString>(Args).RightChop(1), TEXT(" "));
			FTimerHandle Timer;
			World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(World, [World, Command]()
			{
				// Through the player controller, so viewport commands (HighResShot) work too.
				if (APlayerController* PlayerController = World->GetFirstPlayerController())
				{
					PlayerController->ConsoleCommand(Command);
				}
			}), Delay, false);
		}));
#endif
}
