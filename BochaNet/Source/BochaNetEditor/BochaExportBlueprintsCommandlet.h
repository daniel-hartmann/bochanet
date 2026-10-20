#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "BochaExportBlueprintsCommandlet.generated.h"

/**
 * Dumps Blueprint graphs, variables and components to text so they can be ported to C++.
 *
 * Usage: UnrealEditor BochaNet.uproject -run=BochaExportBlueprints [-Out=<dir>]
 */
UCLASS()
class UBochaExportBlueprintsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBochaExportBlueprintsCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	void ExportBlueprint(class UBlueprint* Blueprint, const FString& OutDir) const;
	FString DescribeGraph(class UEdGraph* Graph) const;
};
