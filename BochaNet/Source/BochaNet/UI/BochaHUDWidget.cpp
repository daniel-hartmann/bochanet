#include "UI/BochaHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
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
