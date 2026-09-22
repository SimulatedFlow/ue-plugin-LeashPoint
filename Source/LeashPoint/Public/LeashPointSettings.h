// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LeashPointTypes.h"
#include "LeashPointSettings.generated.h"

/** Project Settings > Plugins > LeashPoint. The defaults every new component starts from. */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "LeashPoint"))
class LEASHPOINT_API ULeashPointSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULeashPointSettings();

	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const ULeashPointSettings* Get();

	UPROPERTY(config, EditAnywhere, Category = "Rules")
	FLeashRules Rules;

	/** Write a line to the log whenever an enemy gives up or starts again. */
	UPROPERTY(config, EditAnywhere, Category = "Diagnostics")
	bool bLogTransitions;
};
