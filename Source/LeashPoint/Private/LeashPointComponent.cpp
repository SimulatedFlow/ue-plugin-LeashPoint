// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "LeashPointComponent.h"

#include "GameFramework/Actor.h"
#include "LeashPointLog.h"
#include "LeashPointSettings.h"
#include "LeashPointStatics.h"

ULeashPointComponent::ULeashPointComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void ULeashPointComponent::BeginPlay()
{
	Super::BeginPlay();

	if (State.Home.IsNearlyZero() && GetOwner())
	{
		// Wherever it was placed is where it belongs. A designer who wants something else calls
		// SetHome; a designer who wants nothing gets the sensible thing without thinking about it.
		State.Home = GetOwner()->GetActorLocation();
	}
	State.SecondsSinceGaveUp = 1000.0f;
}

FLeashRules ULeashPointComponent::GetRules() const
{
	return ULeashPointStatics::NormaliseRules(
		bOverrideRules ? RuleOverride : ULeashPointSettings::Get()->Rules);
}

void ULeashPointComponent::SetHome(const FVector Location)
{
	State.Home = Location;
}

void ULeashPointComponent::SetRuleOverride(const FLeashRules& Rules)
{
	bOverrideRules = true;
	RuleOverride = Rules;
}

float ULeashPointComponent::GetDistanceFromHome() const
{
	const AActor* Owner = GetOwner();
	return Owner ? ULeashPointStatics::DistanceFromHome(Owner->GetActorLocation(), State) : 0.0f;
}

bool ULeashPointComponent::TryAcquire(AActor* InTarget)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !InTarget)
	{
		return false;
	}

	if (!ULeashPointStatics::CanAcquire(State, Owner->GetActorLocation(),
		InTarget->GetActorLocation(), GetRules()))
	{
		return false;
	}

	Target = InTarget;
	State.bEngaged = true;
	State.bReturning = false;
	State.SecondsSinceSeen = 0.0f;
	State.LastReason = ELeashGiveUpReason::None;

	if (ULeashPointSettings::Get()->bLogTransitions)
	{
		UE_LOG(LogLeashPoint, Display, TEXT("[%s] engaged %s"),
			*GetNameSafe(Owner), *GetNameSafe(InTarget));
	}

	OnAcquired.Broadcast(InTarget);
	return true;
}

void ULeashPointComponent::NotifyTargetSeen()
{
	State.SecondsSinceSeen = 0.0f;
}

void ULeashPointComponent::GiveUp(const ELeashGiveUpReason Reason)
{
	if (!State.bEngaged)
	{
		return;
	}

	State.bEngaged = false;
	State.bReturning = true;
	State.SecondsSinceGaveUp = 0.0f;
	State.LastReason = Reason;
	State.GiveUpCount += 1;
	Target = nullptr;

	if (ULeashPointSettings::Get()->bLogTransitions)
	{
		UE_LOG(LogLeashPoint, Display, TEXT("[%s] gave up: %s"),
			*GetNameSafe(GetOwner()), *UEnum::GetValueAsString(Reason));
	}

	OnGaveUp.Broadcast(Reason);

	// The pack goes together. Guarded by bEngaged above, so this does not recurse forever.
	for (const TWeakObjectPtr<ULeashPointComponent>& Member : Group)
	{
		if (ULeashPointComponent* M = Member.Get())
		{
			M->GiveUp(ELeashGiveUpReason::GroupGaveUp);
		}
	}
}

void ULeashPointComponent::NotifyArrivedHome()
{
	if (!State.bReturning)
	{
		return;
	}
	State.bReturning = false;
	OnReturnedHome.Broadcast();
}

void ULeashPointComponent::JoinGroup(ULeashPointComponent* Other)
{
	if (!Other || Other == this)
	{
		return;
	}
	Group.AddUnique(Other);
	Other->Group.AddUnique(this);
}

void ULeashPointComponent::LeaveGroup()
{
	for (const TWeakObjectPtr<ULeashPointComponent>& Member : Group)
	{
		if (ULeashPointComponent* M = Member.Get())
		{
			M->Group.Remove(this);
		}
	}
	Group.Reset();
}

void ULeashPointComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoTick)
	{
		AdvanceTime(DeltaTime);
	}
}

void ULeashPointComponent::AdvanceTime(const float DeltaSeconds)
{
	State = ULeashPointStatics::Advance(State, DeltaSeconds);
	Evaluate(DeltaSeconds);
}

void ULeashPointComponent::Evaluate(const float DeltaSeconds)
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FLeashRules R = GetRules();

	if (State.bEngaged)
	{
		const ELeashGiveUpReason Reason =
			ULeashPointStatics::EvaluateGiveUp(State, Owner->GetActorLocation(), R);
		if (Reason != ELeashGiveUpReason::None)
		{
			GiveUp(Reason);
		}
		return;
	}

	if (State.bReturning && ULeashPointStatics::HasArrivedHome(Owner->GetActorLocation(), State, R))
	{
		NotifyArrivedHome();
	}
}
