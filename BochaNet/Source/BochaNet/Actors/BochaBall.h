#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaBall.generated.h"

class UStaticMeshComponent;

/** Base for every ball on the court: a replicated physics sphere. */
UCLASS()
class BOCHANET_API ABochaBall : public AActor
{
	GENERATED_BODY()

public:
	ABochaBall();

	UStaticMeshComponent* GetSphere() const { return Sphere; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bocha")
	TObjectPtr<UStaticMeshComponent> Sphere;
};
