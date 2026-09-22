// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LeashPointTypes.h"
#include "LeashPointStatics.generated.h"

/**
 * The rules, on their own.
 *
 * No world, no actor, no navigation, no clock. The component calls exactly these and so do the
 * tests, which is the only way the debug drawing and the behaviour cannot drift apart.
 */
UCLASS()
class LEASHPOINT_API ULeashPointStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Rules with the hysteresis enforced: the leash radius is pushed out to at least
	 * AggroRadius + MinRadiusGap.
	 *
	 * Call it once and keep the result. Everything else in this library assumes it has been called,
	 * because a leash radius inside the aggro radius produces an enemy that gives up and re-acquires
	 * in the same frame, forever.
	 */
	UFUNCTION(BlueprintPure, Category = "LeashPoint|Rules")
	static FLeashRules NormaliseRules(const FLeashRules& Rules);

	/**
	 * Should this enemy give up now?
	 *
	 * Distance is measured from HOME. Measured from the target instead, an enemy chases a retreating
	 * player to the end of the level, because the distance between them never grows.
	 */
	UFUNCTION(BlueprintPure, Category = "LeashPoint|Rules")
	static ELeashGiveUpReason EvaluateGiveUp(const FLeashState& State, FVector SelfLocation,
		const FLeashRules& Rules);

	/**
	 * May this enemy start a chase?
	 *
	 * Three separate refusals, and each of them exists because of a specific way this goes wrong:
	 * the target is out of aggro range; the re-acquire delay after the last chase has not run out;
	 * or the target sits outside the leash radius from home, which would make the chase a step long.
	 */
	UFUNCTION(BlueprintPure, Category = "LeashPoint|Rules")
	static bool CanAcquire(const FLeashState& State, FVector SelfLocation, FVector TargetLocation,
		const FLeashRules& Rules);

	/** True once the enemy is close enough to home to call it arrived. */
	UFUNCTION(BlueprintPure, Category = "LeashPoint|Rules")
	static bool HasArrivedHome(FVector SelfLocation, const FLeashState& State, const FLeashRules& Rules);

	/**
	 * Health after DeltaSeconds of walking home.
	 *
	 * OnArrival returns Max only when bArrived is set - the caller decides when that is, because
	 * "arrived" is a question about the world and this function is not allowed to ask the world
	 * anything.
	 */
	UFUNCTION(BlueprintPure, Category = "LeashPoint|Rules")
	static float HealOnReturn(float Current, float Max, bool bArrived, float DeltaSeconds,
		const FLeashRules& Rules);

	/**
	 * Does the pack give up?
	 *
	 * Any member giving up takes the whole pack with it. Half a pack returning while the other half
	 * keeps fighting is the worst of both: the player is still in a fight, and the fight is no
	 * longer the one the designer built.
	 */
	UFUNCTION(BlueprintPure, Category = "LeashPoint|Rules")
	static bool GroupGivesUp(const TArray<ELeashGiveUpReason>& MemberReasons);

	/** Time passing: the two timers run on. Nothing else changes. */
	UFUNCTION(BlueprintPure, Category = "LeashPoint|Rules")
	static FLeashState Advance(const FLeashState& State, float DeltaSeconds);

	/** Distance from home, flat. Height is deliberately ignored - a guard on a ledge is not far away. */
	UFUNCTION(BlueprintPure, Category = "LeashPoint|Rules")
	static float DistanceFromHome(FVector SelfLocation, const FLeashState& State);
};
