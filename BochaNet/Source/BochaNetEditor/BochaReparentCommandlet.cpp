#include "BochaReparentCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "FileHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "WidgetBlueprint.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Event.h"
#include "K2Node_Variable.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogBochaReparent, Log, All);

namespace
{
	UBlueprint* LoadBlueprint(const FString& Path)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
		if (!Blueprint)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("Blueprint not found: %s"), *Path);
		}
		return Blueprint;
	}

	UClass* LoadClassByPath(const FString& Path)
	{
		UClass* Class = LoadObject<UClass>(nullptr, *Path);
		if (!Class)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("Class not found: %s"), *Path);
		}
		return Class;
	}

	TArray<FString> GetStringArray(const TSharedPtr<FJsonObject>& Object, const FString& Field)
	{
		TArray<FString> Result;
		Object->TryGetStringArrayField(Field, Result);
		return Result;
	}
}

UBochaReparentCommandlet::UBochaReparentCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UBochaReparentCommandlet::Main(const FString& Params)
{
	FString SpecPath;
	if (!FParse::Value(*Params, TEXT("Spec="), SpecPath))
	{
		UE_LOG(LogBochaReparent, Error, TEXT("Missing -Spec=<file.json>"));
		return 1;
	}
	const bool bSave = FParse::Param(*Params, TEXT("Save"));

	FString SpecText;
	TSharedPtr<FJsonObject> Spec;
	if (!FFileHelper::LoadFileToString(SpecText, *SpecPath)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(SpecText), Spec) || !Spec.IsValid())
	{
		UE_LOG(LogBochaReparent, Error, TEXT("Could not read spec %s"), *SpecPath);
		return 1;
	}

	FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().SearchAllAssets(true);

	// Load every Blueprint that may depend on the ones being changed.
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	FARFilter Filter;
	for (const FString& Path : GetStringArray(Spec, TEXT("compilePaths")))
	{
		Filter.PackagePaths.Add(FName(*Path));
	}
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	TArray<UBlueprint*> AllBlueprints;
	for (const FAssetData& Asset : Assets)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
		{
			AllBlueprints.Add(Blueprint);
		}
	}

	bool bOk = true;
	TSet<UPackage*> ChangedPackages;

	const TArray<TSharedPtr<FJsonValue>>* NewBlueprints = nullptr;
	if (Spec->TryGetArrayField(TEXT("createBlueprints"), NewBlueprints))
	{
		for (const TSharedPtr<FJsonValue>& Value : *NewBlueprints)
		{
			bOk &= CreateBlueprintAsset(Value->AsObject(), ChangedPackages);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* NewActions = nullptr;
	if (Spec->TryGetArrayField(TEXT("createInputActions"), NewActions))
	{
		for (const TSharedPtr<FJsonValue>& Value : *NewActions)
		{
			bOk &= CreateInputAction(Value->AsObject(), ChangedPackages);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* KeyUnmappings = nullptr;
	if (Spec->TryGetArrayField(TEXT("unmapKeys"), KeyUnmappings))
	{
		for (const TSharedPtr<FJsonValue>& Value : *KeyUnmappings)
		{
			bOk &= UnmapKey(Value->AsObject(), ChangedPackages);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* KeyMappings = nullptr;
	if (Spec->TryGetArrayField(TEXT("mapKeys"), KeyMappings))
	{
		for (const TSharedPtr<FJsonValue>& Value : *KeyMappings)
		{
			bOk &= MapKey(Value->AsObject(), ChangedPackages);
		}
	}

	// Calls and member accesses are retargeted first, while the old pins still carry their links.
	const TArray<TSharedPtr<FJsonValue>>* FunctionRetargets = nullptr;
	if (Spec->TryGetArrayField(TEXT("retargetFunctions"), FunctionRetargets))
	{
		for (const TSharedPtr<FJsonValue>& Value : *FunctionRetargets)
		{
			const TSharedPtr<FJsonObject> Retarget = Value->AsObject();
			UClass* From = LoadClassByPath(Retarget->GetStringField(TEXT("from")));
			UClass* To = LoadClassByPath(Retarget->GetStringField(TEXT("to")));
			if (!From || !To)
			{
				bOk = false;
				continue;
			}
			const TSharedPtr<FJsonObject>* PinRenames = nullptr;
			Retarget->TryGetObjectField(TEXT("pinRenames"), PinRenames);
			for (UBlueprint* Blueprint : AllBlueprints)
			{
				if (RetargetFunctionCalls(Blueprint, From, To, PinRenames ? *PinRenames : nullptr) > 0)
				{
					ChangedPackages.Add(Blueprint->GetPackage());
				}
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Retargets = nullptr;
	if (Spec->TryGetArrayField(TEXT("retargetMembers"), Retargets))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Retargets)
		{
			UClass* From = LoadClassByPath(Value->AsObject()->GetStringField(TEXT("from")));
			UClass* To = LoadClassByPath(Value->AsObject()->GetStringField(TEXT("to")));
			if (!From || !To)
			{
				bOk = false;
				continue;
			}
			for (UBlueprint* Blueprint : AllBlueprints)
			{
				if (RetargetMembers(Blueprint, From, To) > 0)
				{
					ChangedPackages.Add(Blueprint->GetPackage());
				}
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Blueprints = nullptr;
	if (Spec->TryGetArrayField(TEXT("blueprints"), Blueprints))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Blueprints)
		{
			bOk &= ApplyBlueprintSpec(Value->AsObject(), ChangedPackages);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Retypes = nullptr;
	if (Spec->TryGetArrayField(TEXT("retypeVariables"), Retypes))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Retypes)
		{
			bOk &= RetypeVariable(Value->AsObject(), ChangedPackages);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Maps = nullptr;
	if (Spec->TryGetArrayField(TEXT("maps"), Maps))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Maps)
		{
			bOk &= ApplyMapSpec(Value->AsObject(), ChangedPackages);
		}
	}

	for (UBlueprint* Blueprint : AllBlueprints)
	{
		FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
		bOk &= Compile(Blueprint);
	}

	if (!bOk)
	{
		UE_LOG(LogBochaReparent, Error, TEXT("Errors found; nothing was saved."));
		return 1;
	}

	if (!bSave)
	{
		UE_LOG(LogBochaReparent, Display, TEXT("Dry run OK; would save %d packages (pass -Save to write them)."), ChangedPackages.Num());
		for (UPackage* Package : ChangedPackages)
		{
			UE_LOG(LogBochaReparent, Display, TEXT("  %s"), *Package->GetName());
		}
		return 0;
	}

	TArray<UPackage*> ToSave = ChangedPackages.Array();
	for (UPackage* Package : ToSave)
	{
		Package->MarkPackageDirty();
	}
	if (!UEditorLoadingAndSavingUtils::SavePackages(ToSave, false))
	{
		UE_LOG(LogBochaReparent, Error, TEXT("Saving failed."));
		return 1;
	}
	UE_LOG(LogBochaReparent, Display, TEXT("Saved %d packages."), ToSave.Num());
	return 0;
}

bool UBochaReparentCommandlet::ApplyBlueprintSpec(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const
{
	UBlueprint* Blueprint = LoadBlueprint(Spec->GetStringField(TEXT("path")));
	if (!Blueprint)
	{
		return false;
	}

	// Components and variables that the C++ parent now provides must leave the Blueprint first.
	for (const FString& Name : GetStringArray(Spec, TEXT("removeComponents")))
	{
		USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
		USCS_Node* Node = SCS ? SCS->FindSCSNode(FName(*Name)) : nullptr;
		if (!Node)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("%s: component %s not found"), *Blueprint->GetName(), *Name);
			return false;
		}
		SCS->RemoveNodeAndPromoteChildren(Node);
	}

	for (const FString& Name : GetStringArray(Spec, TEXT("removeFunctions")))
	{
		TObjectPtr<UEdGraph>* Graph = Blueprint->FunctionGraphs.FindByPredicate(
			[&Name](const UEdGraph* Candidate) { return Candidate->GetName() == Name; });
		if (!Graph)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("%s: function %s not found"), *Blueprint->GetName(), *Name);
			return false;
		}
		FBlueprintEditorUtils::RemoveGraph(Blueprint, *Graph);
	}

	// FBlueprintEditorUtils::RemoveMemberVariable would also delete every node that uses the variable;
	// those nodes must stay and resolve to the C++ property of the same name.
	for (const FString& Name : GetStringArray(Spec, TEXT("removeVariables")))
	{
		const int32 Index = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(*Name));
		if (Index == INDEX_NONE)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("%s: variable %s not found"), *Blueprint->GetName(), *Name);
			return false;
		}
		Blueprint->NewVariables.RemoveAt(Index);
	}

	// C++ widget bindings (BindWidget) need the designer widget to be a variable.
	for (const FString& Name : GetStringArray(Spec, TEXT("markWidgetsAsVariables")))
	{
		UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
		UWidget* Widget = WidgetBlueprint && WidgetBlueprint->WidgetTree ? WidgetBlueprint->WidgetTree->FindWidget(FName(*Name)) : nullptr;
		if (!Widget)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("%s: widget %s not found"), *Blueprint->GetName(), *Name);
			return false;
		}
		Widget->bIsVariable = true;
		UE_LOG(LogBochaReparent, Display, TEXT("%s: %s is now a variable"), *Blueprint->GetName(), *Name);
	}

	const TArray<FString> EventsToRemove = GetStringArray(Spec, TEXT("removeEvents"));
	if (EventsToRemove.Num() > 0)
	{
		const TSet<UEdGraphNode*> ToRemove = FindNodesConnectedToEvents(Blueprint, EventsToRemove);
		for (UEdGraphNode* Node : ToRemove)
		{
			FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
		}
		UE_LOG(LogBochaReparent, Display, TEXT("%s: removed %d nodes of events %s"), *Blueprint->GetName(), ToRemove.Num(), *FString::Join(EventsToRemove, TEXT(", ")));
	}

	bool bClearEventGraph = false;
	if (Spec->TryGetBoolField(TEXT("clearEventGraph"), bClearEventGraph) && bClearEventGraph)
	{
		const TSet<UEdGraphNode*> Kept = FindNodesConnectedToEvents(Blueprint, GetStringArray(Spec, TEXT("keepEvents")));
		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			for (UEdGraphNode* Node : TArray<UEdGraphNode*>(Graph->Nodes))
			{
				if (!Kept.Contains(Node))
				{
					FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
				}
			}
		}
	}

	FString ParentPath;
	if (Spec->TryGetStringField(TEXT("parent"), ParentPath))
	{
		UClass* NewParent = LoadClassByPath(ParentPath);
		if (!NewParent)
		{
			return false;
		}
		UE_LOG(LogBochaReparent, Display, TEXT("%s: %s -> %s"), *Blueprint->GetName(),
			*GetNameSafe(Blueprint->ParentClass), *NewParent->GetName());
		Blueprint->ParentClass = NewParent;
	}

	for (const FString& Name : GetStringArray(Spec, TEXT("convertEventsToOverrides")))
	{
		if (!ConvertCustomEventToOverride(Blueprint, FName(*Name)))
		{
			return false;
		}
	}

	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	// Dependents may not compile until the rest of the spec is applied; only the final pass decides.
	Compile(Blueprint);

	RestoreNativeComponents(Blueprint, GetStringArray(Spec, TEXT("resetComponents")));
	if (!ResetDefaults(Blueprint, GetStringArray(Spec, TEXT("resetDefaults"))))
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* Defaults = nullptr;
	if (Spec->TryGetObjectField(TEXT("set"), Defaults) && !SetDefaults(Blueprint, *Defaults))
	{
		return false;
	}

	ChangedPackages.Add(Blueprint->GetPackage());
	return true;
}

