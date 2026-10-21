#include "Game/BochaMatch.h"

#include "Actors/BochaBall.h"
#include "Actors/BochaBocha.h"
#include "Actors/BochaBolim.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Game/BochaMinigameManager.h"
#include "Game/BochaReferee.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/BochaPortal.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "BochaMatch"

DEFINE_LOG_CATEGORY_STATIC(LogBochaMatch, Log, All);

namespace
{
	constexpr float RollingCheckInterval = 0.1f;

	int32 OtherTeam(int32 Team)
	{
		return (Team + 1) % ABochaMatch::NumTeams;
	}
}

ABochaMatch::ABochaMatch()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
}

ABochaMatch* ABochaMatch::Find(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ABochaMatch> It(World);
	return It ? *It : nullptr;
}

void ABochaMatch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABochaMatch, CourtOrigin);
	DOREPLIFETIME(ABochaMatch, Phase);
	DOREPLIFETIME(ABochaMatch, CurrentThrower);
	DOREPLIFETIME(ABochaMatch, Players);
	DOREPLIFETIME(ABochaMatch, BochasLeft);
	DOREPLIFETIME(ABochaMatch, Score);
	DOREPLIFETIME(ABochaMatch, PointHolder);
	DOREPLIFETIME(ABochaMatch, Winner);
	DOREPLIFETIME(ABochaMatch, ThrownBall);
}

void ABochaMatch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (HasAuthority())
	{
		if (IsValid(Referee))
		{
			Referee->Destroy();
		}
		for (ABochaBall* Ball : BallsInPlay)
		{
			if (IsValid(Ball))
			{
				Ball->Destroy();
			}
		}
		BallsInPlay.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

FText ABochaMatch::GetTeamName(int32 Team)
{
	return Team == 0 ? LOCTEXT("Red", "Vermelho") : LOCTEXT("Blue", "Azul");
}

void ABochaMatch::BeginPlay()
{
	Super::BeginPlay();

	if (bDrawCourt)
	{
		DrawCourt();
	}

	// The minigame managers that already exist on this machine follow this match from now on.
	for (TActorIterator<ABochaMinigameManager> It(GetWorld()); It; ++It)
	{
		It->SetMatch(this);
	}

	if (!HasAuthority())
	{
		return;
	}

	// The host plays red (team 0), the other player blue (team 1).
	for (APlayerState* PlayerState : GetWorld()->GetGameState()->PlayerArray)
	{
		const APlayerController* Controller = PlayerState->GetPlayerController();
		Players.Insert(PlayerState, Controller && Controller->IsLocalController() ? 0 : Players.Num());
	}
	Players.SetNum(NumTeams);
	if (!Players[1])
	{
		Players[1] = Players[0];
	}

	Score.Init(0, NumTeams);

	// Art. 9 c: a draw decides who throws first.
	BolimThrower = FMath::RandRange(0, NumTeams - 1);
	// Later, so the match has reached the clients when the message goes out.
	FTimerHandle DrawMessageTimer;
	GetWorldTimerManager().SetTimer(DrawMessageTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		MulticastAnnounce(FText::Format(LOCTEXT("Draw", "Sorteio: {0} começa."), GetTeamName(BolimThrower)));
	}), 1.f, false);
	StartRound();
}

bool ABochaMatch::CanThrow(const APlayerState* PlayerState) const
{
	return (Phase == EBochaPhase::ThrowingBolim || Phase == EBochaPhase::ThrowingBochas)
		&& Players.IsValidIndex(CurrentThrower) && Players[CurrentThrower] == PlayerState;
}

bool ABochaMatch::IsBehindThrowLine(const FVector& WorldLocation) const
{
	return ToCourt(WorldLocation).X <= Court.B;
}

int32 ABochaMatch::GetTeamOf(const APlayerState* PlayerState) const
{
	return PlayerState ? Players.IndexOfByKey(PlayerState) : INDEX_NONE;
}

void ABochaMatch::StartRound()
{
	for (ABochaBall* Ball : BallsInPlay)
	{
		if (IsValid(Ball))
		{
			Ball->Destroy();
		}
	}
	BallsInPlay.Reset();
	Bolim = nullptr;
	ThrownBall = nullptr;
	PointHolder = INDEX_NONE;

	BochasLeft.Init(BochasPerPlayer, NumTeams);
	UE_LOG(LogBochaMatch, Display, TEXT("Round starts: team %d throws the jack. Score %d x %d."), BolimThrower, Score[0], Score[1]);
	SetState(EBochaPhase::ThrowingBolim, BolimThrower);
}

