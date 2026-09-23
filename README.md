# LeashPoint

Aggro leash, disengage and return for Unreal Engine 5.8.

An enemy that never gives up is a bug, and an enemy that gives up and immediately starts again is a
worse one. LeashPoint is the small set of rules that decides when a fight is over.

* The leash is measured from the enemy's HOME, not from the target
* The leash radius is held a fixed distance beyond the aggro radius, so the boundary cannot flicker
* A re-acquire delay covers the walk home
* The lost-sight timer resets on every sighting instead of accumulating
* An enemy will not start a chase it would have to abandon a step later
* A pack gives up together; healing on the way back is a setting
* Every rule is a pure function the component and the tests both call

Documentation: https://wiki.teufel-engineering.com/en/LeashPoint/documentation
Support: teufelsilvan@gmail.com

Unreal Engine 5.8 - Win64 - one runtime C++ module - no third-party code - full source included.

<!-- SF-STORE-BLOCK:BEGIN -->
## 🛒 Source-available — see before you buy

This repository contains the **full source** of a commercial Unreal Engine plugin. It is **source-available, not open source**: read it, evaluate it, then buy a license to use it. See **the Fab Content License Agreement / Unreal Engine EULA (purchase required)**.

**Get it / Buy:**
- **Buy on Fab** (this plugin): https://www.fab.com/listings/346904c8-66b4-45dd-b93d-46ca8daf996b
- Fab store — all our UE5 plugins: https://www.fab.com/sellers/Silvan%20Teufel

### 📬 **Free UE5 Snippet-Pack**

10 ready-to-use C++/Blueprint building blocks (subsystems, versioned saves, async nodes, editor tooling) — MIT licensed. Get it by joining the newsletter — plus a heads-up when something new ships. Double opt-in, unsubscribe in one click, no address sharing.

👉 **[Get the free pack](https://silvan.teufel-engineering.com/newsletter/plugins/?q=gh)**

_© 2026 Silvan Teufel. All rights reserved._
<!-- SF-STORE-BLOCK:END -->
