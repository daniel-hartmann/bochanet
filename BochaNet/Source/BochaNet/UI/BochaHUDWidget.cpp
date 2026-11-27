#include "UI/BochaHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Game/BochaGameState.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "BochaHUD"

void UBochaHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	for (int32 Team = 0; Team < UE_ARRAY_COUNT(BallIcons); ++Team)
	{
		for (int32 Index = 1;; ++Index)
		{
			UImage* Icon = Cast<UImage>(WidgetTree->FindWidget(*FString::Printf(TEXT("P%d_B%d"), Team + 1, Index)));
			if (!Icon)
			{
				break;
			}
			BallIcons[Team].Add(Icon);
		}
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!Canvas)
	{
		return;
	}
	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
	FSlateFontInfo Font = MessageText->GetFont();
	Font.Size = MessageFontSize;
	MessageText->SetFont(Font);
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetShadowOffset(FVector2D(2.0, 2.0));
	MessageText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	MessageText->SetVisibility(ESlateVisibility::Collapsed);

	// Centered near the top of the screen, sized to the text.
	UCanvasPanelSlot* MessageSlot = Canvas->AddChildToCanvas(MessageText);
	MessageSlot->SetAnchors(FAnchors(0.5f, 0.12f));
	MessageSlot->SetAlignment(FVector2D(0.5, 0.5));
	MessageSlot->SetPosition(FVector2D::ZeroVector);
	MessageSlot->SetAutoSize(true);

	// Match clock, top center, in the same font as the player rows when there is one.
	ClockText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClockText"));
	FSlateFontInfo ClockFont = P1_Name ? P1_Name->GetFont() : ClockText->GetFont();
	ClockFont.Size = 22;
	ClockText->SetFont(ClockFont);
	ClockText->SetJustification(ETextJustify::Center);
	ClockText->SetShadowOffset(FVector2D(1.0, 1.0));
	ClockText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	ClockText->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* ClockSlot = Canvas->AddChildToCanvas(ClockText);
	ClockSlot->SetAnchors(FAnchors(0.5f, 0.f));
	ClockSlot->SetAlignment(FVector2D(0.5, 0.0));
	ClockSlot->SetPosition(FVector2D(0.0, 12.0));
	ClockSlot->SetAutoSize(true);
}

void UBochaHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateClock();
	GetWorld()->GetTimerManager().SetTimer(ClockTimer, this, &UBochaHUDWidget::UpdateClock, 1.f, true);
}

void UBochaHUDWidget::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ClockTimer);
	}
	Super::NativeDestruct();
}

void UBochaHUDWidget::UpdateClock()
{
	if (!ClockText)
	{
		return;
	}
	const ABochaGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ABochaGameState>() : nullptr;
	if (!GameState || GameState->GetMatchPhase() == EBochaMatchPhase::WaitingForPlayers)
	{
		ClockText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const int32 Seconds = FMath::FloorToInt(GameState->GetMatchTime());
	ClockText->SetText(FText::Format(LOCTEXT("Clock", "Tempo de partida {0}:{1}"),
		FText::AsNumber(Seconds / 60, &FNumberFormattingOptions::DefaultNoGrouping()),
		FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60))));
	ClockText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBochaHUDWidget::ShowMessage(const FText& Message, float Duration)
{
	if (!MessageText)
	{
		return;
	}
	MessageText->SetText(Message);
	MessageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Duration < 0.f)
	{
		Duration = MessageDuration;
	}
	if (Duration > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(MessageTimer, this, &UBochaHUDWidget::HideMessage, Duration);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(MessageTimer);
	}
}

void UBochaHUDWidget::HideMessage()
{
	MessageText->SetVisibility(ESlateVisibility::Collapsed);
}

float UBochaHUDWidget::GetThrowStrength() const
{
	return Slider_Strength->GetValue();
}

void UBochaHUDWidget::SetThrowStrength(float Strength)
{
	Slider_Strength->SetValue(FMath::Clamp(Strength, MinThrowStrength, MaxThrowStrength));
}

void UBochaHUDWidget::ShowPlayers(int32 CurrentTeam, int32 LocalTeam, int32 PointHolder, int32 RedScore, int32 BlueScore)
{
	const FText TeamNames[] = { LOCTEXT("Red", "Vermelho"), LOCTEXT("Blue", "Azul") };
	const int32 Scores[] = { RedScore, BlueScore };
	UTextBlock* const Rows[] = { P1_Name, P2_Name };
	for (int32 Team = 0; Team < UE_ARRAY_COUNT(Rows); ++Team)
	{
		if (!Rows[Team])
		{
			continue;
		}
		FText Name = TeamNames[Team];
		if (Team == LocalTeam)
		{
			Name = FText::Format(LOCTEXT("You", "{0} (você)"), Name);
		}
		Name = FText::Format(LOCTEXT("Score", "{0}: {1}"), Name, Scores[Team]);
		if (Team == PointHolder)
		{
			Name = FText::Format(LOCTEXT("Point", "{0}  (ponto)"), Name);
		}
		if (Team == CurrentTeam)
		{
			Name = FText::Format(LOCTEXT("Turn", "> {0}"), Name);
		}
		Rows[Team]->SetText(Name);
	}
}

void UBochaHUDWidget::ShowBochasLeft(int32 Team, int32 Left, int32 PerPlayer)
{
	if (Team < 0 || Team >= UE_ARRAY_COUNT(BallIcons))
	{
		return;
	}
	const int32 Thrown = PerPlayer - Left;
	for (int32 Index = 0; Index < BallIcons[Team].Num(); ++Index)
	{
		UImage* Icon = BallIcons[Team][Index];
		Icon->SetVisibility(Index < PerPlayer ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		// Thrown bochas fade from the left, so the row keeps its layout.
		Icon->SetRenderOpacity(Index < Thrown ? 0.2f : 1.f);
	}
}

void UBochaHUDWidget::ShowBolimThrower(int32 Team)
{
	P1_Bolim->SetVisibility(Team == 0 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	P2_Bolim->SetVisibility(Team == 1 ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

#undef LOCTEXT_NAMESPACE