bool UBochaReparentCommandlet::CreateBlueprintAsset(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const
{
	const FString PackageName = Spec->GetStringField(TEXT("path"));
	const FString AssetName = FPackageName::GetShortName(PackageName);
	if (LoadObject<UBlueprint>(nullptr, *(PackageName + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		UE_LOG(LogBochaReparent, Display, TEXT("%s already exists"), *PackageName);
		return true;
	}

	UClass* Parent = LoadClassByPath(Spec->GetStringField(TEXT("parent")));
	if (!Parent)
	{
		return false;
	}
	UPackage* Package = CreatePackage(*PackageName);
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(Parent, Package, FName(*AssetName), BPTYPE_Normal,
		UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	FAssetRegistryModule::AssetCreated(Blueprint);
	Package->MarkPackageDirty();
	ChangedPackages.Add(Package);
	UE_LOG(LogBochaReparent, Display, TEXT("Created %s (child of %s)"), *PackageName, *Parent->GetName());
	return true;
}

bool UBochaReparentCommandlet::CreateInputAction(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const
{
	const FString PackageName = Spec->GetStringField(TEXT("path"));
	if (LoadObject<UInputAction>(nullptr, *(PackageName + TEXT(".") + FPackageName::GetShortName(PackageName)), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		UE_LOG(LogBochaReparent, Display, TEXT("%s already exists"), *PackageName);
		return true;
	}

	const UEnum* ValueTypes = StaticEnum<EInputActionValueType>();
	const int64 ValueType = ValueTypes->GetValueByNameString(Spec->GetStringField(TEXT("valueType")));
	if (ValueType == INDEX_NONE)
	{
		UE_LOG(LogBochaReparent, Error, TEXT("%s: unknown valueType"), *PackageName);
		return false;
	}

	UPackage* Package = CreatePackage(*PackageName);
	UInputAction* Action = NewObject<UInputAction>(Package, *FPackageName::GetShortName(PackageName), RF_Public | RF_Standalone);
	Action->ValueType = static_cast<EInputActionValueType>(ValueType);
	FAssetRegistryModule::AssetCreated(Action);
	Package->MarkPackageDirty();
	ChangedPackages.Add(Package);
	UE_LOG(LogBochaReparent, Display, TEXT("Created %s (%s)"), *PackageName, *Spec->GetStringField(TEXT("valueType")));
	return true;
}

bool UBochaReparentCommandlet::UnmapKey(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const
{
	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *Spec->GetStringField(TEXT("context")));
	UInputAction* Action = LoadObject<UInputAction>(nullptr, *Spec->GetStringField(TEXT("action")));
	const FKey Key(*Spec->GetStringField(TEXT("key")));
	if (!Context || !Action || !Key.IsValid())
	{
		UE_LOG(LogBochaReparent, Error, TEXT("unmapKeys: context, action or key not found in %s"), *Spec->GetStringField(TEXT("context")));
		return false;
	}
	Context->UnmapKey(Action, Key);
	UE_LOG(LogBochaReparent, Display, TEXT("%s: %s no longer mapped to %s"), *Context->GetName(), *Key.ToString(), *Action->GetName());
	Context->MarkPackageDirty();
	ChangedPackages.Add(Context->GetPackage());
	return true;
}

bool UBochaReparentCommandlet::MapKey(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const
{
	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *Spec->GetStringField(TEXT("context")));
	UInputAction* Action = LoadObject<UInputAction>(nullptr, *Spec->GetStringField(TEXT("action")));
	const FKey Key(*Spec->GetStringField(TEXT("key")));
	if (!Context || !Action || !Key.IsValid())
	{
		UE_LOG(LogBochaReparent, Error, TEXT("mapKeys: context, action or key not found in %s"), *Spec->GetStringField(TEXT("context")));
		return false;
	}

	const bool bAlreadyMapped = Context->GetMappings().ContainsByPredicate(
		[Action, &Key](const FEnhancedActionKeyMapping& Mapping) { return Mapping.Action == Action && Mapping.Key == Key; });
	if (bAlreadyMapped)
	{
		UE_LOG(LogBochaReparent, Display, TEXT("%s: %s already mapped to %s"), *Context->GetName(), *Key.ToString(), *Action->GetName());
		return true;
	}

	FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
	for (const FString& Modifier : GetStringArray(Spec, TEXT("modifiers")))
	{
		if (Modifier == TEXT("SwizzleAxis"))
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		}
		else if (Modifier == TEXT("Negate"))
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
		else
		{
			UE_LOG(LogBochaReparent, Error, TEXT("mapKeys: unknown modifier %s"), *Modifier);
			return false;
		}
	}

	UE_LOG(LogBochaReparent, Display, TEXT("%s: %s -> %s [%s]"), *Context->GetName(), *Key.ToString(), *Action->GetName(),
		*FString::Join(GetStringArray(Spec, TEXT("modifiers")), TEXT(", ")));
	Context->MarkPackageDirty();
	ChangedPackages.Add(Context->GetPackage());
	return true;
}

bool UBochaReparentCommandlet::ApplyMapSpec(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const
{
	const FString Path = Spec->GetStringField(TEXT("path"));
	UWorld* World = LoadObject<UWorld>(nullptr, *Path);
	AWorldSettings* WorldSettings = World ? World->GetWorldSettings() : nullptr;
	if (!WorldSettings)
	{
		UE_LOG(LogBochaReparent, Error, TEXT("Map or world settings not found: %s"), *Path);
		return false;
	}

	bool bClearGameModeOverride = false;
	if (Spec->TryGetBoolField(TEXT("clearGameModeOverride"), bClearGameModeOverride) && bClearGameModeOverride)
	{
		UE_LOG(LogBochaReparent, Display, TEXT("%s: GameMode override %s -> None"), *World->GetName(), *GetNameSafe(WorldSettings->DefaultGameMode));
		WorldSettings->DefaultGameMode = nullptr;
		WorldSettings->MarkPackageDirty();
		ChangedPackages.Add(World->GetPackage());
	}
	return true;
}

bool UBochaReparentCommandlet::RetypeVariable(const TSharedPtr<FJsonObject>& Spec, TSet<UPackage*>& ChangedPackages) const
{
	UBlueprint* Blueprint = LoadBlueprint(Spec->GetStringField(TEXT("blueprint")));
	UClass* Class = LoadClassByPath(Spec->GetStringField(TEXT("class")));
	if (!Blueprint || !Class)
	{
		return false;
	}

	const FName VarName(*Spec->GetStringField(TEXT("variable")));
	const int32 Index = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogBochaReparent, Error, TEXT("%s: variable %s not found"), *Blueprint->GetName(), *VarName.ToString());
		return false;
	}

	// FBlueprintEditorUtils::ChangeMemberVariableType asks for confirmation in a modal dialog, which
	// crashes a headless editor, so the type is changed directly and the nodes are refreshed instead.
	FEdGraphPinType& PinType = Blueprint->NewVariables[Index].VarType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
	PinType.PinSubCategoryObject = Class;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);

	UE_LOG(LogBochaReparent, Display, TEXT("%s.%s retyped to %s"), *Blueprint->GetName(), *VarName.ToString(), *Class->GetName());
	ChangedPackages.Add(Blueprint->GetPackage());
	Compile(Blueprint);
	return true;
}

int32 UBochaReparentCommandlet::RetargetMembers(UBlueprint* Blueprint, UClass* FromClass, UClass* ToClass) const
{
	TArray<UK2Node_Variable*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes);

	int32 Count = 0;
	for (UK2Node_Variable* Node : Nodes)
	{
		const UClass* Parent = Node->VariableReference.GetMemberParentClass();
		if (Node->VariableReference.IsSelfContext() || !Parent || Parent->GetAuthoritativeClass() != FromClass)
		{
			continue;
		}
		FProperty* Property = FindFProperty<FProperty>(ToClass, Node->VariableReference.GetMemberName());
		if (!Property)
		{
			continue;
		}
		Node->VariableReference.SetFromField<FProperty>(Property, false);
		Node->ReconstructNode();
		++Count;
	}

	if (Count > 0)
	{
		UE_LOG(LogBochaReparent, Display, TEXT("%s: retargeted %d variable nodes %s -> %s"),
			*Blueprint->GetName(), Count, *FromClass->GetName(), *ToClass->GetName());
	}
	return Count;
}

FString UBochaReparentCommandlet::GetEventName(const UEdGraphNode* Node)
{
	if (const UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node))
	{
		return CustomEvent->CustomFunctionName.ToString();
	}
	if (const UK2Node_Event* Event = Cast<UK2Node_Event>(Node))
	{
		return Event->EventReference.GetMemberName().ToString();
	}
	// Enhanced Input action events are named after their action (found by reflection to avoid
	// depending on the InputBlueprintNodes module).
	if (const FObjectProperty* ActionProperty = FindFProperty<FObjectProperty>(Node->GetClass(), TEXT("InputAction")))
	{
		if (const UObject* Action = ActionProperty->GetObjectPropertyValue_InContainer(Node))
		{
			return Action->GetName();
		}
	}
	return FString();
}

TSet<UEdGraphNode*> UBochaReparentCommandlet::FindNodesConnectedToEvents(UBlueprint* Blueprint, const TArray<FString>& EventNames) const
{
	TSet<UEdGraphNode*> Connected;
	TArray<UEdGraphNode*> Pending;

	TArray<UEdGraphNode*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes);
	for (UEdGraphNode* Node : Nodes)
	{
		if (EventNames.Contains(GetEventName(Node)))
		{
			Pending.Add(Node);
		}
	}

	while (Pending.Num() > 0)
	{
		UEdGraphNode* Node = Pending.Pop();
		if (Connected.Contains(Node))
		{
			continue;
		}
		Connected.Add(Node);
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			for (const UEdGraphPin* Linked : Pin->LinkedTo)
			{
				Pending.Add(Linked->GetOwningNode());
			}
		}
	}
	return Connected;
}

