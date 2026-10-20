#include "BochaExportBlueprintsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "WidgetBlueprint.h"

namespace
{
	const TCHAR* PackagePathsToExport[] = { TEXT("/Game/Bocha"), TEXT("/Game/Lobby") };

	// Blueprints outside the folders above that the Bocha logic depends on.
	const TCHAR* ExtraAssetsToExport[] = {
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController.BP_ThirdPersonPlayerController"),
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter"),
	};

	FString SanitizeFileName(const FString& Name)
	{
		FString Result = Name;
		for (TCHAR& Char : Result)
		{
			if (!FChar::IsAlnum(Char) && Char != TEXT('_') && Char != TEXT('-'))
			{
				Char = TEXT('_');
			}
		}
		return Result;
	}

	FString PinTypeToString(const FEdGraphPinType& PinType)
	{
		return UEdGraphSchema_K2::TypeToText(PinType).ToString();
	}

	// Lists every property of Object whose value differs from Archetype (or from the class defaults).
	FString DescribeOverrides(const UObject* Object, const UObject* Archetype)
	{
		if (!Object)
		{
			return TEXT("");
		}
		if (!Archetype)
		{
			Archetype = Object->GetClass()->GetDefaultObject();
		}

		FString Out;
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			const FProperty* Property = *It;
			if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_Deprecated))
			{
				continue;
			}
			FString Value;
			if (!Archetype->GetClass()->IsChildOf(Property->GetOwnerClass()))
			{
				// Declared by this Blueprint: there is no parent value to compare with.
				Property->ExportText_InContainer(0, Value, Object, nullptr, const_cast<UObject*>(Object), PPF_None);
				Out += FString::Printf(TEXT("    %s = %s (own)\n"), *Property->GetName(), *Value.Left(400));
				continue;
			}
			if (Property->Identical_InContainer(Object, Archetype, 0, PPF_None))
			{
				continue;
			}
			Property->ExportText_InContainer(0, Value, Object, Archetype, const_cast<UObject*>(Object), PPF_None);
			Out += FString::Printf(TEXT("    %s = %s\n"), *Property->GetName(), *Value.Left(400));
		}
		return Out;
	}
}

UBochaExportBlueprintsCommandlet::UBochaExportBlueprintsCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UBochaExportBlueprintsCommandlet::Main(const FString& Params)
{
	FString OutDir;
	if (!FParse::Value(*Params, TEXT("Out="), OutDir))
	{
		OutDir = FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), TEXT("Docs"), TEXT("bp-export"));
	}
	OutDir = FPaths::ConvertRelativePathToFull(OutDir);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	for (const TCHAR* Path : PackagePathsToExport)
	{
		Filter.PackagePaths.Add(FName(Path));
	}
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	for (const TCHAR* ObjectPath : ExtraAssetsToExport)
	{
		const FAssetData Extra = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(ObjectPath));
		if (Extra.IsValid())
		{
			Assets.Add(Extra);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Extra asset not found: %s"), ObjectPath);
		}
	}

	int32 Exported = 0;
	for (const FAssetData& Asset : Assets)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
		{
			ExportBlueprint(Blueprint, OutDir);
			++Exported;
		}
	}

	UE_LOG(LogTemp, Display, TEXT("Exported %d blueprints to %s"), Exported, *OutDir);
	return 0;
}

