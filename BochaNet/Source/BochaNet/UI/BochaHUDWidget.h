#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BochaHUDWidget.generated.h"

class UImage;
class USlider;

/** In-match HUD: throw strength slider and the players' ball rows. Layout lives in WBP_HUD. */
UCLASS(Abstract)
class BOCHANET_API UBochaHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr float MinThrowStrength = 100.f;
	static constexpr float MaxThrowStrength = 1000.f;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Bocha|HUD")
	float GetThrowStrength() const;

	/** Clamped to [MinThrowStrength, MaxThrowStrength]. */
	UFUNCTION(BlueprintCallable, Category = "Bocha|HUD")
	void SetThrowStrength(float Strength);

	/** Shows the jack icon on player 1's row and hides it on player 2's. */
	UFUNCTION(BlueprintCallable, Category = "Bocha|HUD")
	void SetP1Bolim();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Strength;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> P1_Bolim;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> P2_Bolim;
};
