#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BochaMatch.generated.h"

class ABochaBall;
class ABochaBolim;
class APlayerState;
class UPrimitiveComponent;
struct FHitResult;

UENUM(BlueprintType)
enum class EBochaPhase : uint8
{
	WaitingForPlayers,
	ThrowingBolim,
	ThrowingBochas,
	/** A thrown ball is still rolling; nobody can throw. */
	BallsMoving,
	/** All bochas of the round were thrown; the next round starts after a pause. */
	RoundOver,
	/** Someone won; the minigame ends after a pause. */
	GameOver,
};

/**
 * Court markings (CBTG rules, Art. 2 a, 26,50 m court) in CanchaMap coordinates, in cm.
 * Play always goes from end A towards end A' (+X).
 */
USTRUCT(BlueprintType)
struct FBochaCourt
{
	GENERATED_BODY()

	/** End A, behind the players (no board there). */
	UPROPERTY(EditAnywhere, Category = "Court")
	float A = -2600.f;

	/** Throws are released behind this line (4 m). */
	UPROPERTY(EditAnywhere, Category = "Court")
	float B = -2200.f;

	/** Limit for the "tiro" throw (7 m). Drawn only. */
	UPROPERTY(EditAnywhere, Category = "Court")
	float C = -1900.f;

	/** 9 m. Drawn only. */
	UPROPERTY(EditAnywhere, Category = "Court")
	float D = -1700.f;

	/** Middle of the court: the jack must fully cross it. */
	UPROPERTY(EditAnywhere, Category = "Court")
	float E = -1275.f;

	/** The jack must not reach it (4 m before A'). */
	UPROPERTY(EditAnywhere, Category = "Court")
	float BPrime = -350.f;

	/** End A', the back board. */
	UPROPERTY(EditAnywhere, Category = "Court")
	float APrime = 50.f;

	UPROPERTY(EditAnywhere, Category = "Court")
	float SideMin = -400.f;

	UPROPERTY(EditAnywhere, Category = "Court")
	float SideMax = 0.f;

	/** Top of the floor. */
	UPROPERTY(EditAnywhere, Category = "Court")
	float FloorZ = 50.f;

	/** A jack this close to a side board (or closer) is irregular (Art. 9 h III). */
	UPROPERTY(EditAnywhere, Category = "Court")
	float SideMargin = 13.f;
};

/**
 * The bocha match shared by both players. Spawned by the server when the minigame starts and
 * replicated to every machine; only the server changes it. Team 0 plays red, team 1 blue.
 */
UCLASS()
class BOCHANET_API ABochaMatch : public AActor
{
	GENERATED_BODY()

public:
	static constexpr int32 NumTeams = 2;

	ABochaMatch();

	/** The match in the world of WorldContext, if it has been spawned (or replicated) yet. */
	static ABochaMatch* Find(const UObject* WorldContext);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Where CanchaMap was loaded; set by the server before the match finishes spawning. */
	void SetCourtOrigin(const FVector& Origin) { CourtOrigin = Origin; }

	/** Whether PlayerState may throw now. */
	bool CanThrow(const APlayerState* PlayerState) const;

	/** Whether a ball released at WorldLocation is behind line B (Art. 2 a II). */
	bool IsBehindThrowLine(const FVector& WorldLocation) const;

	/** World height of the court floor. */
	double GetFloorZ() const { return CourtOrigin.Z + Court.FloorZ; }

	const FBochaCourt& GetCourt() const { return Court; }

	/** World location in CanchaMap coordinates (where Court is measured). */
	FVector ToCourt(const FVector& WorldLocation) const { return WorldLocation - CourtOrigin; }

	/** Server: starts watching a ball that was just thrown by the current thrower. */
	void RegisterThrow(ABochaBall* Ball);

	/**
	 * Server: launch velocity for the jack. Strength (0..1) picks where it stops, between
	 * BolimLineMargin past line E and BolimLineMargin short of line B'; it always goes straight
	 * down the court and is braked to stop there, so it is always valid.
	 */
	FVector LaunchBolim(ABochaBall* JackBall, float Strength) const;

	/** Team of PlayerState, or INDEX_NONE. Playing alone, the same player is both teams; this returns 0. */
	int32 GetTeamOf(const APlayerState* PlayerState) const;

	EBochaPhase GetPhase() const { return Phase; }
	int32 GetCurrentThrower() const { return CurrentThrower; }
	int32 GetBochasLeft(int32 Team) const { return BochasLeft.IsValidIndex(Team) ? BochasLeft[Team] : 0; }
	int32 GetBochasPerPlayer() const { return BochasPerPlayer; }
	int32 GetScore(int32 Team) const { return Score.IsValidIndex(Team) ? Score[Team] : 0; }
	/** Team whose bocha is closest to the jack, or INDEX_NONE (no bochas, or a tie). */
	int32 GetPointHolder() const { return PointHolder; }

	/** "Vermelho" / "Azul". */
	static FText GetTeamName(int32 Team);

	/** The ball of the throw being watched (while BallsMoving), or null. */
	ABochaBall* GetThrownBall() const { return ThrownBall; }

	/** Team that reached TargetScore (set before the victory is confirmed), or INDEX_NONE. */
	int32 GetWinner() const { return Winner; }