void ABochaMatch::RegisterThrow(ABochaBall* Ball)
{
	// By the first throw the court is loaded on the server, so the referee has a floor to stand on.
	if (!Referee)
	{
		SpawnReferee();
	}

	bWaitingForBolim = Phase == EBochaPhase::ThrowingBolim;
	if (bWaitingForBolim)
	{
		Bolim = Ball;
	}
	else
	{
		--BochasLeft[CurrentThrower];
	}
	BallsInPlay.Add(Ball);
	UE_LOG(LogBochaMatch, Display, TEXT("Team %d threw %s from %s."), CurrentThrower,
		bWaitingForBolim ? TEXT("the jack") : TEXT("a bocha"), *ToCourt(Ball->GetActorLocation()).ToCompactString());

	ThrownBall = Ball;
	bThrownBallMadePlay = false;
	bThrownBallAnnulled = false;
	Ball->GetSphere()->SetNotifyRigidBodyCollision(true);
	Ball->GetSphere()->OnComponentHit.AddUniqueDynamic(this, &ABochaMatch::OnBallHit);

	RollingTime = 0.f;
	StillTime = 0.f;
	GetWorldTimerManager().SetTimer(RollingTimer, this, &ABochaMatch::CheckBallsStopped, RollingCheckInterval, true);
	SetState(EBochaPhase::BallsMoving, CurrentThrower);
}

void ABochaMatch::OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!ThrownBall || HitComponent->GetOwner() != ThrownBall)
	{
		return;
	}
	if (OtherActor && OtherActor->IsA<ABochaBall>())
	{
		bThrownBallMadePlay = true;
		return;
	}

	// The back board faces -X; only the bochas are annulled by it.
	const bool bHitBackBoard = Hit.ImpactNormal.X < -0.5 && ToCourt(Hit.ImpactPoint).X >= Court.APrime - 5.f;
	if (bHitBackBoard && !bThrownBallMadePlay && ThrownBall != Bolim)
	{
		bThrownBallAnnulled = true;
	}
}

void ABochaMatch::CheckBallsStopped()
{
	BallsInPlay.RemoveAll([](const ABochaBall* Ball) { return !IsValid(Ball); });

	const bool bAllStill = !BallsInPlay.ContainsByPredicate(
		[this](const ABochaBall* Ball) { return Ball->GetVelocity().Size() >= StoppedSpeed; });
	StillTime = bAllStill ? StillTime + RollingCheckInterval : 0.f;
	RollingTime += RollingCheckInterval;

	if (StillTime >= StoppedTime || RollingTime >= MaxRollingTime)
	{
		GetWorldTimerManager().ClearTimer(RollingTimer);
		OnBallsStopped();
	}
}

void ABochaMatch::OnBallsStopped()
{
	if (bWaitingForBolim)
	{
		OnBolimStopped();
		return;
	}

	RemoveAnnulledBochas();

	// Art. 9 i: the round is replayed from the same end, same team on the jack.
	if (!IsBolimStillInPlay())
	{
		UE_LOG(LogBochaMatch, Display, TEXT("Jack out of play at %s."),
			IsValid(Bolim) ? *ToCourt(Bolim->GetActorLocation()).ToCompactString() : TEXT("(gone)"));
		MulticastAnnounce(LOCTEXT("RoundAnnulled", "O bolim saiu de jogo: mão anulada."));
		StartRound();
		return;
	}

	if (IsValid(Referee))
	{
		StartMeasuring();
		return;
	}
	ResolveThrow();
}

void ABochaMatch::SpawnReferee()
{
	if (!RefereeClass)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector Location = CourtOrigin + FVector(RefereeHome.X, RefereeHome.Y, Court.FloorZ + 100.0);
	// Facing down the court, like the players.
	Referee = GetWorld()->SpawnActor<ABochaReferee>(RefereeClass, Location, FRotator::ZeroRotator, Params);
}

void ABochaMatch::StartMeasuring()
{
	SetState(EBochaPhase::Measuring, CurrentThrower);

	// Walk to just short of the jack, measure there (its "Medir" notify), then announce.
	const FVector Target = Bolim->GetActorLocation() - FVector(RefereeMeasureDistance, 0.0, 0.0);
	Referee->OnArrived.BindWeakLambda(this, [this]()
	{
		Referee->OnArrived.Unbind();
		Referee->Measure();
	});
	Referee->OnMeasured.BindUObject(this, &ABochaMatch::FinishMeasuring);
	Referee->WalkTo(Target);
	GetWorldTimerManager().SetTimer(MeasureTimer, this, &ABochaMatch::FinishMeasuring, MeasureTimeout);
}

