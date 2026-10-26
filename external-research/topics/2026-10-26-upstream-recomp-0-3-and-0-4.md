# Upstream Condemned2Recomp moved fast in October: 0.3.x and 0.4.0 (cheats, no AVX2, a native renderer planned)

**Status:** 🆕 new · **Priority:** medium. Our VR build sits on 0.1.0 game code; upstream is now nine
releases ahead, and one planned change could move the ground our headset patches stand on.

## What is public

Read from the release notes and commit list on GitHub (API), 2026-10-26 `[reported 2026-10-26, release notes]`:

- **0.3.1 (2026-10-17)**, the big one:
  - **"No longer requires CPUs with AVX2."** The prebuilt release should now start on the dev PC's
    Ivy Bridge CPU, which the 0.1.0 release could not.
  - **WIP Cheat Manager**: unlock all levels, certain upgrades, spawn weapons, full accuracy.
  - **Native input driver** replacing the SDK's mouse and keyboard driver.
  - **Full game extractor** (QuickBMS unpacks the `Arch02` archives "for better performance and modding").
  - **Self-updater** that checks the repo at boot, downloads and restarts (0.3.3 adds a switch to turn it off).
  - "Screen effects are properly hooked"; "FOV is scaled by multiplier now"; more DX12 performance and
    stability fixes; WIP multiplayer (boots, no networking).
- **0.3.2 to 0.3.7 (2026-10-17/18)**: combo-attack bind, thrown items no longer vanish, vsync off and a higher
  refresh rate, a `minframerate` option (60 up to 144 fps), UI flicker fixed.
  **"Native Renderer: this is in the works, currently!"** is listed as a planned feature.
- **0.4.0 (2026-10-24)**: "RexCRT" (less memory instability), a PC menu on **F1**, the **Weapon Spawner fully
  implemented** in cheats, improved 3D performance, a warmer shader cache, and **"resolved visual flickers in 3D
  rendering"**.
- Commits on 2026-10-25/26 (after 0.4.0) touch input hooks, vsync to cap FPS, asset reloading (a known issue in
  the Museum level) and `setjmp`/`longjmp`.
- Upstream still pins the ReXGlue SDK at **v0.10.0** as a submodule; the SDK's default branch has no commits
  after its 2026-10-02 nightly `[reported 2026-10-26, GitHub API]`.
- No issue or discussion in the repo mentions VR `[reported 2026-10-26, n=1 search]`. No other Condemned 2 VR
  mod was found.

## Why it matters here

1. **Unlock-everything rule.** The Cheat Manager (levels, upgrades) and the Weapon Spawner are a ready route to
   "every weapon from the start" for testing, if our build moves onto upstream's newer game code. They are
   upstream's own code, so they would come with it rather than being something we copy.
2. **The visual things Tefa saw in the headset on 2026-10-25.** Some of them may simply be the 3D flickers
   upstream fixed in 0.4.0 `[hypothesis]`. Worth checking before writing any fix of our own.
3. **Screen-effects rule.** "Screen effects properly hooked" (0.3.1) sits on the same `CScreenEffectFX` /
   `EnableScreenEffects` system the dossier already maps; upstream's option may now switch them off cleanly.
4. **Risk: the planned native renderer.** Our headset output and per-eye view are SDK patches in the Xenos GPU
   path (the presenter and the set-view hook). If upstream replaces that path with a native renderer, those
   patches would need re-doing on whatever replaces it `[hypothesis]`. Until then, staying on our pinned build is
   safe; just do not take an upstream update blind.
5. **AVX2 dead end.** The dossier's §2 line that the published binaries need AVX2 is now true of 0.1.0 only.
6. **Self-updater.** If an upstream release is ever installed next to our build, turn auto-update off first
   (0.3.3 setting) so it cannot replace our patched program files.

## Next step it unlocks

Rebase our VR patches onto upstream 0.4.0 (or a later tag) on a branch, rebuild, and compare a flat run against
our 0.1.0-based build: same view, flickers gone or not, cheats reachable. Only then decide whether to move.

## Sources

- psxrestore, Condemned2Recomp releases and commits: <https://github.com/psxrestore/Condemned2Recomp/releases>
- ReXGlue SDK: <https://github.com/rexglue/rexglue-sdk>
