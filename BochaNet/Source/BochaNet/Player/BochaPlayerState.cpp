#include "Player/BochaPlayerState.h"

#include "Game/BochaMatch.h"
#include "Net/UnrealNetwork.h"

void ABochaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABochaPlayerState, Team);
	DOREPLIFETIME(ABochaPlayerState, Points);
	DOREPLIFETIME(ABochaPlayerState, BochasLeft);
}

void ABochaPlayerState::StartMatch(int32 NewTeam)
{
	Team = NewTeam;
	Points = 0;
	BochasLeft = 0;
	Changed();
}

void ABochaPlayerState::ClearMatch()
{
	Team = INDEX_NONE;
	Points = 0;
	BochasLeft = 0;
	ForceNetUpdate();
}

void ABochaPlayerState::AddPoints(int32 Amount)
{
	Points += Amount;
	Changed();
}

void ABochaPlayerState::SetBochasLeft(int32 Count)
{
	BochasLeft = FMath::Max(0, Count);
	Changed();
}

void ABochaPlayerState::Changed()
{
	ForceNetUpdate();
	// OnRep only runs on clients; the listen server host needs the notification too.
	OnRep_BochaState();
}

void ABochaPlayerState::OnRep_BochaState()
{
	// The HUD follows the match; a player state may replicate before or after the match itself.
	if (ABochaMatch* Match = ABochaMatch::Find(this))
	{
		Match->NotifyStateChanged();
	}
}
