# 2026-10-15: each eye to the headset (OpenXR), and the head turns the camera (`/pd`, dev PC)

**The game was not launched. Nothing here has run inside the game.** What did run: a test program
that drives the new SDK code exactly as the game will, against the OpenXR simulator on the dev PC.

## What was built

1. **SDK patch 09** (`patches/09-rexglue-sdk-openxr-per-eye.patch`, applies on top of The Darkness's
   SDK patches 07–08c in `the-darkness-vr/dev-archive/sdk-patches/`). Switch: `DK_XR=1` (needs
   `DK_SBS=1`, best with `DK_SBS_HOLD=1`). New files `xr_output.{h,cpp}` (OpenXR session, one
   OpenXR frame per committed pair) and `xr_blit.{h,cpp}` (draws each eye texture into the headset's
   image: a draw, not a copy, because the game's output is 10:10:10:2 and headsets offer 8-bit sRGB).
   Hooked after the SDK's own submission in `IssueSwap`, so the eye textures are complete when read.
   The headset gets exactly the pairs the window shows; a picture with no eye (loading) goes to both
   eyes. `openxr_loader.dll` is loaded from beside the exe (Khronos loader 1.0.22 copied in).
2. **Game side** (`dev-archive/port-project/src/vr_head_pose.{h,cpp}`, `vr_head_math.h`, small hooks
   in `vr_camera_probe.cpp` and `vr_eye_sync.cpp`). Switch: `C2_XR_HEAD=1` (needs `C2_EYE_SYNC=1`).
   Once per pair, at the first player view, the head is sampled through the SDK's
   `rex_xr_sample_head`; both eyes of the pair use that sample; the player camera's quaternion is
   turned by it (head rotation in the camera's own frame), then the eye shift follows the turned
   right axis. The game tells the SDK which sample each pair used (`dk_xr_pair_sample`), so the
   headset receives the exact pose the picture was drawn with, and the field of view the player
   camera really uses (`dk_xr_eye_fov`, from set-view's FOV pair). Head movement is optional
   (`C2_XR_HEAD_POS=1`, `C2_XR_UNITS_PER_M`, default 100, still a guess).

## What is known, and how well

- Both builds compile with no errors (SDK + game, llvm-mingw, x86-64-v2) `[compile-verified 2026-10-15]`.
  Exports present: `rex_xr_sample_head` (rexgpu-xenos.dll), `dk_xr_pair_sample`, `dk_xr_eye_fov`
  (exe) `[verified-numerically 2026-10-15]` (export tables read).
- **The SDK code works against an OpenXR runtime**: test host `tests/dk_xr/xr_host_test.cpp` (in the
  patch) ran 240 and 1,500 frames on the simulator: session reached FOCUSED, 11 formats offered,
  sRGB 8-bit chosen (image typed, so gamma is undone before the sRGB view re-encodes it),
  `draw_errors=0`, every pair after the first found its head sample (1,499/1,500), paced at the
  headset's 90 Hz (`wait_ms_mean` 9.9). The simulator's screenshot showed left red, right blue: each
  eye lands on its own side `[verified-live 2026-10-15, n=2]`
  (simulator runs of the test program, not the game).
- **The head-turn maths** (`vr_head_math.h`, OpenXR right-handed -Z forward → LithTech left-handed
  +Z forward = flip X and Y of the quaternion axis) passed 3,003 checks against independently built
  rotations, including named cases: look left, look up, head tilt `[verified-numerically 2026-10-15]`.
  ⚠️ That the game's camera quaternion really is LithTech-standard (right = q·X, forward = q·Z) rests
  on the 2026-10-12 reader's static read plus the eye-shift having the right sign live
  `[verified-live 2026-10-12, n=1]`; it is not separately proven for rotation.

## Not established

- Nothing about the game with `DK_XR=1`. Risks to watch on the first run: (a) `xrWaitFrame` blocks
  inside the presenter's refresh callback on the GPU thread, which could stall the window or the game
  (fix: move OpenXR to its own thread); (b) the eye shift `C2_CAM_EYE` is in game units and is not
  yet tied to the headset's eye distance, so depth scale is a guess until the world unit is pinned;
  (c) the game draws ~102° wide; the headset shows its own narrower view of that picture (correct,
  but not yet the headset's own projection).
- A wrong turn direction would show as the game view turning the opposite way to the head: that
  means the axis conversion is wrong for this camera, not that the maths needs tuning.

## The one command for the first run (dev PC, simulator, no headset)

Environment for the game process only:
`DK_SBS=1 DK_SBS_HOLD=1 C2_EYE_SYNC=1 C2_CAM_EYE=5 C2_STEREO_PERIOD=1 C2_VR_SCREEN_FX_OFF=1 DK_XR=1 C2_XR_HEAD=1 XR_RUNTIME_JSON=D:\claude video game stuff\tools\OpenXR-Simulator\openxr_simulator.json`,
then reach Rock Bottom (route in dossier §7) and turn the simulator's head (its `set_head_pose`).
- Log `[XR] runtime up` and rising `frames=` with `sample_hits` ≈ frames → the hand-off works.
- Simulator shows the alley in both eyes, slightly shifted → the picture reaches the headset.
- Head yaw +30° turns the view LEFT in both eyes → head tracking is right. RIGHT → flip the conversion.
- `[XR] OFF: <reason>` → it says why. Game or window freezing → the blocking wait; move it to a thread.
