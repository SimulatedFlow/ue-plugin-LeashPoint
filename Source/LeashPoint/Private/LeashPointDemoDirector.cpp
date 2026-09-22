// Copyright 2026 Silvan Teufel. All Rights Reserved.

#include "LeashPointDemoDirector.h"

#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "LeashPointStatics.h"

namespace LeashPointDemoLocal
{
	/** The floor sits in front of the wall; the camera looks along +Y, so horizontal is X. */
	constexpr float FloorY = 260.0f;
	constexpr float GuardSpeed = 240.0f;

	const FColor Idle(150, 155, 170);
	const FColor Chasing(230, 110, 100);
	const FColor Returning(120, 160, 235);
	const FColor HomeRing(90, 100, 120);
	const FColor LeashRing(200, 150, 90);
	const FColor TargetColour(240, 225, 130);

	/** The target's path, as (second, X, Y-offset-on-the-floor). Lerped between keys. */
	struct FSchritt { float T; float X; float Y; };

	static const FSchritt Pfad[] = {
		{ 0.0f, -2500.0f,  -60.0f},
		{ 3.0f,  -900.0f,   40.0f},   // into the lone guard's aggro ring
		{ 8.0f,   400.0f,  120.0f},   // drag it right until the leash gives
		{11.0f,  -400.0f,  160.0f},   // come back at the returning guard
		{15.0f,   900.0f,  -80.0f},   // into the pack
		{21.0f,  2300.0f,  140.0f},   // drag the pack right
		{26.0f,  2700.0f,  220.0f},
		{28.0f,  2900.0f,  240.0f},
	};
	constexpr int32 PfadLaenge = UE_ARRAY_COUNT(Pfad);
}

ALeashPointDemoDirector::ALeashPointDemoDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BoardText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BoardText"));
	BoardText->SetupAttachment(Root);
	BoardText->SetHorizontalAlignment(EHTA_Center);
	BoardText->SetVerticalAlignment(EVRTA_TextBottom);
	BoardText->SetWorldSize(86.0f);
	BoardText->SetTextRenderColor(FColor::White);
	// Yaw 270, not 90: at 90 a TextRender renders mirrored.
	BoardText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	BoardText->SetRelativeLocation(FVector(0.0f, 0.0f, 1180.0f));

	StateText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StateText"));
	StateText->SetupAttachment(Root);
	StateText->SetHorizontalAlignment(EHTA_Center);
	StateText->SetVerticalAlignment(EVRTA_TextTop);
	StateText->SetWorldSize(58.0f);
	StateText->SetTextRenderColor(FColor(205, 212, 226));
	StateText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	StateText->SetRelativeLocation(FVector(-1120.0f, 0.0f, 1030.0f));

	RuleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RuleText"));
	RuleText->SetupAttachment(Root);
	RuleText->SetHorizontalAlignment(EHTA_Center);
	RuleText->SetVerticalAlignment(EVRTA_TextTop);
	RuleText->SetWorldSize(60.0f);
	RuleText->SetTextRenderColor(FColor(250, 205, 120));
	RuleText->SetRelativeRotation(FRotator(0.0f, 270.0f, 0.0f));
	RuleText->SetRelativeLocation(FVector(1140.0f, 0.0f, 1030.0f));
}

FVector ALeashPointDemoDirector::Basis() const
{
	return GetActorLocation() - FVector(0.0f, LeashPointDemoLocal::FloorY, GetActorLocation().Z);
}

void ALeashPointDemoDirector::BeginPlay()
{
	Super::BeginPlay();
	StartCycle();
}