bool UBochaReparentCommandlet::ConvertCustomEventToOverride(UBlueprint* Blueprint, FName EventName) const
{
	TArray<UK2Node_CustomEvent*> CustomEvents;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, CustomEvents);
	UK2Node_CustomEvent** Found = CustomEvents.FindByPredicate(
		[EventName](const UK2Node_CustomEvent* Event) { return Event->CustomFunctionName == EventName; });
	UFunction* Function = Blueprint->ParentClass->FindFunctionByName(EventName);
	if (!Found || !Function)
	{
		UE_LOG(LogBochaReparent, Error, TEXT("%s: custom event or parent function %s not found"), *Blueprint->GetName(), *EventName.ToString());
		return false;
	}
	UK2Node_CustomEvent* CustomEvent = *Found;
	UEdGraph* Graph = CustomEvent->GetGraph();

	UK2Node_Event* EventNode = NewObject<UK2Node_Event>(Graph);
	EventNode->EventReference.SetFromField<UFunction>(Function, false);
	EventNode->bOverrideFunction = true;
	EventNode->NodePosX = CustomEvent->NodePosX;
	EventNode->NodePosY = CustomEvent->NodePosY;
	Graph->AddNode(EventNode, false, false);
	EventNode->CreateNewGuid();
	EventNode->PostPlacedNewNode();
	EventNode->AllocateDefaultPins();

	for (UEdGraphPin* OldPin : CustomEvent->Pins)
	{
		if (OldPin->LinkedTo.Num() == 0)
		{
			continue;
		}
		UEdGraphPin* NewPin = EventNode->FindPin(OldPin->PinName, OldPin->Direction);
		if (!NewPin)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("%s: %s has no pin %s"), *Blueprint->GetName(), *EventName.ToString(), *OldPin->PinName.ToString());
			return false;
		}
		for (UEdGraphPin* Linked : TArray<UEdGraphPin*>(OldPin->LinkedTo))
		{
			NewPin->MakeLinkTo(Linked);
		}
	}

	FBlueprintEditorUtils::RemoveNode(Blueprint, CustomEvent, true);
	UE_LOG(LogBochaReparent, Display, TEXT("%s: custom event %s is now an override"), *Blueprint->GetName(), *EventName.ToString());
	return true;
}

