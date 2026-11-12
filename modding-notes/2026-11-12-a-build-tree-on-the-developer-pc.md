# 2026-11-12: a build tree on the Developer PC (step 1 of the hands-and-melee plan)

Opus. The tree was on PC2 (built there before the roles swapped); the Developer PC had only the built files.

- `D:/condemned-2-build/src/Condemned2Recomp`: upstream `psxrestore/Condemned2Recomp` at tag **0.1.0** (what our VR
  build sat on), plus this repo's `dev-archive/port-project/CMakeLists.txt` and `src/*`; `assets` a junction to the game
  copy `C:/NonSteam/condemned-2/app/Assets` (default.xex for the code generation).
- The SDK is **The Darkness's** (`D:/the-darkness-build/rexglue-sdk`, v0.10.0 + its patches). It is the same line as
  Condemned 2's own patch 09 (switch names `DK_XR`, `DK_SBS`, `DK_SBS_HOLD`; the SDK asks the game for `dk_xr_eye_fov`
  and `dk_xr_pair_sample`, which our code exports) and it adds the motion controllers (`rex_xr_sample_hands`,
  `rex_xr_sample_buttons`, patches 10 / 11).
- Build: `dev-archive/build-scripts/home_build_game.bat` (no wipe). First build: configure OK, code generation 15 s
  (263 files), build OK `[compile-verified 2026-11-12]`. `rexgpu-xenos.dll` comes from the SDK's `out/win-amd64`.
- Test copy `C:/NonSteam/condemned-2/build-test-c2-home` (the new exe, rexruntime, rexgpu-xenos; `Assets` a junction;
  `c2_vr_sim.ps1` = the headset launcher pointed at the simulator). On the simulator `[verified-live 2026-11-12, n=1]`:
  `[XR] hands ready: 5 of 5 controller profiles accepted`, runtime up, `game fov export yes, pair export yes`, session
  states 1-5, 600 frames with no draw errors, the opening film drawn. Closed by WM_CLOSE to the game's own window
  (CloseMainWindow picked the simulator's preview window, which the game process also owns).
- Next, step 2: the controllers as a pad (The Darkness's `vr_bridge.cpp` pad part: `rex_xr_sample_buttons`, a virtual
  pad device beside the real one, Quest B = pad X, X = pad B, menu held = START).
