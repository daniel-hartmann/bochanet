#pragma once

#include "CoreMinimal.h"
#include "Actors/BochaBall.h"
#include "BochaBocha.generated.h"

class UMaterialInterface;

/** A player's bocha ball. Blue balls use BlueMaterial; the others keep the mesh's default material. */
UCLASS()
class BOCHANET_API ABochaBocha : public ABochaBall
{
	GENERATED_BODY()

public:
	ABochaBocha();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_bBlue, Category = "Bocha", meta = (ExposeOnSpawn = "true"))
	bool bBlue = false;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_bBlue();

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	TObjectPtr<UMaterialInterface> BlueMaterial;

private:
	void ApplyColor();
};