void ALeashPointDemoDirector::StartCycle()
{
	CycleTime = 0.0f;

	// Small radii so the whole thing fits in one picture. The shipped defaults are more than twice
	// this; the point being made is about the RELATIONSHIP between the two rings, not their size.
	Regeln = FLeashRules();
	Regeln.AggroRadius = 380.0f;
	Regeln.LeashRadius = 950.0f;
	Regeln.MinRadiusGap = 200.0f;
	Regeln.LoseSightSeconds = 0.0f;     // off: this demo is about distance, and two rules at once
	                                    // in one picture is one rule too many
	Regeln.ReacquireDelaySeconds = 4.0f;
	Regeln.ArriveTolerance = 90.0f;
	Regeln.bRefuseUnwinnableChases = true;
	Regeln = ULeashPointStatics::NormaliseRules(Regeln);

	Wachen.Reset();

	auto Setze = [this](const FString& Name, const float X, const bool bPack)
	{
		FWache W;
		W.Name = Name;
		W.bPack = bPack;
		W.Stand.Home = FVector(X, 0.0f, 0.0f);
		W.Stand.SecondsSinceGaveUp = 1000.0f;
		W.Ort = W.Stand.Home;
		Wachen.Add(W);
	};

	Setze(TEXT("LONE"), -900.0f, false);
	Setze(TEXT("PACK A"), 780.0f, true);
	Setze(TEXT("PACK B"), 980.0f, true);
	Setze(TEXT("PACK C"), 1180.0f, true);

	bBereit = true;
}

FVector ALeashPointDemoDirector::ZielOrt(const float T) const
{
	using namespace LeashPointDemoLocal;

	if (T <= Pfad[0].T)
	{
		return FVector(Pfad[0].X, Pfad[0].Y, 0.0f);
	}
	for (int32 i = 1; i < PfadLaenge; ++i)
	{
		if (T <= Pfad[i].T)
		{
			const float Spanne = FMath::Max(KINDA_SMALL_NUMBER, Pfad[i].T - Pfad[i - 1].T);
			const float A = (T - Pfad[i - 1].T) / Spanne;
			return FVector(FMath::Lerp(Pfad[i - 1].X, Pfad[i].X, A),
				FMath::Lerp(Pfad[i - 1].Y, Pfad[i].Y, A), 0.0f);
		}
	}
	return FVector(Pfad[PfadLaenge - 1].X, Pfad[PfadLaenge - 1].Y, 0.0f);
}

void ALeashPointDemoDirector::SchritteFuer(FWache& W, const float DeltaSeconds, const FVector& Zielort)
{
	const FVector Nach = W.Stand.bEngaged ? Zielort : W.Stand.Home;
	FVector Richtung = Nach - W.Ort;
	Richtung.Z = 0.0f;

	const float Abstand = Richtung.Size();
	if (Abstand > 1.0f)
	{
		const float Schritt = FMath::Min(LeashPointDemoLocal::GuardSpeed * DeltaSeconds, Abstand);
		W.Ort += (Richtung / Abstand) * Schritt;
	}
}

void ALeashPointDemoDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bAutoRun)
	{
		// The editor hands out one enormous delta after a recompile.
		StepDemo(FMath::Clamp(DeltaSeconds, 0.0f, 0.1f));
	}
}