void ABochaMatch::FinishMeasuring()
{
	if (Phase != EBochaPhase::Measuring)
	{
		return;
	}
	if (!GetWorldTimerManager().IsTimerActive(MeasureTimer))
	{
		UE_LOG(LogBochaMatch, Warning, TEXT("The referee did not measure in %.0f s; going on."), MeasureTimeout);
	}
	GetWorldTimerManager().ClearTimer(MeasureTimer);

	if (IsValid(Referee))
	{
		Referee->OnArrived.Unbind();
		Referee->OnMeasured.Unbind();
		Referee->WalkTo(CourtOrigin + FVector(RefereeHome.X, RefereeHome.Y, Court.FloorZ));
	}
	ResolveThrow();
}

void ABochaMatch::ResolveThrow()
{
	const FStanding Standing = GetStanding();
	PointHolder = Standing.Holder;
	UE_LOG(LogBochaMatch, Display, TEXT("Balls stopped. Best red %.1f, blue %.1f; holder %d%s; bochas left %d/%d."),
		Standing.Best[0], Standing.Best[1], Standing.Holder, Standing.bTie ? TEXT(" (tie)") : TEXT(""), BochasLeft[0], BochasLeft[1]);

	if (BochasLeft[0] == 0 && BochasLeft[1] == 0)
	{
		EndRound(Standing);
		return;
	}

	// Art. 3 c: the side without the point plays. On a tie the side that caused it plays again
	// (Art. 13 c); with no bocha on the court, the side whose bocha was annulled plays again (Art. 9 d).
	int32 Next = Standing.bTie || Standing.Holder == INDEX_NONE ? CurrentThrower : OtherTeam(Standing.Holder);
	if (BochasLeft[Next] == 0)
	{
		Next = OtherTeam(Next);
	}

	if (Standing.bTie)
	{
		MulticastAnnounce(LOCTEXT("PointTied", "Ponto empatado."));
	}
	else if (Standing.Holder != INDEX_NONE)
	{
		MulticastAnnounce(FText::Format(LOCTEXT("Point", "Ponto: {0}."), GetTeamName(Standing.Holder)));
	}
	SetState(EBochaPhase::ThrowingBochas, Next);
}

void ABochaMatch::EndRound(const FStanding& Standing)
{
	if (Standing.bTie)
	{
		// Art. 13 c: a tie at the end annuls the round; it is replayed with the same jack thrower.
		MulticastAnnounce(LOCTEXT("TieRound", "Empate no fim da mão: mão anulada."));
	}
	else if (Standing.Holder == INDEX_NONE)
	{
		MulticastAnnounce(LOCTEXT("NoPoints", "Nenhuma bocha válida na cancha: mão sem pontos."));
	}
	else
	{
		// Art. 5 b: one point per bocha closer than the opponent's best.
		const int32 RoundWinner = Standing.Holder;
		const float OpponentBest = Standing.Best[OtherTeam(RoundWinner)];
		int32 Points = 0;
		for (const ABochaBall* Ball : BallsInPlay)
		{
			if (Ball != Bolim && GetBochaTeam(Ball) == RoundWinner && DistanceToBolim(Ball) < OpponentBest)
			{
				++Points;
			}
		}
		Score[RoundWinner] += Points;
		// Art. 9 f: whoever scored throws the next jack.
		BolimThrower = RoundWinner;
		MulticastAnnounce(FText::Format(
			LOCTEXT("Scored", "{0} marcou {1} {1}|plural(one=ponto,other=pontos). Placar: Vermelho {2} x {3} Azul."),
			GetTeamName(RoundWinner), Points, Score[0], Score[1]));

		// Art. 5 a: the first to TargetScore wins. Their machine claims the victory (Server RPC),
		// the server confirms it; if the claim never comes, the server ends the game anyway.
		if (Score[RoundWinner] >= TargetScore)
		{
			Winner = RoundWinner;
			SetState(EBochaPhase::RoundOver, INDEX_NONE);
			GetWorldTimerManager().SetTimer(VictoryTimer, this, &ABochaMatch::ConfirmVictory, VictoryClaimTimeout);
			return;
		}
	}

	SetState(EBochaPhase::RoundOver, INDEX_NONE);
	GetWorldTimerManager().SetTimer(RoundOverTimer, this, &ABochaMatch::StartRound, RoundOverDelay);
}

