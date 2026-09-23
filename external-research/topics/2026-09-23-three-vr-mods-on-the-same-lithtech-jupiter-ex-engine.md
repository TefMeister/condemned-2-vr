# Three VR mods on the same LithTech Jupiter EX engine, and where they hook the frame

**Found:** 2026-09-23, `/gr` estate sweep, through phunkaeg's *VR Modding Playbook*, which calls
Jupiter EX **"the densest single-engine cluster"** in its survey (`sources.yml` → `fear-vr`,
`FEAR2VR`, `condemned-vr`).
**Sources:** DR-89, *fear-vr* — <https://github.com/DR-89/fear-vr> (MIT); *FEAR2VR* and
*condemned-vr* are recorded in the playbook with MIT licences but no public link, and one web search
did not find them. Nothing copied.

## What they are `[reported]`

| Mod | Game | How far it got |
| --- | --- | --- |
| fear-vr | F.E.A.R. (2005), PC D3D9 | Native stereo, head tracking, controllers, stereo HUD, world-locked menu; confirmed in a Quest 3 headset. Its design notes are in **German**. |
| condemned-vr | **Condemned: Criminal Origins**, PC D3D9 | Native stereo, OpenXR head tracking and motion controls, game install unchanged; four milestones passed live, physical melee in progress. |
| FEAR2VR | F.E.A.R. 2, PC D3D9 | Partial; notable mainly for a 2 GB address-space crash lesson. |

## Why it matters here

Condemned 2 is the same engine family, and the first game's VR mod is the nearest thing to prior art
this project has. ⚠️ **Our build is the Xbox 360 game recompiled through ReXGlue**, not a PC Jupiter
EX binary, so no PC address, export or hook transfers `[hypothesis]`. What can transfer is the
**engine's frame order**, because the recompiler keeps the game's own logic:

- fear-vr's rule, read from the engine's own public SDK source: **the smallest safe hook sits below
  all client updates and above the pure world render** `[reported]`. In the PC code that is the step
  where the player camera's render function clears the target and calls the renderer's
  `RenderCamera` exactly once, after pre-render, effects and streaming have run, and before dynamic
  effects and the interface are drawn.
- If Condemned 2's recompiled code keeps that shape, the equivalent function is the place to issue a
  second eye, rather than at the GPU-emulation layer where our current Seam A sits `[hypothesis]`.
  This is a question to ask of the recompiled code, not an answer.

Smaller points `[reported]`:

- **condemned-vr's room-scale write-up** is, by the playbook's account, the most complete in its
  survey: move the player through the game's own movement commands, never teleport; publish one
  command per simulation frame, never per eye; and constrain the head separately, because the body's
  collision does not protect it.
- condemned-vr carries a guarded fix for **Jupiter EX's redundant input-device initialisation**, an
  engine-level performance loss that the playbook expects to transfer across the family.

## Next step

When the stereo row comes up, compare our recompiled Condemned 2 frame against the F.E.A.R. SDK's
frame order (the SDK is Monolith's public release) to see whether a `RenderCamera`-shaped call
exists. Also try once more to find condemned-vr's public home; the playbook lists it but gives no link.

## Credits

DR-89 (fear-vr); the condemned-vr and FEAR2VR authors (unnamed in the source; to be credited by name
once found); phunkaeg (*VR Modding Playbook*); Monolith Productions (F.E.A.R. public SDK, as the
engine reference those mods used).
