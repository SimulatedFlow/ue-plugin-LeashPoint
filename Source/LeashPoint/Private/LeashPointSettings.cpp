// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "LeashPointSettings.h"

ULeashPointSettings::ULeashPointSettings()
	: bLogTransitions(false)
{
	// Nine metres to notice, twenty-two to give up. The gap is what stops the boundary flickering,
	// and three metres of it is enough that an enemy walking back cannot re-trigger on its own step.
	Rules.AggroRadius = 900.0f;
	Rules.LeashRadius = 2200.0f;
	Rules.MinRadiusGap = 300.0f;
	Rules.LoseSightSeconds = 6.0f;
	Rules.ReacquireDelaySeconds = 3.0f;
	Rules.ArriveTolerance = 120.0f;
	Rules.HealMode = ELeashHealMode::OnArrival;
	Rules.HealPerSecond = 40.0f;
	Rules.bRefuseUnwinnableChases = true;
}

const ULeashPointSettings* ULeashPointSettings::Get()
{
	const ULeashPointSettings* Settings = GetDefault<ULeashPointSettings>();
	check(Settings);
	return Settings;
}
