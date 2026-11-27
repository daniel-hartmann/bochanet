#include "Game/BochaGameState.h"

#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogBochaGameState, Log, All);

void ABochaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABochaGameState, MatchPhase);
	DOREPLIFETIME(ABochaGameState, ConnectedPlayers);
	DOREPLIFETIME(ABochaGameState, RequiredPlayers);
	DOREPLIFETIME(ABochaGameState, MatchStartTime);
	DOREPLIFETIME(ABochaGameState, MatchEndTime);
	DOREPLIFETIME(ABochaGameState, WinnerTeam);
	DOREPLIFETIME(ABochaGameState, TurnPhase);
	DOREPLIFETIME(ABochaGameState, CurrentTeam);
}

float ABochaGameState::GetMatchTime() const
{
	switch (MatchPhase)
	{
	case EBochaMatchPhase::InProgress:
		return FMath::Max(0.f, static_cast<float>(GetServerWorldTimeSeconds() - MatchStartTime));
	case EBochaMatchPhase::Finished:
		return FMath::Max(0.f, static_cast<float>(MatchEndTime - MatchStartTime));
	default:
		return 0.f;
	}
}

void ABochaGameState::StartMatch()
{
	if (!HasAuthority())
	{
		return;
	}
	MatchStartTime = GetServerWorldTimeSeconds();
	MatchEndTime = 0.0;
	WinnerTeam = INDEX_NONE;
	SetMatchPhase(EBochaMatchPhase::InProgress);
}

void ABochaGameState::FinishMatch(int32 Winner)
{
	if (!HasAuthority() || MatchPhase != EBochaMatchPhase::InProgress)
	{
		return;
	}
	MatchEndTime = GetServerWorldTimeSeconds();
	WinnerTeam = Winner;
	SetMatchPhase(EBochaMatchPhase::Finished);
}

void ABochaGameState::ResetMatch()
{
	if (!HasAuthority())
	{
		return;
	}
	TurnPhase = EBochaPhase::WaitingForPlayers;
	CurrentTeam = INDEX_NONE;
	SetMatchPhase(EBochaMatchPhase::WaitingForPlayers);
}

void ABochaGameState::SetTurn(EBochaPhase NewPhase, int32 NewTeam)
{
	if (!HasAuthority())
	{
		return;
	}
	TurnPhase = NewPhase;
	CurrentTeam = NewTeam;
	ForceNetUpdate();
	// OnRep only runs on clients; ABochaMatch notifies the listen server host itself.
}

void ABochaGameState::SetPlayerCount(int32 Connected, int32 Required)
{
	if (!HasAuthority())
	{
		return;
	}
	ConnectedPlayers = Connected;
	RequiredPlayers = Required;
	ForceNetUpdate();
	OnRep_MatchState();
}

void ABochaGameState::SetMatchPhase(EBochaMatchPhase NewPhase)
{
	MatchPhase = NewPhase;
	ForceNetUpdate();
	// OnRep only runs on clients; the listen server host needs the notification too.
	OnRep_MatchState();
}

void ABochaGameState::OnRep_MatchState()
{
	UE_LOG(LogBochaGameState, Display, TEXT("[%s] Match %s, players %d/%d, time %.0f s."),
		HasAuthority() ? TEXT("server") : TEXT("client"),
		*UEnum::GetValueAsString(MatchPhase), ConnectedPlayers, RequiredPlayers, GetMatchTime());
	OnMatchStateChanged.Broadcast();
}

void ABochaGameState::OnRep_Turn()
{
	// The HUD follows the match; the turn may replicate before or after the match itself.
	if (ABochaMatch* Match = ABochaMatch::Find(this))
	{
		Match->NotifyStateChanged();
	}
}
