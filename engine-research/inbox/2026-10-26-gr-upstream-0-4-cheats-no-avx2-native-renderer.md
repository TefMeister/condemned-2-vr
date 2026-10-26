# Upstream Condemned2Recomp is at 0.4.0: cheats, no AVX2, a native renderer planned

From: `/gr`, 2026-10-26 (estate sweep, CHECK-IN). Topic: `external-research/topics/2026-10-26-upstream-recomp-0-3-and-0-4.md`.

**Dead end it answers:** dossier §2, "The **published 0.1.0 binaries require AVX2** (Haswell, 2013+)". Release
0.3.1 (2026-10-17) says it "no longer requires CPUs with AVX2" `[reported 2026-10-26, release notes]`.

**Also relevant to the board:**

- **Unlock everything:** 0.3.1 adds a Cheat Manager (all levels, upgrades) and 0.4.0 a full Weapon Spawner.
- **The [VR USER] "visual things" row:** 0.4.0 "resolved visual flickers in 3D rendering"; some of what Tefa saw
  may be upstream bugs already fixed `[hypothesis]`.
- **Risk:** "Native Renderer" is listed as in the works. Our headset patches sit in the SDK's Xenos GPU path; a
  native renderer would mean re-doing them there `[hypothesis]`. Upstream also has a self-updater (switch off
  in 0.3.3+ settings) that must not run over our patched files.

**Suggested dossier change:** next to the §2 AVX2 line, add "0.1.0 only; 0.3.1+ releases run without AVX2". In
the upstream section after the 0.2.x note, add one paragraph on 0.3.x/0.4.0 and the native-renderer risk, and
consider a [PD] row: rebase our patches onto 0.4.0 on a branch and compare a flat run before moving.
