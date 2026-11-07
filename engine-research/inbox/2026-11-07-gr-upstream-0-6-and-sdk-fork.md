# Upstream Condemned2Recomp is at 0.6.0 and now builds on its own SDK fork

From: `/gr`, 2026-11-07 (estate sweep, CHECK-IN). Topic:
`external-research/topics/2026-11-07-upstream-0-5-to-0-6-and-its-own-sdk-fork.md`.

- **SDK fork:** since 2026-11-04 upstream's submodule is `psxrestore/rexglue-sdk`, branch `condemned2`, not
  `rexglue/rexglue-sdk` v0.10.0. Rebasing our VR patches onto 0.6.0+ means building against that fork and
  carrying our SDK-side headset patches onto it `[reported 2026-11-07, GitHub API]` / `[inferred-static]`.
- **Unlock route changed:** 0.5.1 replaced the Weapon Spawner with a "Starting Weapon" cheat; 0.6.0 adds God
  mode and puts the Level 3 Taser in "all upgrades unlock" `[reported 2026-11-07, release notes]`.
- **Quick save / quick load** (0.6.0 PC menu): repeatable headset test spots `[reported]`.
- **Vsync on by default** since 0.5.0: keep in mind for frame-pacing comparisons `[reported]`.
- Main SDK released v0.10.1 on 2026-11-06 `[reported]`.

**Suggested dossier change:** in the upstream section, after the 0.3.x/0.4.0 paragraph from 2026-10-26, one line
on 0.5.x/0.6.0 and the SDK fork; on the board's rebase idea (if a row is made when the project resumes), name the
fork as the SDK to build against.