void ABochaMatch::HandleVictoryClaim(const APlayerState* Claimer)
{
	const bool bValid = Winner != INDEX_NONE && IsOnTeam(Claimer, Winner) && Score[Winner] >= TargetScore;
	UE_LOG(LogBochaMatch, Display, TEXT("Victory claimed by %s: %s."), *GetNameSafe(Claimer), bValid ? TEXT("confirmed") : TEXT("rejected"));
	if (bValid)
	{
		ConfirmVictory();
	}
}

void ABochaMatch::ConfirmVictory()
{
	if (Phase == EBochaPhase::GameOver)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(VictoryTimer);
	SetState(EBochaPhase::GameOver, INDEX_NONE);
	MulticastGameOver(Winner, Score[0], Score[1]);
	GetWorldTimerManager().SetTimer(EndMinigameTimer, this, &ABochaMatch::EndMinigame, GameOverDuration);
}

void ABochaMatch::MulticastGameOver_Implementation(int32 WinnerTeam, int32 RedScore, int32 BlueScore)
{
	UE_LOG(LogBochaMatch, Display, TEXT("[%s] Game over: team %d wins %d x %d."),
		HasAuthority() ? TEXT("server") : TEXT("client"), WinnerTeam, RedScore, BlueScore);
	OnGameOver.Broadcast(WinnerTeam, RedScore, BlueScore);
}

void ABochaMatch::EndMinigame()
{
	// Every machine goes back to the lobby; the portal then destroys this match.
	for (TActorIterator<ABochaPortal> It(GetWorld()); It; ++It)
	{
		It->MulticastEndMinigame();
	}
}

ABochaMatch::FStanding ABochaMatch::GetStanding() const
{
	FStanding Standing;
	for (float& Best : Standing.Best)
	{
		Best = TNumericLimits<float>::Max();
	}
	for (const ABochaBall* Ball : BallsInPlay)
	{
		if (Ball != Bolim && IsValid(Ball))
		{
			float& Best = Standing.Best[GetBochaTeam(Ball)];
			Best = FMath::Min(Best, DistanceToBolim(Ball));
		}
	}

	const bool bBothPlayed = Standing.Best[0] < TNumericLimits<float>::Max() && Standing.Best[1] < TNumericLimits<float>::Max();
	if (bBothPlayed && FMath::Abs(Standing.Best[0] - Standing.Best[1]) <= TieTolerance)
	{
		Standing.bTie = true;
	}
	else if (Standing.Best[0] < TNumericLimits<float>::Max() || Standing.Best[1] < TNumericLimits<float>::Max())
	{
		Standing.Holder = Standing.Best[0] <= Standing.Best[1] ? 0 : 1;
	}
	return Standing;
}

float ABochaMatch::DistanceToBolim(const ABochaBall* Ball) const
{
	return FVector::Dist2D(Ball->GetActorLocation(), Bolim->GetActorLocation());
}

int32 ABochaMatch::GetBochaTeam(const ABochaBall* Ball)
{
	const ABochaBocha* Bocha = Cast<ABochaBocha>(Ball);
	return Bocha && Bocha->bBlue ? 1 : 0;
}

void ABochaMatch::OnBolimStopped()
{
	const FText Problem = GetBolimProblem();
	UE_LOG(LogBochaMatch, Display, TEXT("Jack stopped at %s: %s"),
		IsValid(Bolim) ? *ToCourt(Bolim->GetActorLocation()).ToCompactString() : TEXT("(gone)"),
		Problem.IsEmpty() ? TEXT("valid") : *Problem.ToString());

	// Whoever won the right to the jack throws the first bocha, even after an irregular jack (Art. 9 f).
	if (Problem.IsEmpty())
	{
		SetState(EBochaPhase::ThrowingBochas, BolimThrower);
		return;
	}

	// Left where it stopped for a moment, so the players see why it does not count.
	MulticastAnnounce(FText::Format(LOCTEXT("BolimIrregular", "Bolim irregular ({0}): lance de novo."), Problem));
	GetWorldTimerManager().SetTimer(IrregularBolimTimer, this, &ABochaMatch::RemoveIrregularBolim, IrregularBolimDelay);
}

void ABochaMatch::RemoveIrregularBolim()
{
	BallsInPlay.Remove(Bolim);
	if (IsValid(Bolim))
	{
		Bolim->Destroy();
	}
	Bolim = nullptr;

	// House rule (instead of Art. 9 f): the same player throws the jack again.
	SetState(EBochaPhase::ThrowingBolim, CurrentThrower);
}

