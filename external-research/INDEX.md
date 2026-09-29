# Research index

**Last `/gr` pass: 2026-09-29 (estate sweep) — CHECK-IN.** Inbox empty. One topic answering the `openxr_loader.dll` row: the string is most likely SDL3's unused XR module bundled with the ReXGlue SDK, not a VR path; pointer dropped in `engine-research/inbox/`.

_Previous: **Last `/gr` pass: 2026-09-23 (estate sweep) — FULL.** Checked against phunkaeg's *VR Modding Playbook*: three VR mods on LithTech Jupiter EX (fear-vr, condemned-vr, FEAR2VR) and fear-vr's frame-order rule for where a second eye goes._

_Previous: **Last `/gr` pass: 2026-09-17 (estate sweep) — CHECK-IN.** First pass: folder bootstrapped; one topic on the upstream recomp's activity and the ReXGlue SDK's recent nightlies (nothing in public addresses the AVX2 requirement)._

Every research topic gathered for this project, newest first. Each row links to a self-contained
write-up in `topics/`. Status tags:

- 🆕 **new** — found, not yet acted on by the modding side.
- 👀 **looked at** — the modding side has read it; no verdict yet.
- ✅ **used / confirmed** — acted on, and it held.
- ❌ **dead end** — tried, and it did not work (kept so it is not re-proposed).

| Date | Topic | Status | Why it matters |
| --- | --- | --- | --- |
| 2026-09-29 | [The `openxr_loader.dll` string is almost certainly SDL3's unused XR module, not a VR path in the recompiler](topics/2026-09-29-the-openxr-loader-string-is-sdl3s-dormant-xr-module.md) | 🆕 | Answers the board's `[PD]` row: design starts beside ReXGlue's own D3D12 renderer; one static string check on our binary closes it. |
| 2026-09-23 | [Three VR mods on the same LithTech Jupiter EX engine, and where they hook the frame](topics/2026-09-23-three-vr-mods-on-the-same-lithtech-jupiter-ex-engine.md) | 🆕 | Nearest prior art (Condemned 1 VR); the frame order may survive the recompile, addresses will not. |
| 2026-09-17 | [Upstream check: Condemned2Recomp has been quiet since 2026-08-07; ReXGlue SDK nightlies continue](topics/2026-09-17-upstream-recomp-and-rexglue-nightlies.md) | 🆕 | Tells this project whether upstream has moved (it has not) and where fixes would come from |
