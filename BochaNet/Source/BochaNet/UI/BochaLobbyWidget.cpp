#include "UI/BochaLobbyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Game/BochaGameState.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "BochaLobby"

void UBochaLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LobbyRoot"));
	WidgetTree->RootWidget = Canvas;

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	FSlateFontInfo Font = StatusText->GetFont();
	Font.Size = 26;
	StatusText->SetFont(Font);
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetShadowOffset(FVector2D(2.0, 2.0));
	StatusText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));

	// Top center.
	UCanvasPanelSlot* StatusSlot = Canvas->AddChildToCanvas(StatusText);
	StatusSlot->SetAnchors(FAnchors(0.5f, 0.08f));
	StatusSlot->SetAlignment(FVector2D(0.5, 0.5));
	StatusSlot->SetPosition(FVector2D::ZeroVector);
	StatusSlot->SetAutoSize(true);
}

void UBochaLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	TryBindGameState();
}

void UBochaLobbyWidget::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindTimer);
	}
	if (ABochaGameState* GameState = BoundGameState.Get())
	{
		GameState->OnMatchStateChanged.RemoveAll(this);
	}
	BoundGameState = nullptr;
	Super::NativeDestruct();
}

void UBochaLobbyWidget::TryBindGameState()
{
	UWorld* World = GetWorld();
	ABochaGameState* GameState = World ? World->GetGameState<ABochaGameState>() : nullptr;
	if (!GameState)
	{
		// The game state may not have replicated yet on a client.
		if (StatusText)
		{
			StatusText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (World)
		{
			World->GetTimerManager().SetTimer(BindTimer, this, &UBochaLobbyWidget::TryBindGameState, 0.5f, false);
		}
		return;
	}
	BoundGameState = GameState;
	GameState->OnMatchStateChanged.AddUObject(this, &UBochaLobbyWidget::Refresh);
	Refresh();
}

void UBochaLobbyWidget::Refresh()
{
	const ABochaGameState* GameState = BoundGameState.Get();
	if (!StatusText || !GameState)
	{
		return;
	}
	if (GameState->GetMatchPhase() != EBochaMatchPhase::WaitingForPlayers)
	{
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	StatusText->SetText(GameState->HasEnoughPlayers()
		? FText::Format(LOCTEXT("Ready", "Jogadores: {0}/{1}. Vá até o portal e interaja para começar a partida."),
			GameState->GetConnectedPlayers(), GameState->GetRequiredPlayers())
		: FText::Format(LOCTEXT("Waiting", "Aguardando jogadores: {0}/{1}"),
			GameState->GetConnectedPlayers(), GameState->GetRequiredPlayers()));
	StatusText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

#undef LOCTEXT_NAMESPACE
