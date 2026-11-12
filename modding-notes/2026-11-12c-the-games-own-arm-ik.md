# The game's own arm IK, and the way in for the motion controllers (2026-11-12, Fable; nothing launched)

Step 3 of the hands-and-melee plan (board, `claude-memory/status/condemned-2-vr.md`). Static reading only; the probe
build compiles and waits for its first run.

## Result

**Condemned 2 has a two-bone arm IK of its own, and it will solve towards a point we write** `[inferred-static 2026-11-12]`.
So the hands go the same way as The Darkness's (the game's own IK fed a target per hand), not by moving the whole
arm-and-weapon as one piece.

## What was read (guest addresses; tools in `dev-archive/tools/`, image `D:/condemned-2-build/unpacked/default.bin`)

- Class names in the program: `PlayerBody` (vtable 0x82020D88), `PlayerAiming` (0x82021A68), `PlayerWeaponMgr`
  (0x8201DE10), `CHHWeaponModel` (0x8202EB5C), `CNodeTracker` (0x8206D91C), `CLeanNodeController` (0x8206BE70),
  `CServerMeleeCollisionController` (0x82039520). The player has a full body (`PlayerBody Upper / Lower / Main`).
- Database category `Character/Models/IKChain`; a chain record has `RootNode`, `JointNode`, `TargetNode`,
  `JointTargetDist` (default 100), `DownJointSign` (default 1). The records are in the compressed archives: their
  names are not readable from disk (searched both `.Arch02` files: no plain text).
- **`sub_82396D30` = the model key `IK ON <chain> SyncTarget|Weapon <socket> [blend]` / `IK OFF [blend]`**: r3 = the
  IK manager, r4 = the model, r5 = a tracker byte, r6 = the parsed words (count at +0, word i's text pointer at
  +12+8i+4). Called from three model-key handlers; the manager is always `<character client object> + 2264`
  (`sub_822E8838` directly; `sub_823AC358` and `sub_823B7640` through `[x+4]+308` / `[x+20]+308`).
- **The manager holds TWO chain slots of 216 bytes** (manager+8 and +224); a slot is in use when its +12 is set.
- **`sub_823966F8(chain, model, record)` sets a chain up**: +12 model, +16 record, +92 / +96 / +100 the root / joint /
  target node handles, +104 / +108 the two bone lengths, +112 JointTargetDist, +116 DownJointSign, then registers
  three node-control callbacks (user data = the chain): root `sub_82396130`, joint `sub_82395E68`, target `sub_823957E8`.
- **ILTModel (client) = `*(0x829643F0)`**; vtable +8 GetSocket(obj, name, &socket), +12 GetSocketTransform(obj,
  socket, &transform, world), +48 GetNode(obj, name, &node), +88 GetNodeTransform(obj, node, &transform, world),
  +96 AddNodeControlFn(obj, node, fn, user), +172 (obj, tracker, &anim: the chain remembers the animation that
  switched it on; `IKAutoReleaseBlendTime` in `sub_82395878` lets go when it changes) `[inferred-static]`.
  The database is `*(0x82966BA8)`; the IKChain category handle is `*(0x829643D4)`; `sub_826A5F78(category, name)`
  returns a record.
- **The way in: the root callback `sub_82396130(r3 = node data, r4 = chain)`** does nothing when the chain's weight
  (+208) is 0; when the target object (+56) is set it asks ILTModel for that object's socket (+60) transform and
  stores position at +64 and rotation at +76; **when the target object is 0 it solves towards whatever +64 / +76
  hold.** A transform is 3 floats position + 4 floats quaternion.
- Melee, for step 4: `CServerMeleeCollisionController` ("EnableCollisions: Too many attacks and blocks at once"), the
  attack kinds `MAT_Forehand`, `MAT_Backhand`, `MAT_RightOverHand`, `MAT_LeftOverHand`, and
  `PlayerBodyMeleeImpactFXIntersectDist`: melee hits come from a collision that follows the weapon, and the attacks
  already have directions.

## The probe build (compiled, in `C:/NonSteam/condemned-2/build-test-c2-home` only; b003 stays the installed build)

`dev-archive/port-project/src/vr_hand_ik.cpp` `[compile-verified 2026-11-12]`:
- logs every IK key with its words, every chain set-up and every chain the root callback solves (`[C2IK]` lines);
- `ik.force=1` makes every solved chain reach for the world point `ik.tx / ik.ty / ik.tz` (the target object is
  hidden from the callback for that call and put back);
