#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BochaLobbyWidget.generated.h"

class ABochaGameState;
class UTextBlock;

/**
 * Lobby banner, built in C++ (no widget Blueprint): "waiting for players (1/2)" until enough players
 * are connected, then how to start the match. Hidden while a match is in progress. Driven by
 * ABochaGameState::OnMatchStateChanged (no Tick, no property binding).
 */
UCLASS()
class BOCHANET_API UBochaLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	/** Binds to the game state once it has replicated to this machine. */
	void TryBindGameState();
	void Refresh();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	TWeakObjectPtr<ABochaGameState> BoundGameState;
	FTimerHandle BindTimer;
};
