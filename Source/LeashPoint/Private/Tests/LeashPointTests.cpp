// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "LeashPointStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace LeashPointTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::EngineFilter;

	static FLeashRules Rules()
	{
		FLeashRules R;
		R.AggroRadius = 900.0f;
		R.LeashRadius = 2200.0f;
		R.MinRadiusGap = 300.0f;
		R.LoseSightSeconds = 6.0f;
		R.ReacquireDelaySeconds = 3.0f;
		R.ArriveTolerance = 120.0f;
		R.HealMode = ELeashHealMode::OnArrival;
		R.HealPerSecond = 40.0f;
		R.bRefuseUnwinnableChases = true;
		return R;
	}

	static FLeashState Engaged(const FVector Home = FVector::ZeroVector)
	{
		FLeashState S;
		S.Home = Home;
		S.bEngaged = true;
		S.SecondsSinceSeen = 0.0f;
		S.SecondsSinceGaveUp = 1000.0f;
		return S;
	}

	static FVector At(const float X, const float Y = 0.0f)
	{
		return FVector(X, Y, 0.0f);
	}
}

// -------------------------------------------------------------------------------------------------
// The rule the whole plugin is for.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeashPointMeasuredFromHome,
	"LeashPoint.GiveUp.MeasuredFromHomeNotFromTheTarget", LeashPointTests::TestFlags)

bool FLeashPointMeasuredFromHome::RunTest(const FString&)
{
	using namespace LeashPointTests;
	const FLeashRules R = Rules();
	const FLeashState S = Engaged(At(0.0f));

	// Just inside the leash radius: still chasing, however far the player has run.
	TestTrue(TEXT("inside the leash radius the chase continues"),
		ULeashPointStatics::EvaluateGiveUp(S, At(2100.0f), R) == ELeashGiveUpReason::None);

	// Past it: over, and for the reason that says so.
	TestTrue(TEXT("past the leash radius the chase ends"),
		ULeashPointStatics::EvaluateGiveUp(S, At(2300.0f), R) == ELeashGiveUpReason::OutOfRange);

	// THE POINT. Measured between enemy and target, a player who keeps running at the enemy's speed
	// holds the distance constant and is never dropped. Measured from home, the enemy's own
	// position decides - and it is the only measurement the enemy can be sure about.
	//
	// Here the enemy is standing ON its post and the target is a mile away. Distance between them
	// is enormous; distance from home is zero. The chase does not end for range.
	TestTrue(TEXT("standing at home is never out of range, whatever the target does"),
		ULeashPointStatics::EvaluateGiveUp(S, At(0.0f), R) == ELeashGiveUpReason::None);

	// Height is not distance. A guard on a ledge above its post has not wandered off.
	FLeashState High = S;
	TestTrue(TEXT("two metres up is still home"),
		ULeashPointStatics::EvaluateGiveUp(High, FVector(0.0f, 0.0f, 5000.0f), R) == ELeashGiveUpReason::None);

	// A disengaged enemy is never "giving up".
	FLeashState Idle = S;
	Idle.bEngaged = false;
	TestTrue(TEXT("an enemy that is not fighting has nothing to give up"),
		ULeashPointStatics::EvaluateGiveUp(Idle, At(9999.0f), R) == ELeashGiveUpReason::None);

	return true;
}

// -------------------------------------------------------------------------------------------------
// The boundary must not flicker.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeashPointHysteresisIsEnforced,
	"LeashPoint.Rules.HysteresisIsEnforcedNotAssumed", LeashPointTests::TestFlags)

bool FLeashPointHysteresisIsEnforced::RunTest(const FString&)
{
	using namespace LeashPointTests;

	// Somebody writes a leash radius smaller than the aggro radius. On the boundary the enemy would
	// give up (out of leash range) and re-acquire (inside aggro range) in the same frame, forever.
	FLeashRules Silly = Rules();
	Silly.AggroRadius = 1200.0f;
	Silly.LeashRadius = 400.0f;

	const FLeashRules Fixed = ULeashPointStatics::NormaliseRules(Silly);
	TestNearlyEqual(TEXT("the leash radius is pushed out past the aggro radius"),
		Fixed.LeashRadius, 1500.0f, 0.001f);   // 1200 + 300
	TestTrue(TEXT("and the gap is at least the minimum"),
		Fixed.LeashRadius - Fixed.AggroRadius >= Fixed.MinRadiusGap - KINDA_SMALL_NUMBER);

	// A sane pair is left alone.
	const FLeashRules Sane = ULeashPointStatics::NormaliseRules(Rules());
	TestNearlyEqual(TEXT("a sane leash radius is not moved"), Sane.LeashRadius, 2200.0f, 0.001f);

	// And the consequence: at the exact spot where it gave up, the enemy cannot start again -
	// because the target would have to be within the aggro radius of the enemy, and the enemy is
	// standing a whole gap beyond that.
	FLeashState Gave = Engaged(At(0.0f));
	Gave.bEngaged = false;
	Gave.SecondsSinceGaveUp = 1000.0f;   // the delay is not what is being tested here
	const FVector Boundary = At(Fixed.LeashRadius + 1.0f);
	TestFalse(TEXT("standing just past the leash radius, a target at home is out of aggro range"),
		ULeashPointStatics::CanAcquire(Gave, Boundary, At(0.0f), Silly));

	return true;
}