- **the MCP Game Bridge is in the game now** (`C2_BRIDGE=1`; `src/bridge/` from The Darkness's copy), with guest tools
  run at frame end on the game's thread: `gmem <addr> [n]`, `gcall <fn> [r3..r8]`, `gstr <text>`, `gbuf`, `ik`.

## Open, for the first run (simulator, pretend controllers)

1. Does the game switch any chain on for the PLAYER in ordinary play (a two-handed weapon's off hand)? The log says.
2. The player's client object and model: through a logged key, else from `PlayerBody` (0x82020D88) live.
3. The arm's node names (GetNode with `gstr`), or a chain record by name, to set up one chain per arm ourselves
   (the two slots, target object 0), kept on against the auto-release.
4. Then the feed: each controller's grip, relative to the head, laid from the camera (as The Darkness's `ik.src=1`).

## First runs, the same evening (simulator, pretend controllers; b004, installed)

**The hands follow the controllers** `[verified-live 2026-11-12, n=3 positions, simulator]`. Pictures:
`dev-archive/recon/2026-11-12-hands-first-run/` (left to right: off = the game's own pose, on, on with reach x1.6;
and on after walking to a wall).

- The game switches no chain on for the player by itself at rest (no key, no set-up in the log).
- **The player, live:** `*(*(0x829343B4) + 604 + 4)` = player 0 (`Player`, 3976 bytes); `+216` = its `PlayerBody`
  (vtable 0x82020D88; `PlayerBody+4` = the player); `+308` = the character client object, whose IK manager is at
  `+2264` (manager+0 = that object; both slots free) `[verified-live]`. `sub_822E3A38(character)` = its model object.
- **The IKChain category has exactly two records: `RightArm` and `LeftArm`** (nodes named like `R_Armu`, `L_Armu`,
  `L_Arml`) `[verified-live]`. The models database object is `*(0x829643D4)`, the category at its `+324`; a category
  holds its count at `+8` and its records (24 bytes each) at `+12`; database vtable (0x820915F0): slot 17 = record by
  name, 18 = count, 19 = record by index, 13 = category name.
- **What the mod does (`vr_hand_ik.cpp`, `C2_HANDS=1` / bridge `ik.on`):** at frame end, for each free slot of the
  player's manager (slot 0 `LeftArm`, slot 1 `RightArm`): `sub_823966F8(chain, model, record)`, target object 0,
  state (`+212`) 2 = held. The game's chain update `sub_82396B90` is skipped for our chains (it lets go of a chain
  with no target object, and when the animation changes). In the root callback each solve gets the weight (`+208`:
  1 with a valid controller, 0 without = the animation's arm) and the target (`+64`): camera position + the game
  camera's rotation (before the head turn) applied to (grip - head), OpenXR to LithTech axes (z flipped), x 100
  units per metre x `ik.scale`. `ik.rot=1` also writes the controller's rotation to `+76` (not looked at yet).
- The weapon (a plank in the right hand) moves with the hand: it hangs on the hand bone.
- Bridge: `hands` prints chains, the controller sample, the camera and both targets; `gmem` / `gcall` / `gstr` / `gbuf`.

## The Quit line's fault in b003, fixed in b004

A on ANY main-menu line builds every page's texts at once, `IDS_MULTIPLAYER_MENU` among them, so b003's hook closed
the game on Single Player too `[verified-live 2026-11-12, n=2]` (the b003 check only ever pressed A on the Quit line).
b004 remembers the last `*_TOOLTIP` text looked up (the highlighted line's hint) and closes only when that is
`IDS_MULTIPLAYER_TOOLTIP`: A on Single Player loads the save `[verified-live 2026-11-12, n=1]`. Quit itself on b004:
not pressed again yet.

## Open

1. In the headset: do the hands sit where the real controllers are (scale, a fixed offset from grip to palm)?
2. The hand's rotation (`ik.rot`, plus a fixed turn from the controller's axes to the hand bone's).
3. Cutscenes, ladders, finishing moves, two-handed weapons: where our chains must let go (weight 0).
4. Does the game's own `IK ON` key ever want a slot we hold (it finds none free and silently does nothing)?
5. Level change: the chains are set up again by themselves (player pointer); not exercised yet.