void UBochaExportBlueprintsCommandlet::ExportBlueprint(UBlueprint* Blueprint, const FString& OutDir) const
{
	const FString BlueprintDir = FPaths::Combine(OutDir, Blueprint->GetName());
	IFileManager::Get().MakeDirectory(*BlueprintDir, true);

	// Summary: parent, variables, components, widgets, graph list.
	FString Summary;
	Summary += FString::Printf(TEXT("Blueprint: %s\n"), *Blueprint->GetPathName());
	Summary += FString::Printf(TEXT("Parent: %s\n"), Blueprint->ParentClass ? *Blueprint->ParentClass->GetPathName() : TEXT("None"));

	Summary += TEXT("\n[Variables]\n");
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		TArray<FString> Flags;
		if (Var.PropertyFlags & CPF_Net) { Flags.Add(TEXT("Replicated")); }
		if (!Var.RepNotifyFunc.IsNone()) { Flags.Add(FString::Printf(TEXT("RepNotify=%s"), *Var.RepNotifyFunc.ToString())); }
		if (Var.PropertyFlags & CPF_ExposeOnSpawn) { Flags.Add(TEXT("ExposeOnSpawn")); }
		if (!(Var.PropertyFlags & CPF_DisableEditOnInstance)) { Flags.Add(TEXT("InstanceEditable")); }
		Summary += FString::Printf(TEXT("%s : %s = '%s' [%s] Category=%s\n"),
			*Var.VarName.ToString(), *PinTypeToString(Var.VarType), *Var.DefaultValue,
			*FString::Join(Flags, TEXT(", ")), *Var.Category.ToString());
	}

	if (Blueprint->SimpleConstructionScript)
	{
		Summary += TEXT("\n[Components]\n");
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			const USCS_Node* ParentNode = Blueprint->SimpleConstructionScript->FindParentNode(Node);
			Summary += FString::Printf(TEXT("%s : %s (parent: %s, socket: %s)\n"),
				*Node->GetVariableName().ToString(),
				Node->ComponentClass ? *Node->ComponentClass->GetName() : TEXT("None"),
				ParentNode ? *ParentNode->GetVariableName().ToString() : *Node->ParentComponentOrVariableName.ToString(),
				*Node->AttachToName.ToString());
		}
	}

	if (Blueprint->GeneratedClass)
	{
		const UObject* ClassDefaults = Blueprint->GeneratedClass->GetDefaultObject();
		UObject* ParentDefaults = Blueprint->GeneratedClass->GetSuperClass()->GetDefaultObject();
		Summary += TEXT("\n[Class Defaults overridden vs parent]\n");
		Summary += DescribeOverrides(ClassDefaults, ParentDefaults);

		// Native components inherited from C++ parents, as modified by this Blueprint.
		if (const AActor* ActorDefaults = Cast<AActor>(ClassDefaults))
		{
			TInlineComponentArray<UActorComponent*> NativeComponents;
			ActorDefaults->GetComponents(NativeComponents);
			for (const UActorComponent* Component : NativeComponents)
			{
				const UObject* ParentComponent = ParentDefaults->GetDefaultSubobjectByName(Component->GetFName());
				Summary += FString::Printf(TEXT("\n[Native component %s : %s overridden vs parent]\n"),
					*Component->GetName(), *Component->GetClass()->GetName());
				Summary += DescribeOverrides(Component, ParentComponent);
			}
		}
	}

	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			Summary += FString::Printf(TEXT("\n[Component template %s : %s overridden vs class defaults]\n"),
				*Node->GetVariableName().ToString(), Node->ComponentClass ? *Node->ComponentClass->GetName() : TEXT("None"));
			Summary += DescribeOverrides(Node->ComponentTemplate, nullptr);
		}
	}

	if (UInheritableComponentHandler* Handler = Blueprint->GetInheritableComponentHandler(false))
	{
		TArray<UActorComponent*> Templates;
		Handler->GetAllTemplates(Templates);
		for (UActorComponent* Template : Templates)
		{
			Summary += FString::Printf(TEXT("\n[Inherited component override %s : %s overridden vs parent template]\n"),
				*Template->GetName(), *Template->GetClass()->GetName());
			Summary += DescribeOverrides(Template, Template->GetArchetype());
		}
	}

	if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint))
	{
		Summary += TEXT("\n[Widgets]\n");
		if (WidgetBlueprint->WidgetTree)
		{
			WidgetBlueprint->WidgetTree->ForEachWidget([&Summary](UWidget* Widget)
			{
				Summary += FString::Printf(TEXT("%s : %s (parent: %s)%s\n"),
					*Widget->GetName(), *Widget->GetClass()->GetName(),
					Widget->GetParent() ? *Widget->GetParent()->GetName() : TEXT("root"),
					Widget->bIsVariable ? TEXT(" [IsVariable]") : TEXT(""));
				Summary += DescribeOverrides(Widget, nullptr);
			});
		}
	}

	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);

	Summary += TEXT("\n[Graphs]\n");
	TSet<FString> UsedNames;
	for (UEdGraph* Graph : Graphs)
	{
		FString BaseName = SanitizeFileName(Graph->GetName());
		FString FileName = BaseName;
		for (int32 Suffix = 1; UsedNames.Contains(FileName); ++Suffix)
		{
			FileName = FString::Printf(TEXT("%s_%d"), *BaseName, Suffix);
		}
		UsedNames.Add(FileName);

		Summary += FString::Printf(TEXT("%s (%d nodes) -> %s.txt\n"), *Graph->GetName(), Graph->Nodes.Num(), *FileName);

		FFileHelper::SaveStringToFile(DescribeGraph(Graph), *FPaths::Combine(BlueprintDir, FileName + TEXT(".txt")));

		TSet<UObject*> NodesToExport;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			NodesToExport.Add(Node);
		}
		FString T3D;
		FEdGraphUtilities::ExportNodesToText(NodesToExport, T3D);
		FFileHelper::SaveStringToFile(T3D, *FPaths::Combine(BlueprintDir, FileName + TEXT(".t3d")));
	}

	FFileHelper::SaveStringToFile(Summary, *FPaths::Combine(BlueprintDir, TEXT("_summary.txt")));
}

