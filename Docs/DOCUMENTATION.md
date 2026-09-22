# LeashPoint — Aggro Leash, Disengage & Return

**An enemy that never gives up is a bug. An enemy that gives up and immediately starts again is a
worse one.**

LeashPoint is the small set of rules that decides when a fight is over.

---

## 0. Supported engine and platforms

* Unreal Engine **5.8**
* **Win64.** The plugin's `PlatformAllowList` is Win64 only; Mac and Linux are not supported.
* One runtime C++ module, no third-party code, full source included.
* No dependency on AIModule or NavigationSystem. LeashPoint decides **when** a chase is over and
  **where home is**; walking back there is your behaviour tree's job.

---

## 1. The five-minute install

1. Add a **Leash Point** component to the enemy. Home defaults to wherever it was placed.
2. From your perception handler: `TryAcquire(Target)` — it returns whether the rules allowed it.
3. While the target is visible: `NotifyTargetSeen()`.
4. Bind `OnGaveUp(Reason)` and send the pawn home with whatever navigation you already use.
5. Bind `OnReturnedHome()` and put it back on its patrol.

---

## 2. The leash is measured from HOME

Not from the target. This is the rule the plugin exists for.

Measure the distance between the enemy and its target, and a player running away at the enemy's own
speed holds that distance constant forever — the enemy never gives up, and it ends up three rooms
away from where the designer put it.

Measured from home, the enemy's **own** position decides, which is the only measurement it can be
sure about. `LeashRadius` is a circle around the post, and stepping outside it ends the chase.

Height is deliberately ignored. A guard standing on a ledge two metres above its post has not
wandered off, and counting the height would make every enemy on a staircase leash early.

---

## 3. The boundary must not flicker

`AggroRadius` starts a chase. `LeashRadius` ends one. If the second is not comfortably larger than
the first, the enemy gives up and re-acquires on the same spot, forever.

`NormaliseRules` enforces it: `LeashRadius` is pushed out to at least `AggroRadius + MinRadiusGap`.
Everything in the library assumes that has been done, and the component does it for you.

This is hysteresis, and it is not optional.

---

## 4. The re-acquire delay covers the walk home

`ReacquireDelaySeconds` runs from the moment the chase was **abandoned**, not from the moment the
enemy arrives. A delay that only started on arrival would let a player re-pull an enemy halfway back
across the room, and the fight would never end.

---

## 5. The lost-sight timer resets, it does not accumulate

`LoseSightSeconds` is measured since the **last sighting**. `NotifyTargetSeen` puts it back to zero.

Accumulate blind seconds across glimpses instead and you get an enemy that drops a target it can
plainly see, purely because the fight has been going on for a while.

`LoseSightSeconds = 0` switches the rule off. It does not mean "give up immediately" — the opposite
reading would make every enemy give up on the first frame of every fight.

---

## 6. An enemy will not start a chase it would have to abandon

`bRefuseUnwinnableChases` (on by default) refuses a target that is already outside the leash radius
from home, even if it is inside the aggro radius of the enemy.

A chase that has to be abandoned on the second step looks exactly like a bug. The enemy is better
off never leaving its post.

---

## 7. The pack goes together

`JoinGroup(Other)` links enemies both ways. When any one of them gives up, the whole group does,
with the reason `GroupGaveUp`.

Any member, not all members: half a pack returning while the other half keeps swinging is the worst
of both. The player is still in a fight, and it is no longer the fight that was designed.

`ULeashPointStatics::GroupGivesUp` is the rule on its own if you would rather run your own group
bookkeeping.

---

## 8. Coming home is a setting

| `HealMode` | What it means |
|---|---|
| `None` | The enemy walks home hurt. A second attempt is cheaper than the first. |
| `OnArrival` | Full health the moment it arrives. A failed pull costs nothing and gains nothing. |
| `OverTime` | Health comes back on the way. Rewards a player who cuts the escape off. |

All three are real designs, and which one a project wants is not something a plugin gets to decide.
`HealOnReturn` never asks the world whether the enemy has arrived — the caller answers that, because
a pure function is not allowed to ask the world anything.

---

## 9. What LeashPoint is not

* **It does not move anything.** No navigation, no behaviour tree, no pathfinding. It decides; you
  move.
* **It does not do perception.** Call `NotifyTargetSeen` from whatever sight system you already have.
* **It is not a threat table.** Who an enemy hates is a different question from whether the fight is
  still on.
* **It is not replicated.** Leash state is a server-side decision; replicate the outcome.

---

## 10. Console commands

| Command | What it does |
|---|---|
| `LeashPoint.Dump` | Every enemy with a leash component: state, distance from home, both timers, give-up count and last reason. |
| `LeashPoint.Release` | Send every engaged enemy home. The way out when a fight will not end. |

---

## 11. API reference

### `ULeashPointComponent`

`SetHome(FVector)`, `GetHome()`, `TryAcquire(AActor*)`, `NotifyTargetSeen()`, `GiveUp(Reason)`,
`NotifyArrivedHome()`, `JoinGroup(Other)`, `LeaveGroup()`, `AdvanceTime(float)`, `SetAutoTick(bool)`,
`SetRuleOverride(FLeashRules)`, `IsEngaged()`, `IsReturning()`, `GetTarget()`, `GetState()`,
`GetRules()`, `GetDistanceFromHome()`.

Delegates: `OnGaveUp(Reason)`, `OnAcquired(Target)`, `OnReturnedHome()`.

### `ULeashPointStatics` — the rules, on their own

`NormaliseRules`, `EvaluateGiveUp`, `CanAcquire`, `HasArrivedHome`, `HealOnReturn`, `GroupGivesUp`,
`Advance`, `DistanceFromHome`.

No world, no actor, no navigation, no clock. The component calls exactly these and so do the tests.

---

## 12. The demo level

`Content/LeashPoint/Maps/L_LeashPointDemo` — one lone guard and a pack of three, and a target that
walks past both.

Watch the lone guard first: it starts when the target enters the inner ring, follows, and stops at
the outer ring — measured from its post, which is drawn as a line back to it. Then the target walks
straight back into the returning guard and **nothing happens**, because the re-acquire delay is
still running.

Then the pack: all three start, the target drags them right, and the first one to reach its own
outer ring takes the other two with it.

The radii in the demo are deliberately small so the whole thing fits in one picture — the shipped
defaults are more than twice as large. The point being made is about the **relationship** between
the two rings, not their size.

**If the level looks frozen**, the viewport is not set to realtime. Either switch realtime on, or
call `StepDemo(Seconds)` on the director yourself — that is what the screenshot run does.

---

## 13. Troubleshooting

**The enemy never gives up.** It is never getting far enough from home. Check `LeashPoint.Dump`:
"from home" against the leash radius. Also check you are calling `AdvanceTime` or leaving `bAutoTick`
on.

**The enemy gives up instantly.** `LoseSightSeconds` is small and nothing is calling
`NotifyTargetSeen`, or home was never set and defaults to the origin while the enemy stands
elsewhere.

**It flickers between chasing and returning.** You are bypassing `NormaliseRules` — call the statics
through the component, or normalise once and keep the result.

**A player can re-pull it halfway home.** `ReacquireDelaySeconds` is zero.

**One pack member keeps fighting.** It was never added with `JoinGroup`, or it was added to a
different member and the link is one-way — `JoinGroup` links both directions, so call it once per
pair.

**It refuses to start a fight it clearly should.** `bRefuseUnwinnableChases` with a target outside
the leash radius. That is the rule working; move the post or widen the leash.
