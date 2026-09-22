// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LeashPointTypes.generated.h"

/** Why a chase ended. Handed to your project so the bark and the animation can differ. */
UENUM(BlueprintType)
enum class ELeashGiveUpReason : uint8
{
	/** Still chasing. */
	None,
	/** The enemy got further from its HOME than the leash radius. */
	OutOfRange,
	/** Nobody has seen the target for long enough. */
	LostSight,
	/** Somebody else in the pack gave up and the pack goes together. */
	GroupGaveUp,
};

/** What returning home does to health. */
UENUM(BlueprintType)
enum class ELeashHealMode : uint8
{
	/** Nothing. The enemy walks home hurt, and a second attempt is cheaper than the first. */
	None,
	/** Full health the moment it arrives. The MMO reading: a failed pull costs you nothing and gains you nothing. */
	OnArrival,
	/** Health comes back on the way. Rewards a player who cuts the escape off. */
	OverTime,
};

/**
 * The rules. Passed to the pure functions explicitly rather than read from the settings inside them,
 * so the tests can hand in their own and a project can vary them per enemy.
 */
USTRUCT(BlueprintType)
struct LEASHPOINT_API FLeashRules
{
	GENERATED_BODY()

	/** How close a target has to be before the enemy starts. Measured from the ENEMY. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint", meta = (ClampMin = "0.0"))
	float AggroRadius = 900.0f;

	/**
	 * How far the enemy may get from HOME before it gives up. Measured from HOME.
	 *
	 * Not from the target, and that is the point: measured from the target, an enemy chases forever
	 * as long as the player keeps moving away at the same speed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint", meta = (ClampMin = "0.0"))
	float LeashRadius = 2200.0f;

	/**
	 * The smallest gap NormaliseRules will allow between the two radii.
	 *
	 * Without a gap the enemy gives up and re-acquires on the same spot, and the result is a guard
	 * that vibrates on the boundary. This is hysteresis, and it is not optional.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint", meta = (ClampMin = "0.0"))
	float MinRadiusGap = 300.0f;

	/** Seconds without a sighting before the chase is abandoned. Zero switches the rule off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint", meta = (ClampMin = "0.0"))
	float LoseSightSeconds = 6.0f;

	/**
	 * After arriving home, how long the enemy ignores targets.
	 *
	 * Covers the walk back too - the timer runs from the moment it gave up. Without it, a player who
	 * follows the returning enemy restarts the whole fight from the middle of the room.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint", meta = (ClampMin = "0.0"))
	float ReacquireDelaySeconds = 3.0f;

	/** How close to home counts as home. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint", meta = (ClampMin = "1.0"))
	float ArriveTolerance = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint")
	ELeashHealMode HealMode = ELeashHealMode::OnArrival;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint", meta = (ClampMin = "0.0"))
	float HealPerSecond = 40.0f;

	/**
	 * Refuse to start a chase whose target is already outside the leash radius from home.
	 *
	 * On by default. A chase that has to be abandoned on the second step looks exactly like a bug,
	 * and the enemy is better off never leaving its post.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint")
	bool bRefuseUnwinnableChases = true;
};

/** One enemy's leash state. Plain data: no actor, no world, no clock. */
USTRUCT(BlueprintType)
struct LEASHPOINT_API FLeashState
{
	GENERATED_BODY()

	/** Where this enemy belongs. Set it once; everything is measured from here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint")
	FVector Home = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "LeashPoint")
	bool bEngaged = false;

	/** True from giving up until arriving home. */
	UPROPERTY(BlueprintReadOnly, Category = "LeashPoint")
	bool bReturning = false;

	/** Reset to zero by NotifyTargetSeen. Grows whenever nobody reports a sighting. */
	UPROPERTY(BlueprintReadOnly, Category = "LeashPoint")
	float SecondsSinceSeen = 0.0f;

	/** Since the chase was abandoned. Drives the re-acquire delay, and it runs during the walk home. */
	UPROPERTY(BlueprintReadOnly, Category = "LeashPoint")
	float SecondsSinceGaveUp = 1000.0f;

	UPROPERTY(BlueprintReadOnly, Category = "LeashPoint")
	ELeashGiveUpReason LastReason = ELeashGiveUpReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "LeashPoint")
	int32 GiveUpCount = 0;
};
