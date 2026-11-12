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
