# Upstream Condemned2Recomp 0.5.0 to 0.6.0, and it now builds on its own fork of the ReXGlue SDK

**Status:** 🆕 new · **Priority:** medium (project is paused; this matters the day it resumes). Follows the
2026-10-26 topic on 0.3.x/0.4.0.

## What is public

Read from the release notes, commit list and `.gitmodules` on GitHub (API), 2026-11-07
`[reported 2026-11-07, release notes + GitHub API]`:

- **0.5.0 (2026-10-26):** dynamic allocation in the asset manager, "fixing numerous issues, including broken
  cutscenes"; `setjmp`/`longjmp` added; **vsync on by default**; hook attack can be held.
- **0.5.1 (2026-10-27):** the Weapon Spawner cheat is **replaced by a "Starting Weapon" cheat** (the spawner
  interfered with assets); texture-caching settings removed.
- **0.6.0 (2026-11-02):** new launcher and in-game menu; **quick save, quick load and return-to-main-menu** in
  the PC menu; fixes for a crash on checkpoint reload / mission restart, a Mission 2 input break, a Mission 9
  cutscene trigger bug and a crash on losing the internet connection; **toggleable slow motion, a God mode
  cheat, flashlight intensity, Level 3 Taser added to "all upgrades unlock"**; game-language selection.
- **2026-11-04 commit "use separate fork of RexGlue-SDK":** the submodule now points at
  `psxrestore/rexglue-sdk`, branch `condemned2`, instead of `rexglue/rexglue-sdk` at v0.10.0.
- The main ReXGlue SDK released **v0.10.1 on 2026-11-06** (Xbox 360 input features, a movie fix for
  `vpkuwus`/`vpkuhus` packing, atomic `stwcx.` codegen).
- No VR mention in the release notes; no other Condemned 2 VR mod found (GitHub search) `[reported 2026-11-07,
  n=1 search]`.

## Why it matters here

1. **Our self-build recipe** (board, 2026-09-16) clones the main `rexglue/rexglue-sdk`. Upstream game code from
   2026-11-04 on expects the `condemned2` branch of psxrestore's fork, so a rebase of our VR patches onto 0.6.0+
   must build against that fork, and our SDK-side headset patches (presenter, set-view hook) would have to be
   carried onto it `[inferred-static]`.
2. **Unlock-everything rule:** the route changed shape. The Weapon Spawner is gone from 0.5.1; the testing route
   is now "Starting Weapon" + "all upgrades unlock" (now with the Level 3 Taser) + God mode `[reported]`.
3. **Testing comfort:** quick save / quick load make repeatable headset test spots possible without replaying a
   level; slow motion may help reading per-eye artefacts `[hypothesis]`.
4. **Screen effects / jitter:** vsync now defaults on (0.5.0); remember it when comparing frame pacing against
   our 0.1.0-based build.
5. The **SDK fault our compiler flagged** (`xam_info.cpp`, board [PD] row) may or may not exist in the fork; check
   there before reporting it upstream `[hypothesis]`.

## Sources

- psxrestore, Condemned2Recomp releases, commits and submodule list: <https://github.com/psxrestore/Condemned2Recomp>
- psxrestore, rexglue-sdk fork (`condemned2` branch): <https://github.com/psxrestore/rexglue-sdk>
- ReXGlue SDK v0.10.1 release notes and contributors: <https://github.com/rexglue/rexglue-sdk/releases>
