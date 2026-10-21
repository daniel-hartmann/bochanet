#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BochaStateScreenWidget.generated.h"

class UButton;
class UTextBlock;

UENUM(BlueprintType)
enum class EBochaScreen : uint8
{
	Start,
	Pause,
	Victory,
	Defeat,
};

/**
 * Full-screen game state card (WBP_GameStateScreen): start of the match, local pause, victory and
 * defeat. Shown on demand by the minigame manager (events), never polled.
 */
UCLASS(Abstract)
class BOCHANET_API UBochaStateScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows Screen with Subtitle, for Duration seconds (0: until hidden). */
	void Show(EBochaScreen Screen, const FText& Subtitle, float Duration = 0.f);
	void Hide();

	bool IsShowing(EBochaScreen Screen) const { return IsVisible() && Current == Screen; }

	/** The pause screen's "Continuar" button. */
	FSimpleMulticastDelegate OnContinue;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SubtitleText;

	/** Controls reminder (start and pause). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ControlsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha|Screen")
	FSlateColor VictoryColor = FSlateColor(FLinearColor(0.3f, 1.f, 0.4f));

	UPROPERTY(EditDefaultsOnly, Category = "Bocha|Screen")
	FSlateColor DefeatColor = FSlateColor(FLinearColor(1.f, 0.35f, 0.3f));

private:
	UFUNCTION()
	void HandleContinueClicked();

	EBochaScreen Current = EBochaScreen::Start;
	FTimerHandle HideTimer;
};
