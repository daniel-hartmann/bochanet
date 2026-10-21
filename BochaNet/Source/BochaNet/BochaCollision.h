#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

/** Collision channels and presets of the bocha minigame (defined in Config/DefaultEngine.ini). */
namespace BochaCollision
{
	/** Object channel of the balls. */
	inline constexpr ECollisionChannel BallChannel = ECC_GameTraceChannel1;

	/** Object channel of the referee. */
	inline constexpr ECollisionChannel RefereeChannel = ECC_GameTraceChannel2;

	/** Trace channel for "is this ball resting on something?". */
	inline constexpr ECollisionChannel GroundTrace = ECC_GameTraceChannel3;

	/** Preset of a ball in play. */
	inline const FName BallProfile(TEXT("BochaBall"));

	/** Preset of the referee's capsule. */
	inline const FName RefereeProfile(TEXT("BochaReferee"));
}
