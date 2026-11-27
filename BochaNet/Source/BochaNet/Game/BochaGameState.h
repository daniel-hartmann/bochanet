#pragma once

#include "CoreMinimal.h"
#include "Game/BochaMatch.h"
#include "GameFramework/GameStateBase.h"
#include "BochaGameState.generated.h"

/** Life cycle of the match, controlled by the server. */
UENUM(BlueprintType)
enum class EBochaMatchPhase : uint8
{
	/** In the lobby, until there are enough players and one of them starts the match at the portal. */
	WaitingForPlayers,
	/** The bocha match is being played on the court. */
	InProgress,
	/** Someone won; the result is on screen until everyone goes back to the lobby. */
	Finished,
};

/**
 * Global state of the game, replicated to every machine; only the server changes it:
 * - the match life cycle (WaitingForPlayers -> InProgress -> Finished), with the number of players;
 * - the match clock;
 * - the turn: rules phase of the match and the team that throws (written by ABochaMatch);
 * - the winner.
 * Each player's points and bochas are in ABochaPlayerState. ABochaMatch keeps the rules and the balls.
 */
UCLASS()
class BOCHANET_API ABochaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Bocha")
	EBochaMatchPhase GetMatchPhase() const { return MatchPhase; }

	/** Seconds since the match started (frozen when it finishes; 0 while waiting). Same on every machine. */
	UFUNCTION(BlueprintPure, Category = "Bocha")
	float GetMatchTime() const;

	/** Team that won the last match, or INDEX_NONE. */
	UFUNCTION(BlueprintPure, Category = "Bocha")
	int32 GetWinnerTeam() const { return WinnerTeam; }

	/** Rules phase of the match in progress (whose turn, balls rolling, measuring...). */
	EBochaPhase GetTurnPhase() const { return TurnPhase; }

	/** Team that throws now, or INDEX_NONE. */
	UFUNCTION(BlueprintPure, Category = "Bocha")
	int32 GetCurrentTeam() const { return CurrentTeam; }

	UFUNCTION(BlueprintPure, Category = "Bocha")
	int32 GetConnectedPlayers() const { return ConnectedPlayers; }

	/** Players needed to start a match (ABochaGameMode::RequiredPlayers). */
	UFUNCTION(BlueprintPure, Category = "Bocha")
	int32 GetRequiredPlayers() const { return RequiredPlayers; }

	UFUNCTION(BlueprintPure, Category = "Bocha")
	bool HasEnoughPlayers() const { return ConnectedPlayers >= RequiredPlayers; }

	/** Server: the match starts (WaitingForPlayers -> InProgress) and the clock starts. */
	void StartMatch();

	/** Server: the match ends with Winner (InProgress -> Finished) and the clock stops. */
	void FinishMatch(int32 Winner);

	/** Server: back to the lobby (-> WaitingForPlayers). */
	void ResetMatch();

	/** Server: ABochaMatch moved to another rules phase or turn. */
	void SetTurn(EBochaPhase NewPhase, int32 NewTeam);

	/** Server: ABochaGameMode counted the players (on login and logout). */
	void SetPlayerCount(int32 Connected, int32 Required);

	/** Fired on every machine when the match phase or the number of players changes. */
	FSimpleMulticastDelegate OnMatchStateChanged;

protected:
	UFUNCTION()
	void OnRep_MatchState();

	UFUNCTION()
	void OnRep_Turn();

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	EBochaMatchPhase MatchPhase = EBochaMatchPhase::WaitingForPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	int32 ConnectedPlayers = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	int32 RequiredPlayers = 2;

	/** Server world time when the match started; clients compare it with GetServerWorldTimeSeconds. */
	UPROPERTY(Replicated)
	double MatchStartTime = 0.0;

	/** Server world time when the match finished. */
	UPROPERTY(Replicated)
	double MatchEndTime = 0.0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	int32 WinnerTeam = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_Turn)
	EBochaPhase TurnPhase = EBochaPhase::WaitingForPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_Turn)
	int32 CurrentTeam = INDEX_NONE;

private:
	void SetMatchPhase(EBochaMatchPhase NewPhase);
};
