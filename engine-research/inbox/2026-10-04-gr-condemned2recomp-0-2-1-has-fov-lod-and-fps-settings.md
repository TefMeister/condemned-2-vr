# Condemned2Recomp 0.2.0 / 0.2.1 (2026-09-29): FOV, detail and frame-cap settings, mouse look, performance fixes

From: `/gr`, 2026-10-04. Our board and dossier are written against **0.1.0** (2026-08-05).

- **0.2.0** (2026-09-29) `[reported 2026-10-04, release notes]`: a first-time launcher (ISO and title-update
  extraction); a custom input driver (native mouse look, melee/firearm control swap); WIP graphics settings (object
  and shadow LODs, screen effects, **FOV**, **max fps**); "performance and stability fixes for DX12/Windows"; a warmed
  ROV/RTV cache.
- **0.2.1** hotfix the same day: user settings were being overwritten by defaults applied too early; ships an empty
  config toml to override 0.2.0's settings "fixing performance issues from ROV render target path"; the native
  resolution cvar is temporarily disabled.
- **Since then on `main`** (unreleased): settings apply as early as possible, an "Off" mode for shadow LOD, better
  object-shadow control (2026-10-01); a reworked custom native input driver (2026-10-03).
- **Why it matters here:** the home-PC frame-rate measurement and the windowed / low-graphics `[USER]` row were both
  done on 0.1.0. 0.2.1 adds the settings that row was improvising (FOV, LODs, a frame cap), and its DX12 performance
  fixes may change the measured frame rate. A native input driver also changes what our input automation has to
  drive.
- **Suggested:** move the home-PC build to 0.2.1 (or `main`) before the next frame-rate or windowed test, and re-read
  its config keys before writing our own toml; note that the native-resolution setting is off in 0.2.1.
- Source: <https://github.com/psxrestore/Condemned2Recomp/releases>.
