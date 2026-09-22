// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeashPointTypes.h"
#include "LeashPointComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLeashGaveUpSignature, ELeashGiveUpReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLeashAcquiredSignature, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLeashReturnedHomeSignature);

/**
 * ULeashPointComponent
 *
 * Put it on an enemy. Tell it where home is, tell it when the target is visible, and let it decide
 * when the fight is over.
 *
 * It moves nothing. Bind OnGaveUp and send your own pawn home with whatever navigation the project
 * already uses; bind OnReturnedHome to put it back on its patrol.
 */
UCLASS(ClassGroup = (LeashPoint), meta = (BlueprintSpawnableComponent))
class LEASHPOINT_API ULeashPointComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeashPointComponent();

	/** Where this enemy belongs. Defaults to wherever it was standing at BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint")
	void SetHome(FVector Location);

	UFUNCTION(BlueprintPure, Category = "LeashPoint")
	FVector GetHome() const { return State.Home; }

	/**
	 * Start a chase, if the rules allow it. Returns whether it did.
	 *
	 * Call it from your perception handler; the component answers the three separate questions that
	 * make up "may I" - range, the re-acquire delay, and whether the chase is winnable at all.
	 */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint")
	bool TryAcquire(AActor* Target);

	/** The target is visible right now. Resets the lost-sight timer. */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint")
	void NotifyTargetSeen();

	/** Abandon the chase by hand - a scripted retreat, a phase change, a death. */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint")
	void GiveUp(ELeashGiveUpReason Reason = ELeashGiveUpReason::GroupGaveUp);

	/** Report arrival yourself when your movement code gets there, or let the component notice. */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint")
	void NotifyArrivedHome();

	UFUNCTION(BlueprintPure, Category = "LeashPoint")
	bool IsEngaged() const { return State.bEngaged; }

	UFUNCTION(BlueprintPure, Category = "LeashPoint")
	bool IsReturning() const { return State.bReturning; }

	UFUNCTION(BlueprintPure, Category = "LeashPoint")
	AActor* GetTarget() const { return Target; }

	UFUNCTION(BlueprintPure, Category = "LeashPoint")
	const FLeashState& GetState() const { return State; }

	/** The rules in force, with the hysteresis already enforced. */
	UFUNCTION(BlueprintPure, Category = "LeashPoint")
	FLeashRules GetRules() const;

	/** How far the enemy currently is from home, flat. For a debug readout. */
	UFUNCTION(BlueprintPure, Category = "LeashPoint")
	float GetDistanceFromHome() const;

	/**
	 * Everyone in this group gives up together.
	 *
	 * Add each member once; the component collects the reasons every tick and calls GiveUp on all of
	 * them as soon as any one qualifies.
	 */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint|Group")
	void JoinGroup(ULeashPointComponent* Other);

	UFUNCTION(BlueprintCallable, Category = "LeashPoint|Group")
	void LeaveGroup();

	/** Step the clock by hand. Public so a server on a fixed step or a demo can drive it. */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint")
	void AdvanceTime(float DeltaSeconds);

	/** Turn the component's own clock on or off. See AdvanceTime. */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint")
	void SetAutoTick(bool bEnabled) { bAutoTick = bEnabled; }

	/** Use these rules instead of the project settings, from code. */
	UFUNCTION(BlueprintCallable, Category = "LeashPoint")
	void SetRuleOverride(const FLeashRules& Rules);

	UPROPERTY(BlueprintAssignable, Category = "LeashPoint")
	FLeashGaveUpSignature OnGaveUp;

	UPROPERTY(BlueprintAssignable, Category = "LeashPoint")
	FLeashAcquiredSignature OnAcquired;

	UPROPERTY(BlueprintAssignable, Category = "LeashPoint")
	FLeashReturnedHomeSignature OnReturnedHome;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeashPoint")
	bool bOverrideRules = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeashPoint", meta = (EditCondition = "bOverrideRules"))
	FLeashRules RuleOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeashPoint")
	bool bAutoTick = true;

private:
	UPROPERTY()
	FLeashState State;

	UPROPERTY()
	TObjectPtr<AActor> Target = nullptr;

	/** Everyone who leashes together. Weak on purpose: a dead pack member must not keep the pack alive. */
	UPROPERTY()
	TArray<TWeakObjectPtr<ULeashPointComponent>> Group;

	void Evaluate(float DeltaSeconds);
};