// -------------------------------------------------------------------------------------------------
// Sight.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeashPointSightTimerResets,
	"LeashPoint.GiveUp.SightTimerResetsAndDoesNotAccumulate", LeashPointTests::TestFlags)

bool FLeashPointSightTimerResets::RunTest(const FString&)
{
	using namespace LeashPointTests;
	const FLeashRules R = Rules();   // six seconds

	FLeashState S = Engaged();

	// Five seconds of nothing: still chasing.
	S = ULeashPointStatics::Advance(S, 5.0f);
	TestTrue(TEXT("five seconds is not yet six"),
		ULeashPointStatics::EvaluateGiveUp(S, At(100.0f), R) == ELeashGiveUpReason::None);

	// A glimpse. The timer goes back to zero - it does NOT keep a running total of blind seconds.
	// Accumulating across glimpses gives an enemy that drops a target it can plainly see, purely
	// because the fight has been going on for a while.
	S.SecondsSinceSeen = 0.0f;
	S = ULeashPointStatics::Advance(S, 5.0f);
	TestTrue(TEXT("after a sighting, five more seconds is still not six"),
		ULeashPointStatics::EvaluateGiveUp(S, At(100.0f), R) == ELeashGiveUpReason::None);

	// Now let it run out.
	S = ULeashPointStatics::Advance(S, 1.5f);
	TestTrue(TEXT("six and a half seconds without a sighting ends it"),
		ULeashPointStatics::EvaluateGiveUp(S, At(100.0f), R) == ELeashGiveUpReason::LostSight);

	// Zero switches the rule off rather than firing on the first frame.
	FLeashRules NoSightRule = R;
	NoSightRule.LoseSightSeconds = 0.0f;
	FLeashState Blind = Engaged();
	Blind.SecondsSinceSeen = 9999.0f;
	TestTrue(TEXT("zero means the rule is off, not instant"),
		ULeashPointStatics::EvaluateGiveUp(Blind, At(100.0f), NoSightRule) == ELeashGiveUpReason::None);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Coming back.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeashPointReacquireDelayCoversTheWalkHome,
	"LeashPoint.Acquire.DelayCoversTheWalkHome", LeashPointTests::TestFlags)

bool FLeashPointReacquireDelayCoversTheWalkHome::RunTest(const FString&)
{
	using namespace LeashPointTests;
	const FLeashRules R = Rules();   // three seconds

	FLeashState S = Engaged();
	S.bEngaged = false;
	S.bReturning = true;
	S.SecondsSinceGaveUp = 0.0f;

	// The player walks straight back into range while the enemy is still walking home.
	TestFalse(TEXT("immediately after giving up, nothing re-acquires"),
		ULeashPointStatics::CanAcquire(S, At(300.0f), At(500.0f), R));

	S = ULeashPointStatics::Advance(S, 2.0f);
	TestFalse(TEXT("two seconds in, still nothing"),
		ULeashPointStatics::CanAcquire(S, At(300.0f), At(500.0f), R));

	S = ULeashPointStatics::Advance(S, 1.5f);
	TestTrue(TEXT("past the delay it starts again"),
		ULeashPointStatics::CanAcquire(S, At(300.0f), At(500.0f), R));

	// An enemy already fighting does not "acquire" again.
	FLeashState Busy = Engaged();
	TestFalse(TEXT("an engaged enemy does not acquire"),
		ULeashPointStatics::CanAcquire(Busy, At(0.0f), At(100.0f), R));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeashPointRefusesUnwinnableChases,
	"LeashPoint.Acquire.RefusesAChaseItWouldHaveToAbandon", LeashPointTests::TestFlags)

