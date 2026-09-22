// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "LeashPointStatics.h"

FLeashRules ULeashPointStatics::NormaliseRules(const FLeashRules& Rules)
{
	FLeashRules Out = Rules;
	Out.AggroRadius = FMath::Max(0.0f, Out.AggroRadius);
	Out.MinRadiusGap = FMath::Max(0.0f, Out.MinRadiusGap);
	Out.LeashRadius = FMath::Max(Out.LeashRadius, Out.AggroRadius + Out.MinRadiusGap);
	Out.LoseSightSeconds = FMath::Max(0.0f, Out.LoseSightSeconds);
	Out.ReacquireDelaySeconds = FMath::Max(0.0f, Out.ReacquireDelaySeconds);
	Out.ArriveTolerance = FMath::Max(1.0f, Out.ArriveTolerance);
	Out.HealPerSecond = FMath::Max(0.0f, Out.HealPerSecond);
	return Out;
}

float ULeashPointStatics::DistanceFromHome(const FVector SelfLocation, const FLeashState& State)
{
	// Flat on purpose. A guard standing on a two-metre ledge above its post has not wandered off,
	// and counting the height would make every enemy on a staircase leash early.
	return FVector::Dist2D(SelfLocation, State.Home);
}

ELeashGiveUpReason ULeashPointStatics::EvaluateGiveUp(const FLeashState& State,
	const FVector SelfLocation, const FLeashRules& Rules)
{
	if (!State.bEngaged)
	{
		return ELeashGiveUpReason::None;
	}

	const FLeashRules R = NormaliseRules(Rules);

	if (DistanceFromHome(SelfLocation, State) > R.LeashRadius)
	{
		return ELeashGiveUpReason::OutOfRange;
	}

	// Zero means the rule is off. Checked explicitly, because ">= 0" would make every enemy give up
	// on the first frame of the fight.
	if (R.LoseSightSeconds > 0.0f && State.SecondsSinceSeen >= R.LoseSightSeconds)
	{
		return ELeashGiveUpReason::LostSight;
	}

	return ELeashGiveUpReason::None;
}

bool ULeashPointStatics::CanAcquire(const FLeashState& State, const FVector SelfLocation,
	const FVector TargetLocation, const FLeashRules& Rules)
{
	if (State.bEngaged)
	{
		return false;
	}

	const FLeashRules R = NormaliseRules(Rules);

	// The delay runs from the moment the chase was abandoned, so it covers the walk home too. A
	// delay that only started on arrival would let a player re-pull an enemy halfway back.
	if (State.SecondsSinceGaveUp < R.ReacquireDelaySeconds)
	{
		return false;
	}

	if (FVector::Dist2D(SelfLocation, TargetLocation) > R.AggroRadius)
	{
		return false;
	}

	if (R.bRefuseUnwinnableChases && FVector::Dist2D(TargetLocation, State.Home) > R.LeashRadius)
	{
		return false;
	}

	return true;
}

bool ULeashPointStatics::HasArrivedHome(const FVector SelfLocation, const FLeashState& State,
	const FLeashRules& Rules)
{
	return DistanceFromHome(SelfLocation, State) <= FMath::Max(1.0f, Rules.ArriveTolerance);
}

float ULeashPointStatics::HealOnReturn(const float Current, const float Max, const bool bArrived,
	const float DeltaSeconds, const FLeashRules& Rules)
{
	switch (Rules.HealMode)
	{
	case ELeashHealMode::None:
		return Current;

	case ELeashHealMode::OnArrival:
		return bArrived ? Max : Current;

	case ELeashHealMode::OverTime:
		return FMath::Min(Max, Current + FMath::Max(0.0f, Rules.HealPerSecond) * FMath::Max(0.0f, DeltaSeconds));

	default:
		return Current;
	}
}

bool ULeashPointStatics::GroupGivesUp(const TArray<ELeashGiveUpReason>& MemberReasons)
{
	for (const ELeashGiveUpReason Reason : MemberReasons)
	{
		if (Reason != ELeashGiveUpReason::None)
		{
			return true;
		}
	}
	return false;
}

FLeashState ULeashPointStatics::Advance(const FLeashState& State, const float DeltaSeconds)
{
	FLeashState Out = State;
	if (DeltaSeconds <= 0.0f)
	{
		return Out;
	}
	Out.SecondsSinceSeen = State.SecondsSinceSeen + DeltaSeconds;
	Out.SecondsSinceGaveUp = State.SecondsSinceGaveUp + DeltaSeconds;
	return Out;
}
