#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BochaPlayerState.generated.h"

/**
 * Each player's own state in the bocha match, replicated to every machine: team, points and bochas
 * left in the round. Only the server changes it (through ABochaMatch).
 * Playing alone (BochaSoloGameMode), the blue team is a second ABochaPlayerState flagged as a bot,
 * thrown for by the same player (see ABochaMatch::IsOnTeam).
 */
UCLASS()
class BOCHANET_API ABochaPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 0 red, 1 blue; INDEX_NONE outside a match. */
	UFUNCTION(BlueprintPure, Category = "Bocha")
	int32 GetTeam() const { return Team; }

	/** Points in the current match. */
	UFUNCTION(BlueprintPure, Category = "Bocha")
	int32 GetPoints() const { return Points; }

	/** Bochas this player can still throw in the current round. */
	UFUNCTION(BlueprintPure, Category = "Bocha")
	int32 GetBochasLeft() const { return BochasLeft; }

	/** Server: joins the match on NewTeam with no points. */
	void StartMatch(int32 NewTeam);

	/** Server: leaves the match (back to the lobby). Does not notify the match, which is going away. */
	void ClearMatch();

	/** Server. */
	void AddPoints(int32 Amount);

	/** Server. */
	void SetBochasLeft(int32 Count);

protected:
	UFUNCTION()
	void OnRep_BochaState();

	UPROPERTY(ReplicatedUsing = OnRep_BochaState)
	int32 Team = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_BochaState)
	int32 Points = 0;

	UPROPERTY(ReplicatedUsing = OnRep_BochaState)
	int32 BochasLeft = 0;

private:
	/** Sends the change now (player states update rarely by default) and refreshes the HUD on the host. */
	void Changed();
};