	/** Whether PlayerState plays for Team (playing alone, the same player is both teams). */
	bool IsOnTeam(const APlayerState* PlayerState, int32 Team) const
	{
		return PlayerState && Players.IsValidIndex(Team) && Players[Team] == PlayerState;
	}

	/** Server: the winner's machine claims the victory; confirmed when the score backs it up. */
	void HandleVictoryClaim(const APlayerState* Claimer);

	/** Fired on every machine when the game is over: winner team, red score, blue score. */
	TMulticastDelegate<void(int32, int32, int32)> OnGameOver;
	bool IsThrowingBolim() const { return Phase == EBochaPhase::ThrowingBolim; }

	/** Fired on every machine whenever the replicated state changes. */
	FSimpleMulticastDelegate OnStateChanged;

	/** Fired on every machine with each rules message. */
	TMulticastDelegate<void(const FText&)> OnAnnounce;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Points to win the match (Art. 5 a: 12 to 15). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	int32 TargetScore = 12;

	/** The winner's machine has this long to claim the victory before the server ends the game anyway (s). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float VictoryClaimTimeout = 3.f;

	/** How long the result stays on screen before everyone goes back to the lobby (s). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float GameOverDuration = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	FBochaCourt Court;

	/** Draws the court lines with debug lines (development builds). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	bool bDrawCourt = true;

	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	int32 BochasPerPlayer = 4;

	/** A ball counts as stopped below this speed (cm/s)... */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float StoppedSpeed = 2.f;

	/** ...for this long (s). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float StoppedTime = 0.5f;

	/** Gives up waiting for the balls after this long (s). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float MaxRollingTime = 15.f;

	/** The jack stops at least this far past line E and short of line B' (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float BolimLineMargin = 50.f;

	/** Launch speed of the jack (cm/s); more than enough to reach B', it is braked to its spot. */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float BolimLaunchSpeed = 1000.f;

	/** How long an irregular jack stays on the court before it is removed (s). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float IrregularBolimDelay = 2.f;

	/** Pause between rounds (s). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float RoundOverDelay = 3.f;

	/** Bochas closer than this to the same distance from the jack are tied (cm, Art. 13 c). */
	UPROPERTY(EditDefaultsOnly, Category = "Bocha")
	float TieTolerance = 1.f;

private:
	/** Who is closest to the jack right now. */
	struct FStanding
	{
		/** Closest distance of each team's bochas to the jack; infinite without bochas. */
		float Best[NumTeams];
		/** INDEX_NONE without bochas on the court or on a tie. */
		int32 Holder = INDEX_NONE;
		bool bTie = false;
	};

	void StartRound();
	void EndRound(const FStanding& Standing);
	void ConfirmVictory();
	void EndMinigame();

	/** Announces the result on every machine (Game Over). */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGameOver(int32 WinnerTeam, int32 RedScore, int32 BlueScore);
	FStanding GetStanding() const;
	/** Distance on the floor from a ball to the jack. */
	float DistanceToBolim(const ABochaBall* Ball) const;
	/** Team that owns a bocha (blue = team 1). */
	static int32 GetBochaTeam(const ABochaBall* Ball);
	void CheckBallsStopped();
	void OnBallsStopped();
	void OnBolimStopped();
	void RemoveIrregularBolim();
	/** Why the jack is irregular (Art. 9 h), or empty when it is valid. */
	FText GetBolimProblem() const;
	void RemoveAnnulledBochas();
	void SetState(EBochaPhase NewPhase, int32 NewThrower);
	void DrawCourt() const;

	bool IsOnCourt(const ABochaBall* Ball) const;
	/** Art. 9 i: the jack stays in play while on the court and fully past line E. */
	bool IsBolimStillInPlay() const;

	UFUNCTION()
	void OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& Hit);

	/** Shows a rules message on every screen. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastAnnounce(const FText& Message);

	UFUNCTION()
	void OnRep_State();

	UPROPERTY(Replicated)
	FVector CourtOrigin = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	EBochaPhase Phase = EBochaPhase::WaitingForPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	int32 CurrentThrower = INDEX_NONE;

	/** Index = team. Both entries are the same player when playing alone. */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	TArray<TObjectPtr<APlayerState>> Players;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	TArray<int32> BochasLeft;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	TArray<int32> Score;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	int32 PointHolder = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	int32 Winner = INDEX_NONE;

	/** The ball of the throw being watched; replicated so every machine can follow it. */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	TObjectPtr<ABochaBall> ThrownBall;

	// Server only.
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABochaBall>> BallsInPlay;

	UPROPERTY(Transient)
	TObjectPtr<ABochaBall> Bolim;


	/** The thrown ball touched another ball ("fez jogo"). */
	bool bThrownBallMadePlay = false;
	/** The thrown bocha hit the back board before making play (Art. 10 g, 13 e). */
	bool bThrownBallAnnulled = false;

	/** Team that throws the jack this round, and the first bocha. */
	int32 BolimThrower = 0;
	bool bWaitingForBolim = false;
	float RollingTime = 0.f;
	float StillTime = 0.f;
	FTimerHandle RollingTimer;
	FTimerHandle RoundOverTimer;
	FTimerHandle IrregularBolimTimer;
	FTimerHandle VictoryTimer;
	FTimerHandle EndMinigameTimer;
};
