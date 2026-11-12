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

## Step 2: the controllers as a pad (b002, installed in `build-local` the same evening)

`dev-archive/port-project/src/vr_xr_pad.cpp` (+ `condemned2recomp_app.h`: the input factory) adds one more pad device fed
by the SDK's `rex_xr_sample_buttons`; the real pad keeps working beside it. Quest A = A, Y = Y, B = pad X (reload),
X = pad B (flashlight) (`C2_XR_SWAP_BX=0` = as printed), triggers, sticks, stick clicks; left grip = LB (run); right
grip = nothing (RB throws the weapon; `C2_XR_RGRIP_RB=1` gives it anyway); left menu held 0.6 s = START, tapped = BACK.
On with `DK_XR=1` unless `C2_XR_PAD=0`.

Simulator, pretend controllers only `[verified-live 2026-11-12, n=1]`: `[C2XR] controller pad on`, START x5 to the main
menu (Single Player / Multiplayer / **Instant Action** / Bloodshot Fight Club / Options / Extras), A, A = Continue game,
Rock Bottom loaded; left stick walked (picture change 1.13 against 0.28 standing); Quest X lit the flashlight
(brightness 3.9 -> 66.2), the plank in the right hand. Builds store started: b001 = the build worn on 2026-10-25, b002 =
this one (old files also in `build-local/_backup-2026-11-12-before-controller-pad`).

Next, step 3 (`MODEL: FABLE`): how the game places the first-person arms and the weapon.
