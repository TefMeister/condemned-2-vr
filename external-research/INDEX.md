# Research index

**Last `/gr` pass: 2026-11-14 — FULL.** Inbox empty. The "D:\Save access denied" start-up line is the game asking to write on the SDK's read-only disc mount (`d:` = the game files), not a broken save folder; real saves go to the user-data folder (Documents `condemned2recomp` for our 0.1.0 base, `UserData` beside the exe upstream since 2026-10-05). A second F.E.A.R. VR mod (TheFreeMike) does contact melee that leaves the hit to the game's own melee rules: prior art for step 4. Upstream still 0.6.0; since then SDK v0.10.1 and an input-driver overhaul. Two topics, one pointer in `engine-research/inbox/`.

_Previous: **Last `/gr` pass: 2026-11-07 (estate sweep) — CHECK-IN.** Inbox empty. Upstream is at 0.6.0 (2026-11-02: quick save/load, God mode, Starting Weapon replaces the spawner) and since 2026-11-04 builds on its own fork of the ReXGlue SDK; one topic, pointer sent to the dossier._

_Previous: **Last `/gr` pass: 2026-10-26 (estate sweep) — CHECK-IN.** Inbox empty. Upstream is at 0.4.0 (2026-10-24): no AVX2 needed since 0.3.1, a Cheat Manager and Weapon Spawner, 3D flickers fixed, and a native renderer planned that could move our headset patches' ground; one topic, pointer sent to the dossier._

_Previous: **Last `/gr` pass: 2026-10-04 (estate sweep) — CHECK-IN.** Inbox empty. Condemned2Recomp released 0.2.0/0.2.1 on 2026-09-29 (FOV, LOD and frame-cap settings, mouse look, DX12 performance fixes); we are on 0.1.0. Pointer sent to the dossier._

_Previous: **Last `/gr` pass: 2026-09-29 (estate sweep) — CHECK-IN.** Inbox empty. One topic answering the `openxr_loader.dll` row: the string is most likely SDL3's unused XR module bundled with the ReXGlue SDK, not a VR path; pointer dropped in `engine-research/inbox/`._

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
| 2026-11-14 | [The "D:\Save access denied" line is the game asking to write on the read-only disc mount; real saves live in the user-data folder](topics/2026-11-14-the-d-save-refusal-is-the-read-only-disc-mount.md) | 🆕 | Answers the Gold Digger note: probably harmless; one save-and-load in a flat run settles it. A rebase onto 0.5.x+ moves saves to `UserData` beside the exe (copy the old folder first). Also: upstream commits since 0.6.0. Pointer in `engine-research/inbox/`. |
| 2026-11-14 | [A second F.E.A.R. VR mod (TheFreeMike) does contact melee and leaves the hit to the game's own melee rules](topics/2026-11-14-fear-vr-by-thefreemike-contact-melee-on-the-sister-engine.md) | 🆕 | Prior art for physical melee (step 4) on the sister engine: fists and weapons strike physically, native collision and scripts decide; scripted breakables needed distinct strikes. |
| 2026-11-07 | [Upstream 0.5.0 to 0.6.0, and it now builds on its own fork of the ReXGlue SDK](topics/2026-11-07-upstream-0-5-to-0-6-and-its-own-sdk-fork.md) | 🆕 | A rebase onto 0.6.0+ needs psxrestore's `condemned2` SDK branch; unlock route is now Starting Weapon + upgrades + God mode; quick save/load for test spots. |
| 2026-10-26 | [Upstream moved fast in October: 0.3.x and 0.4.0 (cheats, no AVX2, a native renderer planned)](topics/2026-10-26-upstream-recomp-0-3-and-0-4.md) | 🆕 | Cheat Manager + Weapon Spawner = an unlock-everything route; 0.4.0 fixed 3D flickers (may be some of Tefa's headset visuals); a planned native renderer could move the ground our headset patches stand on. |
| 2026-09-29 | [The `openxr_loader.dll` string is almost certainly SDL3's unused XR module, not a VR path in the recompiler](topics/2026-09-29-the-openxr-loader-string-is-sdl3s-dormant-xr-module.md) | 🆕 | Answers the board's `[PD]` row: design starts beside ReXGlue's own D3D12 renderer; one static string check on our binary closes it. |
| 2026-09-23 | [Three VR mods on the same LithTech Jupiter EX engine, and where they hook the frame](topics/2026-09-23-three-vr-mods-on-the-same-lithtech-jupiter-ex-engine.md) | 🆕 | Nearest prior art (Condemned 1 VR); the frame order may survive the recompile, addresses will not. |
| 2026-09-17 | [Upstream check: Condemned2Recomp has been quiet since 2026-08-07; ReXGlue SDK nightlies continue](topics/2026-09-17-upstream-recomp-and-rexglue-nightlies.md) | 🆕 | Tells this project whether upstream has moved (it has not) and where fixes would come from |
