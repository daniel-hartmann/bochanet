#include "Net/BochaGameInstance.h"

#include "Engine/Engine.h"
#include "GameMapsSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogBochaGameInstance, Log, All);

void UBochaGameInstance::OnStart()
{
	Super::OnStart();

	FString Address;
	if (FParse::Value(FCommandLine::Get(), TEXT("-join="), Address) && !Address.IsEmpty())
	{
		UE_LOG(LogBochaGameInstance, Display, TEXT("Joining %s."), *Address);
		GEngine->SetClientTravel(GetWorld(), *Address, TRAVEL_Absolute);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("host")))
	{
		const FString Lobby = FPackageName::ObjectPathToPackageName(UGameMapsSettings::GetGameDefaultMap()) + TEXT("?listen");
		UE_LOG(LogBochaGameInstance, Display, TEXT("Hosting %s."), *Lobby);
		GEngine->SetClientTravel(GetWorld(), *Lobby, TRAVEL_Absolute);
	}
}