void ALeashPointDemoDirector::StepDemo(const float DeltaSeconds)
{
	// BeginPlay does not run in an editor viewport.
	if (!bBereit)
	{
		StartCycle();
	}

	CycleTime += DeltaSeconds;
	if (CycleTime > CycleSeconds)
	{
		StartCycle();
		return;
	}

	Ziel = ZielOrt(CycleTime);

	// --- the rules, once per guard -------------------------------------------------------------
	for (FWache& W : Wachen)
	{
		W.Stand = ULeashPointStatics::Advance(W.Stand, DeltaSeconds);

		// A sighting every frame: this demo is about distance, not about sight.
		if (W.Stand.bEngaged)
		{
			W.Stand.SecondsSinceSeen = 0.0f;
		}

		if (W.Stand.bEngaged)
		{
			const ELeashGiveUpReason Grund = ULeashPointStatics::EvaluateGiveUp(W.Stand, W.Ort, Regeln);
			if (Grund != ELeashGiveUpReason::None)
			{
				W.Stand.bEngaged = false;
				W.Stand.bReturning = true;
				W.Stand.SecondsSinceGaveUp = 0.0f;
				W.Stand.LastReason = Grund;
				W.Stand.GiveUpCount += 1;
			}
		}
		else if (ULeashPointStatics::CanAcquire(W.Stand, W.Ort, Ziel, Regeln))
		{
			W.Stand.bEngaged = true;
			W.Stand.bReturning = false;
			W.Stand.SecondsSinceSeen = 0.0f;
			W.Stand.LastReason = ELeashGiveUpReason::None;
		}

		if (W.Stand.bReturning && ULeashPointStatics::HasArrivedHome(W.Ort, W.Stand, Regeln))
		{
			W.Stand.bReturning = false;
		}
	}

	// --- the pack goes together ----------------------------------------------------------------
	TArray<ELeashGiveUpReason> PackGruende;
	for (const FWache& W : Wachen)
	{
		if (W.bPack)
		{
			PackGruende.Add(W.Stand.LastReason != ELeashGiveUpReason::None && !W.Stand.bEngaged
				? W.Stand.LastReason : ELeashGiveUpReason::None);
		}
	}
	if (ULeashPointStatics::GroupGivesUp(PackGruende))
	{
		for (FWache& W : Wachen)
		{
			if (W.bPack && W.Stand.bEngaged)
			{
				W.Stand.bEngaged = false;
				W.Stand.bReturning = true;
				W.Stand.SecondsSinceGaveUp = 0.0f;
				W.Stand.LastReason = ELeashGiveUpReason::GroupGaveUp;
				W.Stand.GiveUpCount += 1;
			}
		}
	}

	for (FWache& W : Wachen)
	{
		SchritteFuer(W, DeltaSeconds, Ziel);
	}

	if (BoardText)
	{
		BoardText->SetText(FText::FromString(HeadlineFor()));
	}
	if (StateText)
	{
		StateText->SetText(FText::FromString(BuildStateText()));
	}
	if (RuleText)
	{
		RuleText->SetText(FText::FromString(RuleFor()));
	}

	if (bDrawDemo)
	{
		DrawScene();
	}
}

FString ALeashPointDemoDirector::HeadlineFor() const
{
	// Each headline has to hold for the WHOLE phase it covers.
	if (CycleTime < 8.5f)
	{
		return TEXT("the leash is measured from the guard's post, never from the target");
	}
	if (CycleTime < 13.0f)
	{
		return TEXT("it gave up and is walking back - and walking into it does not restart the fight");
	}
	if (CycleTime < 21.5f)
	{
		return TEXT("a pack: the first one to reach its own leash takes the other two with it");
	}
	return TEXT("everybody is going home, and what that costs the player is a setting");
}

FString ALeashPointDemoDirector::RuleFor() const
{
	if (CycleTime < 8.5f)
	{
		return TEXT("THE RULE\n\ndistance is measured\nfrom HOME\n\nmeasured between guard\nand target, a player\nwho keeps running is\nnever dropped");
	}
	if (CycleTime < 13.0f)
	{
		return TEXT("NO FLICKER\n\nthe leash ring is held\na fixed gap outside the\naggro ring\n\nand the re-acquire delay\ncovers the walk back");
	}
	if (CycleTime < 21.5f)
	{
		return TEXT("THE PACK\n\nany one member giving up\ntakes the whole pack\n\nhalf a pack returning is\nworse than none");
	}
	return TEXT("COMING BACK\n\nheal on arrival, heal on\nthe way, or not at all\n\nit is a setting, not an\nassumption");
}