int32 UBochaReparentCommandlet::RetargetFunctionCalls(UBlueprint* Blueprint, UClass* FromClass, UClass* ToClass, const TSharedPtr<FJsonObject>& PinRenames) const
{
	TArray<UK2Node_CallFunction*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes);

	int32 Count = 0;
	for (UK2Node_CallFunction* Node : Nodes)
	{
		const UClass* Parent = Node->FunctionReference.GetMemberParentClass();
		if (Node->FunctionReference.IsSelfContext() || !Parent || Parent->GetAuthoritativeClass() != FromClass)
		{
			continue;
		}
		const FName FunctionName = Node->FunctionReference.GetMemberName();
		UFunction* Function = ToClass->FindFunctionByName(FunctionName);
		if (!Function)
		{
			continue;
		}

		// Renaming the old pins lets ReconstructNode move their links onto the new function's pins.
		const TSharedPtr<FJsonObject>* Renames = nullptr;
		if (PinRenames.IsValid() && PinRenames->TryGetObjectField(FunctionName.ToString(), Renames))
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				FString NewName;
				if ((*Renames)->TryGetStringField(Pin->PinName.ToString(), NewName))
				{
					Pin->PinName = FName(*NewName);
				}
			}
		}

		Node->FunctionReference.SetFromField<UFunction>(Function, false);
		Node->ReconstructNode();
		++Count;
	}

	if (Count > 0)
	{
		UE_LOG(LogBochaReparent, Display, TEXT("%s: retargeted %d function calls %s -> %s"),
			*Blueprint->GetName(), Count, *FromClass->GetName(), *ToClass->GetName());
	}
	return Count;
}

