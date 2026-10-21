#include "UI/BochaStateScreenWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "BochaStateScreen"

void UBochaStateScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ContinueButton->OnClicked.AddDynamic(this, &UBochaStateScreenWidget::HandleContinueClicked);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UBochaStateScreenWidget::Show(EBochaScreen Screen, const FText& Subtitle, float Duration)
{
	Current = Screen;

	static const FText Titles[] = {
		LOCTEXT("Start", "Bocha"),
		LOCTEXT("Pause", "Pausa"),
		LOCTEXT("Victory", "Vitória!"),
		LOCTEXT("Defeat", "Derrota"),
	};
	TitleText->SetText(Titles[static_cast<int32>(Screen)]);
	TitleText->SetColorAndOpacity(Screen == EBochaScreen::Victory ? VictoryColor
		: Screen == EBochaScreen::Defeat ? DefeatColor : FSlateColor(FLinearColor::White));
	SubtitleText->SetText(Subtitle);

	const bool bShowControls = Screen == EBochaScreen::Start || Screen == EBochaScreen::Pause;
	ControlsText->SetText(LOCTEXT("Controls",
		"A/D: andar   Q/E ou mouse: mirar   W/S: força   Espaço: arremessar   P: pausa"));
	ControlsText->SetVisibility(bShowControls ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	ContinueButton->SetVisibility(Screen == EBochaScreen::Pause ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// Only the pause screen takes the mouse; the others are drawn over the game.
	SetVisibility(Screen == EBochaScreen::Pause ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(HideTimer);
	if (Duration > 0.f)
	{
		Timers.SetTimer(HideTimer, this, &UBochaStateScreenWidget::Hide, Duration);
	}
}

void UBochaStateScreenWidget::Hide()
{
	GetWorld()->GetTimerManager().ClearTimer(HideTimer);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UBochaStateScreenWidget::HandleContinueClicked()
{
	OnContinue.Broadcast();
}

#undef LOCTEXT_NAMESPACE