bool FLeashPointRefusesUnwinnableChases::RunTest(const FString&)
{
	using namespace LeashPointTests;
	const FLeashRules R = Rules();   // aggro 900, leash 2200

	// The enemy is out at the edge of its territory. The target is inside aggro range of the enemy
	// but well outside the leash radius from home. Starting here means giving up one step later,
	// which on screen is indistinguishable from a bug.
	FLeashState S;
	S.Home = At(0.0f);
	S.bEngaged = false;
	S.SecondsSinceGaveUp = 1000.0f;

	TestFalse(TEXT("a target beyond the leash radius is not worth starting for"),
		ULeashPointStatics::CanAcquire(S, At(2100.0f), At(2600.0f), R));

	// The same target, with the refusal switched off, is taken.
	FLeashRules Eager = R;
	Eager.bRefuseUnwinnableChases = false;
	TestTrue(TEXT("with the rule off, the enemy starts anyway"),
		ULeashPointStatics::CanAcquire(S, At(2100.0f), At(2600.0f), Eager));

	// And a target that is inside both is taken either way.
	TestTrue(TEXT("a reachable target is taken"),
		ULeashPointStatics::CanAcquire(S, At(2100.0f), At(1900.0f), R));

	return true;
}

// -------------------------------------------------------------------------------------------------
// The pack, and coming home.
// -------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeashPointGroupGivesUpTogether,
	"LeashPoint.Group.AnyMemberTakesThePackWithIt", LeashPointTests::TestFlags)

bool FLeashPointGroupGivesUpTogether::RunTest(const FString&)
{
	TArray<ELeashGiveUpReason> AllFighting;
	AllFighting.Add(ELeashGiveUpReason::None);
	AllFighting.Add(ELeashGiveUpReason::None);
	AllFighting.Add(ELeashGiveUpReason::None);
	TestFalse(TEXT("a pack that is all still fighting keeps fighting"),
		ULeashPointStatics::GroupGivesUp(AllFighting));

	TArray<ELeashGiveUpReason> OneOut = AllFighting;
	OneOut[1] = ELeashGiveUpReason::OutOfRange;
	TestTrue(TEXT("one member out of range takes the pack with it"),
		ULeashPointStatics::GroupGivesUp(OneOut));

	// Half a pack returning while the other half keeps swinging is the worst outcome: the player is
	// still in a fight, and it is no longer the fight that was designed.
	TArray<ELeashGiveUpReason> OneBlind = AllFighting;
	OneBlind[2] = ELeashGiveUpReason::LostSight;
	TestTrue(TEXT("and so does one member losing sight"),
		ULeashPointStatics::GroupGivesUp(OneBlind));

	TestFalse(TEXT("an empty pack gives up nothing"),
		ULeashPointStatics::GroupGivesUp(TArray<ELeashGiveUpReason>()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLeashPointHealingIsASetting,
	"LeashPoint.Return.HealingIsASettingNotAnAssumption", LeashPointTests::TestFlags)

bool FLeashPointHealingIsASetting::RunTest(const FString&)
{
	using namespace LeashPointTests;
	FLeashRules R = Rules();

	R.HealMode = ELeashHealMode::None;
	TestNearlyEqual(TEXT("None leaves it hurt, even on arrival"),
		ULeashPointStatics::HealOnReturn(30.0f, 100.0f, true, 1.0f, R), 30.0f, 0.001f);

	R.HealMode = ELeashHealMode::OnArrival;
	TestNearlyEqual(TEXT("OnArrival heals nothing on the way"),
		ULeashPointStatics::HealOnReturn(30.0f, 100.0f, false, 1.0f, R), 30.0f, 0.001f);
	TestNearlyEqual(TEXT("and everything when it gets there"),
		ULeashPointStatics::HealOnReturn(30.0f, 100.0f, true, 0.0f, R), 100.0f, 0.001f);

	R.HealMode = ELeashHealMode::OverTime;
	TestNearlyEqual(TEXT("OverTime heals per second"),
		ULeashPointStatics::HealOnReturn(30.0f, 100.0f, false, 1.0f, R), 70.0f, 0.001f);
	TestNearlyEqual(TEXT("and never past the maximum"),
		ULeashPointStatics::HealOnReturn(90.0f, 100.0f, false, 5.0f, R), 100.0f, 0.001f);

	// Arriving home is a question about the world, so the caller answers it.
	FLeashState S = Engaged(At(0.0f));
	TestTrue(TEXT("inside the tolerance counts as home"),
		ULeashPointStatics::HasArrivedHome(At(100.0f), S, R));
	TestFalse(TEXT("outside it does not"),
		ULeashPointStatics::HasArrivedHome(At(200.0f), S, R));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
