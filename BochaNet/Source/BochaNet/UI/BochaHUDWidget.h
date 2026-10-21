#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BochaHUDWidget.generated.h"

class UImage;
class USlider;
class UTextBlock;

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

	/**
	 * Fills the two player rows: name, "(você)" on the local player's, "> " on whose turn it is
	 * (INDEX_NONE: nobody), the score and "(ponto)" on the side holding the point.
	 */
	void ShowPlayers(int32 CurrentTeam, int32 LocalTeam, int32 PointHolder, int32 RedScore, int32 BlueScore);

	/** Team's ball icons: one per bocha of the round, the ones already thrown dimmed. */
	void ShowBochasLeft(int32 Team, int32 Left, int32 PerPlayer);

	/** Shows the jack icon on Team's row only (INDEX_NONE hides both). */
	void ShowBolimThrower(int32 Team);

	/**
	 * Shows a rules message in large type at the top of the screen, for Duration seconds
	 * (negative: MessageDuration; 0: until the next message).
	 */
	void ShowMessage(const FText& Message, float Duration = -1.f);

protected:
	virtual void NativeOnInitialized() override;

	/** How long a message stays on screen (s). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha|HUD")
	float MessageDuration = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha|HUD")
	int32 MessageFontSize = 34;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Strength;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> P1_Bolim;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> P2_Bolim;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> P1_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> P2_Name;

private:
	void HideMessage();

	/** P1_B1..P1_B5 and P2_B1..P2_B5 from WBP_HUD, per team. */
	TArray<TObjectPtr<UImage>> BallIcons[2];

	/** Created at runtime on the root canvas, so the WBP_HUD layout stays untouched. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	FTimerHandle MessageTimer;
};
