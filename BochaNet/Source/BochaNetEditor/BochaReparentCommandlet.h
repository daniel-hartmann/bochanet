#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "BochaReparentCommandlet.generated.h"

class UBlueprint;
class FJsonObject;

/**
 * Moves Blueprints onto C++ parent classes following a JSON spec (see the JSON files in Docs/reparent).
 * Recompiles every Blueprint under the spec's compilePaths and saves only if all of them compile
 * and -Save is passed; otherwise it is a dry run.
 *
 * Usage: UnrealEditor BochaNet.uproject -run=BochaReparent -Spec=<file.json> [-Save]
 */
UCLASS()
class UBochaReparentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBochaReparentCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	bool ApplyBlueprintSpec(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const;
	bool ApplyMapSpec(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const;
	bool RetypeVariable(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const;
	int32 RetargetMembers(UBlueprint* Blueprint, UClass* FromClass, UClass* ToClass) const;
	static FString GetEventName(const UEdGraphNode* Node);
	TSet<UEdGraphNode*> FindNodesConnectedToEvents(UBlueprint* Blueprint, const TArray<FString>& EventNames) const;
	bool ConvertCustomEventToOverride(UBlueprint* Blueprint, FName EventName) const;
	int32 RetargetFunctionCalls(UBlueprint* Blueprint, UClass* FromClass, UClass* ToClass, const TSharedPtr<FJsonObject>& PinRenames) const;
	void RestoreNativeComponents(UBlueprint* Blueprint, const TArray<FString>& ComponentsToReset) const;
	bool ResetDefaults(UBlueprint* Blueprint, const TArray<FString>& PropertyNames) const;
	bool SetDefaults(UBlueprint* Blueprint, const TSharedPtr<FJsonObject>& Values) const;
	bool Compile(UBlueprint* Blueprint) const;
};