FText ABochaMatch::GetBolimProblem() const
{
	if (!IsOnCourt(Bolim))
	{
		return LOCTEXT("BolimOffCourt", "saiu da cancha");
	}
	const FVector Location = ToCourt(Bolim->GetActorLocation());
	const float Radius = Bolim->GetRadius();
	if (Location.X - Radius <= Court.E)
	{
		return LOCTEXT("BolimShort", "não passou inteiro a linha vermelha E");
	}
	if (Location.X + Radius >= Court.BPrime)
	{
		return LOCTEXT("BolimLong", "chegou à linha verde B' do fundo");
	}
	if (Location.Y - Radius - Court.SideMin <= Court.SideMargin || Court.SideMax - (Location.Y + Radius) <= Court.SideMargin)
	{
		return LOCTEXT("BolimSide", "parou a 13 cm ou menos da lateral");
	}
	return FText::GetEmpty();
}

void ABochaMatch::RemoveAnnulledBochas()
{
	// Art. 10 g, 11 b, 13 e: bochas off the court, or the thrown one that hit the back board first.
	int32 Removed = 0;
	for (ABochaBall* Ball : TArray<TObjectPtr<ABochaBall>>(BallsInPlay))
	{
		if (Ball == Bolim || (IsOnCourt(Ball) && !(Ball == ThrownBall && bThrownBallAnnulled)))
		{
			continue;
		}
		BallsInPlay.Remove(Ball);
		Ball->Destroy();
		++Removed;
	}
	if (Removed > 0)
	{
		MulticastAnnounce(Removed == 1
			? LOCTEXT("BochaAnnulled", "Bocha anulada.")
			: FText::Format(LOCTEXT("BochasAnnulled", "{0} bochas anuladas."), Removed));
	}
}

bool ABochaMatch::IsOnCourt(const ABochaBall* Ball) const
{
	if (!IsValid(Ball))
	{
		return false;
	}
	const FVector Location = ToCourt(Ball->GetActorLocation());
	return Location.X >= Court.A && Location.X <= Court.APrime
		&& Location.Y >= Court.SideMin && Location.Y <= Court.SideMax
		&& Location.Z >= Court.FloorZ - 20.f;
}

bool ABochaMatch::IsBolimStillInPlay() const
{
	return IsOnCourt(Bolim) && ToCourt(Bolim->GetActorLocation()).X - Bolim->GetRadius() > Court.E;
}

void ABochaMatch::SetState(EBochaPhase NewPhase, int32 NewThrower)
{
	Phase = NewPhase;
	CurrentThrower = NewThrower;
	ForceNetUpdate();
	// OnRep only runs on clients; the listen server host needs the notification too.
	OnStateChanged.Broadcast();
}

void ABochaMatch::OnRep_State()
{
	OnStateChanged.Broadcast();
}

void ABochaMatch::MulticastAnnounce_Implementation(const FText& Message)
{
	UE_LOG(LogBochaMatch, Display, TEXT("[%s] %s"), HasAuthority() ? TEXT("server") : TEXT("client"), *Message.ToString());
	OnAnnounce.Broadcast(Message);
}

FVector ABochaMatch::LaunchBolim(ABochaBall* JackBall, float Strength) const
{
	const double TargetX = FMath::Lerp(Court.E + BolimLineMargin, Court.BPrime - BolimLineMargin, FMath::Clamp(Strength, 0.f, 1.f));
	JackBall->SetStopTarget(CourtOrigin.X + TargetX);
	return FVector(BolimLaunchSpeed, 0.0, 0.0);
}

void ABochaMatch::DrawCourt() const
{
	const float Z = Court.FloorZ + 1.f;
	auto DrawLine = [this, Z](float X, const FColor& Color)
	{
		DrawDebugLine(GetWorld(), CourtOrigin + FVector(X, Court.SideMin, Z), CourtOrigin + FVector(X, Court.SideMax, Z),
			Color, true, -1.f, 0, 2.f);
	};
	// Lines measured from A, mirrored from A' (Figure 1 colors).
	auto DrawPair = [&](float X, const FColor& Color)
	{
		DrawLine(X, Color);
		DrawLine(Court.A + Court.APrime - X, Color);
	};
	DrawPair(Court.A, FColor::White);
	DrawPair(Court.B, FColor::Green);
	DrawPair(Court.C, FColor::Blue);
	DrawPair(Court.D, FColor::Yellow);
	DrawLine(Court.E, FColor::Red);
}

#undef LOCTEXT_NAMESPACE