void UBochaReparentCommandlet::RestoreNativeComponents(UBlueprint* Blueprint, const TArray<FString>& ComponentsToReset) const
{
	AActor* ClassDefaults = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
	UObject* ParentDefaults = Blueprint->GeneratedClass->GetSuperClass()->GetDefaultObject();
	if (!ClassDefaults)
	{
		return;
	}

	// A Blueprint variable that had the same name as a new native component (e.g. an SCS component the
	// C++ parent now creates) leaves its old null value in the native pointer after the reparent.
	for (TFieldIterator<FObjectProperty> It(ClassDefaults->GetClass()); It; ++It)
	{
		FObjectProperty* Property = *It;
		if (!ParentDefaults->GetClass()->IsChildOf(Property->GetOwnerClass()))
		{
			continue; // Declared by the Blueprint itself; the parent has no such value.
		}
		UObject* ParentValue = Property->GetObjectPropertyValue_InContainer(ParentDefaults);
		if (Property->GetObjectPropertyValue_InContainer(ClassDefaults) || !ParentValue || ParentValue->GetOuter() != ParentDefaults)
		{
			continue;
		}
		if (UObject* OwnSubobject = ClassDefaults->GetDefaultSubobjectByName(ParentValue->GetFName()))
		{
			Property->SetObjectPropertyValue_InContainer(ClassDefaults, OwnSubobject);
			UE_LOG(LogBochaReparent, Display, TEXT("%s: restored %s -> %s"), *Blueprint->GetName(), *Property->GetName(), *OwnSubobject->GetName());
		}
	}

	// Values the Blueprint used to keep on its own components now come from the C++ constructor.
	for (const FString& Name : ComponentsToReset)
	{
		UObject* Component = ClassDefaults->GetDefaultSubobjectByName(FName(*Name));
		UObject* Archetype = ParentDefaults->GetDefaultSubobjectByName(FName(*Name));
		if (!Component || !Archetype)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("%s: native component %s not found"), *Blueprint->GetName(), *Name);
			continue;
		}
		for (TFieldIterator<FProperty> It(Component->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_InstancedReference | CPF_ContainsInstancedReference)
				|| Property->Identical_InContainer(Component, Archetype))
			{
				continue;
			}
			// Pointers into the parent's defaults (attach parent, etc.) must keep pointing at our own subobjects.
			if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
			{
				const UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(Archetype);
				if (Value && Value->IsIn(ParentDefaults))
				{
					continue;
				}
			}
			Property->CopyCompleteValue_InContainer(Component, Archetype);
			UE_LOG(LogBochaReparent, Display, TEXT("%s: reset %s.%s"), *Blueprint->GetName(), *Name, *Property->GetName());
		}
	}

	ClassDefaults->MarkPackageDirty();
}

