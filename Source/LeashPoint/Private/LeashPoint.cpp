// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "LeashPoint.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "LeashPointComponent.h"
#include "LeashPointLog.h"
#include "LeashPointStatics.h"

DEFINE_LOG_CATEGORY(LogLeashPoint);

#define LOCTEXT_NAMESPACE "FLeashPointModule"

namespace
{
	UWorld* LeashPointConsoleWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void LeashPointDumpCommand()
	{
		UWorld* World = LeashPointConsoleWorld();
		if (!World)
		{
			UE_LOG(LogLeashPoint, Warning, TEXT("LeashPoint.Dump: no running world."));
			return;
		}

		int32 Found = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const ULeashPointComponent* Leash = It->FindComponentByClass<ULeashPointComponent>();
			if (!Leash)
			{
				continue;
			}
			++Found;
			const FLeashState& S = Leash->GetState();
			const FLeashRules R = Leash->GetRules();
			UE_LOG(LogLeashPoint, Display,
				TEXT("%-22s %-10s from home %7.0f / %7.0f  unseen %5.1fs  since give-up %6.1fs  gave up %d  last %s"),
				*It->GetName(),
				S.bEngaged ? TEXT("CHASING") : (S.bReturning ? TEXT("returning") : TEXT("idle")),
				Leash->GetDistanceFromHome(), R.LeashRadius,
				S.SecondsSinceSeen, S.SecondsSinceGaveUp, S.GiveUpCount,
				*UEnum::GetValueAsString(S.LastReason));
		}

		if (Found == 0)
		{
			UE_LOG(LogLeashPoint, Display, TEXT("LeashPoint.Dump: nothing in this level has a leash component."));
		}
	}

	void LeashPointReleaseCommand()
	{
		UWorld* World = LeashPointConsoleWorld();
		if (!World)
		{
			UE_LOG(LogLeashPoint, Warning, TEXT("LeashPoint.Release: no running world."));
			return;
		}

		int32 Count = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (ULeashPointComponent* Leash = It->FindComponentByClass<ULeashPointComponent>())
			{
				if (Leash->IsEngaged())
				{
					Leash->GiveUp(ELeashGiveUpReason::GroupGaveUp);
					++Count;
				}
			}
		}
		UE_LOG(LogLeashPoint, Display, TEXT("LeashPoint.Release: sent %d home."), Count);
	}

	FAutoConsoleCommand GLeashPointDump(
		TEXT("LeashPoint.Dump"),
		TEXT("Every enemy with a leash component: state, distance from home and the timers."),
		FConsoleCommandDelegate::CreateStatic(&LeashPointDumpCommand));

	FAutoConsoleCommand GLeashPointRelease(
		TEXT("LeashPoint.Release"),
		TEXT("Send every engaged enemy home. The way out when a fight will not end."),
		FConsoleCommandDelegate::CreateStatic(&LeashPointReleaseCommand));
}

void FLeashPointModule::StartupModule()
{
}

void FLeashPointModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLeashPointModule, LeashPoint)