FString ALeashPointDemoDirector::BuildStateText() const
{
	FString S = FString::Printf(TEXT("aggro %.0f   leash %.0f\n\n"), Regeln.AggroRadius, Regeln.LeashRadius);
	S += TEXT("THE GUARDS\n");
	for (const FWache& W : Wachen)
	{
		const float Weg = ULeashPointStatics::DistanceFromHome(W.Ort, W.Stand);
		const TCHAR* Zustand = W.Stand.bEngaged ? TEXT("CHASING  ")
			: (W.Stand.bReturning ? TEXT("returning") : TEXT("at post  "));

		S += FString::Printf(TEXT("%-7s %s %4.0f/%4.0f\n"), *W.Name, Zustand, Weg, Regeln.LeashRadius);
	}

	S += TEXT("\nSINCE GIVING UP\n");
	for (const FWache& W : Wachen)
	{
		if (W.Stand.GiveUpCount == 0)
		{
			S += FString::Printf(TEXT("%-7s -\n"), *W.Name);
			continue;
		}
		const bool bGesperrt = W.Stand.SecondsSinceGaveUp < Regeln.ReacquireDelaySeconds;
		S += FString::Printf(TEXT("%-7s %4.1fs %s\n"), *W.Name, W.Stand.SecondsSinceGaveUp,
			bGesperrt ? TEXT("(will not start)") : TEXT(""));
	}
	return S;
}

void ALeashPointDemoDirector::DrawScene() const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UWorld* Mutable = const_cast<UWorld*>(World);

	// Bleibende Linien plus ein Loeschen zu Beginn jedes Schritts - NICHT Linien mit Standzeit.
	//
	// WARUM: eine Standzeit laeuft ueber den Welt-Takt ab, und in einem Editor-Viewport ohne
	// Echtzeit tickt die Welt gar nicht. Die Formen blieben dann entweder ewig stehen (und jedes
	// Bild waere die Summe aller vorherigen) oder waeren beim Auslesen laengst weg. So haengt das
	// Bild nur noch davon ab, dass StepDemo gerufen wurde.
	FlushPersistentDebugLines(Mutable);

	using namespace LeashPointDemoLocal;


	const FVector B = Basis();

	// The circles lie flat on the floor: the two axes span the XY plane.
	const FVector AchseX(1.0f, 0.0f, 0.0f);
	const FVector AchseY(0.0f, 1.0f, 0.0f);

	for (const FWache& W : Wachen)
	{
		const FVector Post = B + W.Stand.Home + FVector(0.0f, 0.0f, 4.0f);

		DrawDebugCircle(Mutable, Post, Regeln.AggroRadius, 48, HomeRing, true, -1.0f, 0, 4.0f,
			AchseX, AchseY, false);
		DrawDebugCircle(Mutable, Post, Regeln.LeashRadius, 64, LeashRing, true, -1.0f, 0, 5.0f,
			AchseX, AchseY, false);

		// The post itself.
		DrawDebugLine(Mutable, Post, Post + FVector(0.0f, 0.0f, 120.0f), HomeRing, true, -1.0f, 0, 6.0f);

		const FColor Farbe = W.Stand.bEngaged ? Chasing : (W.Stand.bReturning ? Returning : Idle);
		const FVector Hier = B + W.Ort + FVector(0.0f, 0.0f, 60.0f);

		DrawDebugSphere(Mutable, Hier, 55.0f, 12, Farbe, true, -1.0f, 0, 6.0f);

		// A line back to the post, so "how far from home" is a thing you can see rather than read.
		DrawDebugLine(Mutable, Hier, Post, Farbe, true, -1.0f, 0, 3.0f);

		if (W.Stand.bEngaged)
		{
			DrawDebugLine(Mutable, Hier, B + Ziel + FVector(0.0f, 0.0f, 60.0f),
				Chasing, true, -1.0f, 0, 5.0f);
		}
	}

	// The target.
	const FVector Z = B + Ziel + FVector(0.0f, 0.0f, 70.0f);
	DrawDebugSphere(Mutable, Z, 70.0f, 14, TargetColour, true, -1.0f, 0, 8.0f);
	DrawDebugLine(Mutable, Z - FVector(0.0f, 0.0f, 70.0f), Z + FVector(0.0f, 0.0f, 90.0f),
		TargetColour, true, -1.0f, 0, 5.0f);
#endif
}