bool UBochaReparentCommandlet::ResetDefaults(UBlueprint* Blueprint, const TArray<FString>& PropertyNames) const
{
	// Values left over from the old parent class (they are copied by name on reparent).
	UObject* ClassDefaults = Blueprint->GeneratedClass->GetDefaultObject();
	UObject* ParentDefaults = Blueprint->GeneratedClass->GetSuperClass()->GetDefaultObject();
	for (const FString& Name : PropertyNames)
	{
		FProperty* Property = FindFProperty<FProperty>(ParentDefaults->GetClass(), FName(*Name));
		if (!Property)
		{
			UE_LOG(LogBochaReparent, Error, TEXT("%s: property %s not found"), *Blueprint->GetName(), *Name);
			return false;
		}
		Property->CopyCompleteValue_InContainer(ClassDefaults, ParentDefaults);
		UE_LOG(LogBochaReparent, Display, TEXT("%s: reset %s"), *Blueprint->GetName(), *Name);
	}
	ClassDefaults->MarkPackageDirty();
	return true;
}

bool UBochaReparentCommandlet::SetDefaults(UBlueprint* Blueprint, const TSharedPtr<FJsonObject>& Values) const
{
	UObject* ClassDefaults = Blueprint->GeneratedClass->GetDefaultObject();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Values->Values)
	{
		FProperty* Property = FindFProperty<FProperty>(ClassDefaults->GetClass(), FName(*Pair.Key));
		if (!Property || !Property->ImportText_InContainer(*Pair.Value->AsString(), ClassDefaults, ClassDefaults, PPF_None))
		{
			UE_LOG(LogBochaReparent, Error, TEXT("%s: could not set %s = %s"), *Blueprint->GetName(), *Pair.Key, *Pair.Value->AsString());
			return false;
		}
		UE_LOG(LogBochaReparent, Display, TEXT("%s: %s = %s"), *Blueprint->GetName(), *Pair.Key, *Pair.Value->AsString());
	}
	ClassDefaults->MarkPackageDirty();
	return true;
}

bool UBochaReparentCommandlet::Compile(UBlueprint* Blueprint) const
{
	FCompilerResultsLog Results;
	Results.SetSourcePath(Blueprint->GetPathName());
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &Results);

	for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages)
	{
		if (Message->GetSeverity() <= EMessageSeverity::Warning)
		{
			UE_LOG(LogBochaReparent, Warning, TEXT("%s: %s"), *Blueprint->GetName(), *Message->ToText().ToString());
		}
	}

	if (Blueprint->Status == BS_Error)
	{
		UE_LOG(LogBochaReparent, Error, TEXT("%s failed to compile"), *Blueprint->GetName());
		return false;
	}
	return true;
}
