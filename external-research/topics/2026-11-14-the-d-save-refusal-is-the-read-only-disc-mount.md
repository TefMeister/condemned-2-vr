# The "D:\Save access denied" line is the game asking to write on the read-only disc mount; real saves live in the user-data folder

**Status:** 🆕 new · **Priority:** medium (answers the Gold Digger note of 2026-11-14 in `engine-research/inbox/`).

## The question

The recomp's log at start, 2026-11-12: `[NtCreateFile] FAILED: path='D:\Save' -> 0xc0000022` (access denied, not
"not found"). Does saving silently fail?

## What is public

Read in the GitHub viewer / API, 2026-11-14, nothing downloaded:

1. **In the ReXGlue SDK, the guest drive `d:` is the game's disc files, mounted read-only.** The runtime mounts the
   game-data folder (our `Assets`) as the guest hard-disk partition and registers both `game:` and `d:` as links to
   it; the mount is read-only unless the developer switch `allow_game_relative_writes` is on (default off; its own
   description says it is "not useful to non-developers", for making test data) `[inferred-static 2026-11-14,
   rexglue-sdk src/system/runtime.cpp + src/filesystem/virtual_file_system.cpp]`. That file has not changed since
   2026-08-12, so the v0.10.0 SDK copy our build uses maps it the same way `[inferred-static]`.
2. So a create of `D:\Save` hits a read-only device, and "access denied" is the expected answer, not a broken
   folder or a locked file on our disk `[inferred-static]`.
3. **Where the saves really go: the user-data folder.** Condemned2Recomp sets that folder in its own settings code.
   Since commit `a7da7a7` (2026-10-05, "UserData now saves in same dir as Exe") it is `UserData` beside the exe;
   before that it was the SDK default, a `condemned2recomp` folder under the user's Documents
   `[inferred-static 2026-11-14, src/condemned2_settings.cpp + its history]`. A player confirms the old place:
   after updating to 0.5.1 their progress was gone until they copied the Documents `condemned2recomp` folder into
   the new `UserData` folder (issue #63) `[reported 2026-10-31]`.
4. Our build is upstream **0.1.0** (2026-09), older than that commit, so its saves should be in the Documents
   `condemned2recomp` folder, not beside the exe `[inferred-static]`. ⚠️ A later rebase onto 0.5.x/0.6.x moves the
   save folder to `UserData` beside the exe: copy the old folder across first, or the progress looks lost.

## What it does NOT settle

- Whether the game itself treats the `D:\Save` refusal as fatal for its save path, or just probes it (a leftover
  development path) and then uses the content/save device. Nothing public says `[hypothesis: harmless probe]`.
- The test that separates the two: one save (or a checkpoint) and one load in a flat run, then look for files with
  a fresh time in the Documents `condemned2recomp` folder. If they appear and the load works, the line is noise.
- Do not turn on `allow_game_relative_writes` to "fix" it: it would let the game write into the disc files.

## Upstream since the 2026-11-07 pass (no new release; still 0.6.0)

Commits 2026-11-08 to 2026-11-13 `[reported 2026-11-14, GitHub API]`: the game now builds on ReXGlue **v0.10.1**;
the `unlock_framerate` hook was removed; an "IsInMenu" check for input; native mouse sensitivity (separate for
tuning and aiming); **"input: driver overhaul"** (fixed the player Use input, ActionID names, slower walk speed for
stealth, fixed the Flashlight Intensity and Level 3 Taser cheats); a launcher layout in progress. Nothing about VR,
the arms or melee. Open issues include weapons that cannot be picked up and odd frame rates (#73).

## Sources

- psxrestore and contributors, Condemned2Recomp: settings code, commits, issues #51, #62, #63, #73 —
  <https://github.com/psxrestore/Condemned2Recomp>
- GuidoBisocoli, issue #63 ("Lost my progress", the save-folder move) —
  <https://github.com/psxrestore/Condemned2Recomp/issues/63>
- ReXGlue SDK developers, runtime and virtual file system source — <https://github.com/rexglue/rexglue-sdk>
