#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BochaGameInstance.generated.h"

/**
 * Lets a packaged build host or join a match from the command line:
 *   -host          opens the lobby as a listen server
 *   -join=<IP>     connects to a host
 * Shipping builds ignore a map or address given on the command line, so the game travels itself.
 */
UCLASS()
class BOCHANET_API UBochaGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void OnStart() override;
};
