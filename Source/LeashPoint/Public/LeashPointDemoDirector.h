// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LeashPointTypes.h"
#include "LeashPointDemoDirector.generated.h"

class UTextRenderComponent;

/**
 * Runs the shipped demo: one guard and one pack, and a target that walks past both.
 *
 * The guards are drawn rather than spawned - an editor viewport is where the store images are taken,
 * and spawning actors from a viewport tick would leave them in the level. Every decision they make
 * comes from ULeashPointStatics, which is the plugin: the component in LeashPointComponent.h is a
 * clock and a delegate list around exactly these calls, and the tests call them too.
 *
 * The radii here are small so the whole thing fits in one picture. The defaults that ship in
 * Project Settings are more than twice as large.
 */
UCLASS(meta = (DisplayName = "LeashPoint Demo Director"))
class LEASHPOINT_API ALeashPointDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	ALeashPointDemoDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint Demo",
		meta = (ClampMin = "16.0", ClampMax = "180.0"))
	float CycleSeconds = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint Demo")
	bool bDrawDemo = true;

	/** Let Tick drive the demo. Off when something else steps it - see StepDemo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LeashPoint Demo")
	bool bAutoRun = true;

	/**
	 * Advance the demo by exactly this many seconds and redraw.
	 *
	 * An actor only ticks in an editor viewport while that viewport is set to realtime, which is
	 * the user's setting and not the plugin's. The screenshot run steps the demo by hand instead,
	 * so frame N always shows the same moment of the script.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LeashPoint Demo")
	void StepDemo(float Seconds);

	/** The headline. TextRender, because HighResShot does not capture DrawDebugString. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LeashPoint Demo")
	TObjectPtr<UTextRenderComponent> BoardText;

	/** Every guard, in words. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LeashPoint Demo")
	TObjectPtr<UTextRenderComponent> StateText;

	/** The rule the current phase is showing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LeashPoint Demo")
	TObjectPtr<UTextRenderComponent> RuleText;

private:
	/** One guard: the plugin's state, plus where it currently is. */
	struct FWache
	{
		FLeashState Stand;
		FVector Ort = FVector::ZeroVector;
		FString Name;
		bool bPack = false;
	};

	void StartCycle();
	FVector ZielOrt(float T) const;
	void SchritteFuer(FWache& W, float DeltaSeconds, const FVector& Ziel);
	FString BuildStateText() const;
	FString HeadlineFor() const;
	FString RuleFor() const;
	void DrawScene() const;

	FVector Basis() const;

	TArray<FWache> Wachen;
	FVector Ziel = FVector::ZeroVector;
	float CycleTime = 0.0f;
	bool bBereit = false;

	FLeashRules Regeln;
};