FString UBochaExportBlueprintsCommandlet::DescribeGraph(UEdGraph* Graph) const
{
	TMap<const UEdGraphNode*, int32> NodeIds;
	for (int32 Index = 0; Index < Graph->Nodes.Num(); ++Index)
	{
		NodeIds.Add(Graph->Nodes[Index], Index);
	}

	auto PinRef = [&NodeIds](const UEdGraphPin* Pin)
	{
		const int32* Id = NodeIds.Find(Pin->GetOwningNode());
		return FString::Printf(TEXT("N%d.%s"), Id ? *Id : -1, *Pin->PinName.ToString());
	};

	FString Out = FString::Printf(TEXT("Graph: %s\n\n"), *Graph->GetName());
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		const bool bDisabled = !Node->IsNodeEnabled();
		Out += FString::Printf(TEXT("[N%d] %s (%s)%s\n"),
			NodeIds[Node],
			*Node->GetNodeTitle(ENodeTitleType::ListView).ToString().Replace(TEXT("\n"), TEXT(" / ")),
			*Node->GetClass()->GetName(),
			bDisabled ? TEXT(" {DISABLED}") : TEXT(""));

		if (!Node->NodeComment.IsEmpty())
		{
			Out += FString::Printf(TEXT("    // %s\n"), *Node->NodeComment.Replace(TEXT("\n"), TEXT(" ")));
		}

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->bHidden && Pin->LinkedTo.Num() == 0)
			{
				continue;
			}

			const TCHAR* Dir = Pin->Direction == EGPD_Input ? TEXT("in ") : TEXT("out");
			const FString Type = PinTypeToString(Pin->PinType);

			if (Pin->LinkedTo.Num() > 0)
			{
				TArray<FString> Links;
				for (const UEdGraphPin* Linked : Pin->LinkedTo)
				{
					Links.Add(PinRef(Linked));
				}
				Out += FString::Printf(TEXT("    %s %s : %s %s %s\n"), Dir, *Pin->PinName.ToString(), *Type,
					Pin->Direction == EGPD_Input ? TEXT("<-") : TEXT("->"), *FString::Join(Links, TEXT(", ")));
			}
			else if (Pin->Direction == EGPD_Input)
			{
				FString Value = Pin->DefaultObject ? Pin->DefaultObject->GetPathName()
					: !Pin->DefaultTextValue.IsEmpty() ? Pin->DefaultTextValue.ToString()
					: Pin->DefaultValue;
				if (!Value.IsEmpty())
				{
					Out += FString::Printf(TEXT("    in  %s : %s = %s\n"), *Pin->PinName.ToString(), *Type, *Value);
				}
			}
		}
		Out += TEXT("\n");
	}
	return Out;
}
