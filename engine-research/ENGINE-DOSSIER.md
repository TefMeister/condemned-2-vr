# Engine dossier — Condemned 2: Bloodshot (via ReXGlue static recompilation)

What we actually know about what we are modding, and how well we know it. Confidence tags
follow the house vocabulary: `[verified-live]`, `[measured]`, `[verified-numerically]`,
`[compile-verified]`, `[inferred-static]`, `[reported]`, `[hypothesis]`, `[disproved]`.

## 1. What we are actually running

Condemned 2 is a 2008 Xbox 360 / PS3 title by Monolith, published by SEGA. **There has never
been a PC version.** What runs on PC is
[Condemned2Recomp](https://github.com/psxrestore/Condemned2Recomp): the original PowerPC
executable is **statically recompiled** into native x64 C++ by the
[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk), then compiled as an ordinary Windows
program. It is **not** emulation — there is no interpreter or JIT in the hot path.

This matters more than it sounds. Every other flat-to-VR project on this account starts from a
sealed retail binary. Here **the entire program is rebuilt from source we can read and
change**, so stereo rendering can be added at the source level rather than injected.

- Disc: **Xbox 360 XGD2**, serial `SE-2031`, xemid `SE203101W0X11`, NTSC.
  `default.xex` build stamp **2008-02-17 21:36:01 UTC** `[verified-live 2026-09-16, n=1]`.
- Codegen translated the whole `default.xex` into **263 C++ files in 29.7 s**
  `[verified-live 2026-09-16, n=1]`. ⚠️ Expected to be slow and fragile; it was neither.

## 2. Renderer

- **D3D12, and only D3D12 in this build.** `condemned2recomp.toml` exposes
  `render_target_path_d3d12 = "rov"` (rasterizer-ordered views), `resolution_scale`,
  `native_2x_msaa`, `anisotropic_override`, `vsync` `[verified-live 2026-09-16, n=1]`.
- The runtime carries `rex::ui::d3d12::D3D12Presenter`,
  `rex::graphics::d3d12::D3D12CommandProcessor::IssueSwap`, `PipelineCache`,
  `GuestOutputRefreshContext` `[verified-live 2026-09-16]`.
- **The SDK also has a Vulkan backend** (`include/rex/ui/vulkan/…`) not used by this build
  `[verified-live 2026-09-16]`. Relevant because OpenXR supports either.
- **The design is visibly Xenia-derived** — `CommandProcessor`, `PipelineCache`, `GuestOutput`,
  and a GPU module named `xenos` `[inferred-static 2026-09-16]`. Not confirmed against ReXGlue's
  own history, but if it holds, Xenia's documentation applies to the renderer.

## 3. Where stereo would go — the seam

`D3D12Presenter` / `D3D12CommandProcessor::IssueSwap` is where one finished guest frame is
handed to the swapchain. That is the point a second eye would be introduced.
`[hypothesis]` — identified from symbol names only, **not yet read in source.**

⚠️ **No VR anywhere in the SDK's own source:** zero code hits for `openxr`, no VR file in the
repository tree, no VR issue or pull request `[verified-live 2026-09-16]`. The apparent "vr"
matches are PowerPC vector-instruction tests (`instr_lvr.s`, `instr_vrlh.s`).

⚠️ **One unexplained loose end:** `rexruntime.dll` contains the literal string
`openxr_loader.dll`, alongside `renderdoc.dll`, `steam_api64.dll` and `gameinput.dll`
`[verified-live 2026-09-16]`. None is a static import and the SDK source contains no such
string, so it most likely comes from a bundled third-party library probing for optional
modules. **Not traced.** `[hypothesis]` — worth two minutes before building a VR plan on
"this is greenfield", because if a dependency already has an OpenXR path, the work starts
somewhere else.

## 4. CPU requirements — the thing that nearly stopped the project

- The **published 0.1.0 binaries require AVX2** (Haswell, 2013+). On an Ivy Bridge i7-3770S
  they die instantly with `STATUS_ILLEGAL_INSTRUCTION` at a `vpbroadcastq ymm0, xmm0`, behind
  a misleading `0xc0000142` "unable to start correctly" dialog
  `[verified-numerically 2026-09-16]`.
- **Built from source at the SDK's own `-march=x86-64-v2` baseline, the game runs correctly on
  that CPU** `[verified-live 2026-09-16, n=1]`. AVX2 instruction count across all three
  binaries drops **6,214 → 39**, and the remaining 39 are never reached in practice.
- ⚠️ The SDK needs *enough* SIMD as well as *not too much*: below `x86-64-v2` it fails on
  `_mm_shuffle_epi8` requiring SSSE3. The window is **SSSE3/SSE4.2 yes, AVX2 no**.
- Expect this wall on **any** ReXGlue recomp tried on a pre-2013 CPU, not just this game.

Full evidence, measurements and the fixes: [`../patches/UPSTREAM-REPORT.md`](../patches/UPSTREAM-REPORT.md).

## 5. Input

- Gamepad is the supported path; keyboard and mouse are described upstream as work-in-progress
  `[reported]`.
- The shipped config is more complete than that suggests: `mnk_mode`, `mnk_sensitivity`,
  `mnk_smoothing`, `mnk_acceleration_exponent`, `mnk_decay`, `mnk_deadzone_compensation`,
  `mnk_invert_y` and a full keybind block are all present `[verified-live 2026-09-16, n=1]`.
  `input_backend = 'sdl'`.

## 6. Dead ends and corrections

- ⛔ **Building with LLVM-MinGW on Windows does not work** `[disproved 2026-09-16]`. The SDK
  uses Microsoft **Structured Exception Handling** (`__try`/`__except`) in five files, including
  `include/rex/platform/exceptions.h` and two `src/codegen/` files — that is the guest
  memory-fault machinery, not incidental — and `d3d12_api.h` needs `DXProgrammableCapture.h`
  from the Windows SDK. The working configuration is **clang targeting the MSVC ABI**
  (`-DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc`) inside a `vcvars64.bat` environment,
  which is what the upstream README means by needing both VS2022 and LLVM/Clang.
- ⛔ The SDK having `if(NOT MSVC)` branches does **not** imply MinGW support on Windows
  `[disproved 2026-09-16]` — those branches exist for Linux and macOS.

## 7. The camera seam, live on the dev PC (2026-10-12, `/lm`)

- **The game runs on the dev PC from our own build, rebuilt today on the shared SDK** (now carrying The Darkness's side-by-side patches 07/08/08b/08c, all off unless switched on): logo, film, menus, mission select, Rock Bottom's opening and gameplay `[verified-live 2026-10-12, n=4 launches]`.
- **`sub_8212BAB0` (set view) is the seam, as the reader found.** The probe logs every view; the player view is mode 0, view block 0x82925958, 1280x720, fov tangents (1.2448, 0.7002), near 0.01; all 3D views arrive through one return address, 0x8212B854, so the caller cannot be told apart by lr `[verified-live 2026-10-12, n=1]`. Positions are around (7900, 3500, 6700), consistent with centimetres `[hypothesis]`.
- **The eye shift works and reaches the screen.** With `DK_SBS=1 C2_CAM_EYE=<e> C2_STEREO_PERIOD=1`, the halves show the same alley from two viewpoints: about 2 px apart at e=5 and about 17 px (by eye) at e=40 `[verified-live 2026-10-12, n=2 launches]`. Screen-space UI (the tutorial box) is identical in both halves, as it should be. Whether the two halves are always the SAME moment (The Darkness's two-thread problem) is not yet checked here.
- **Heavy blur in play** (motion blur / depth of field / damage FX) makes the halves soft; the screen-effects switches below are the next step.
- **Music:** `"MusicEnable" "0"` added to `Assets/autoexec.cfg` (backup beside it, CRLF kept) `[verified-numerically 2026-10-12]`; not yet heard. The in-game slider was also set to 0, but without a signed-in profile it does not save.
- **Driving:** a pad that disappears pauses the game; the first press after a new pad is sometimes lost; short presses are dropped at this PC's frame rate (hold ~0.35 s); a slider needs a long hold. Use one long-lived pad process (`recon/2026-10-12-camera-seam-live/c2_padd.py`). Route: START (skip film), START, START (title), B (no sign-in), START (copyright), A (brightness), A (Single Player), A (Rock Bottom), A (difficulty), ~4 min to the 3D level, B (tutorial box).

- **The screen-effects switch works** (`C2_VR_SCREEN_FX_OFF=1`, `src/vr_screenfx_music.cpp`): same alley, same eye shift, edge sharpness 0.66 → 2.08 and the blur is gone by eye `[verified-live 2026-10-12, n=1]`. Log: `EnableScreenEffects`/`EnableDamageFX`/`CamDamage` defaults 1 → 0, `HeadBob` 1 → 0, `MotionBlur_Enable` 1 → 0. Off unless switched, so flat play is unchanged. Evidence `recon/2026-10-12-camera-seam-live/screen-fx-off-*`.
- **Probably two moments per pair, as on The Darkness:** in a sharp pair the walking figure stands in a different pose and ~70 px apart between halves, more than the ~17 px eye shift `[hypothesis, by eye n=1]`. The reader is mapping the threads and clock for a freeze.

- ⭐ **Same moment in both eyes, live** (`C2_EYE_SYNC=1` + `DK_SBS_HOLD=1`, `src/vr_eye_sync.cpp`): `[C2VR] SYNC L=3900 R=3899, skipped_vdswap=0, keepalive_swaps=0, sdk_lookup_miss=0`; `FREEZE pair_dt mean=0.000ms max=0.000ms`, capped=2; by eye the figure at the bin holds the same pose in both halves while the viewpoint shifts, and the shift has the right sign (the right eye sees near things further left) `[verified-live 2026-10-12, n=1 run, 3 pictures]`. Smoke and snow differ slightly between halves (other clocks) `[hypothesis]`. The earlier "probably two moments" line is answered by this. Evidence `recon/2026-10-12-camera-seam-live/eye-sync-*`.

### From the reader: same moment (threads, clocks, design)

From: `/lm` reader (static only; nothing launched). Code: `staging/condemned-2-vr/same-moment-2026-10-12/`.
Tools: The Darkness's `idx.py`/`callers.py`/`fn.py`/`xref.py` pointed at Condemned's generated code,
`img.py` with `off(a) = a - 0x82000000` on a fresh `xex_unpack.py` image.

## Short answer

Mostly no. No game/render packet queue was found; the frame, the views and FlipScreen run on one
thread. But two things can still put the wrong picture in a half, and the world is not yet held
still for the second eye.

## Threads

- **FlipScreen and every logged set-view ran on the same host thread** in all three camera-seam runs
  (`flip thread=` hash equals every `view ... thread=` hash) `[verified-live 2026-10-12, n=3 runs]`.
  ⚠️ Sampled: only the first 3 calls of each view kind are logged with a thread.
- **The main loop is single-threaded:** `xstart -> sub_82178E90 -> sub_8217CEE8`, which loops
  `while (!sub_821802A8(*(0x829277B0))) {}`. `sub_821802A8` is the whole frame: clocks, client/server
  update, render (through interface vtables, so not visible as direct calls) and, last, a frame
  limiter `[inferred-static 2026-10-12]`.
- **No render-command queue:** no queue/render-thread strings, no render-thread class in RTTI (only
  the generic `CXenon_LTThread`, Havok worker threads `PhysicsClientThreads/ServerThreads`, a resource
  loader) `[inferred-static]`. Set-view builds every matrix (`sub_8212B010`) on the calling thread,
  so the eye shift is fixed when the view is set, not when a packet is played `[inferred-static]`.
- That FlipScreen is called INSIDE `sub_821802A8` is not proven statically (vtable calls)
  `[hypothesis]`; the new probe counts `flips_in_frame` / `flips_outside_frame`.

## Correction: `sub_82671908` is a thread, not "present while waiting"

- It is a **thread entry**: D3D device create `sub_8265EA28 -> sub_8266ABF8 -> sub_82671E10` calls
  CreateThread (`sub_826505C0`) with start address 0x82671908 `[inferred-static 2026-10-12]`.
  Supersedes: 2026-10-12-lm-reader-camera-seam, "Frame end" bullet 2 (it said "reached only through a
  function pointer: presents while the game waits on a lock/event").
- It waits on an event with a 30 ms timeout (-300000 x 100 ns); on timeout (258) it re-presents the
  stored front buffer (`device+14844`) through `sub_8265F2B0` if `device+10942` bit 0x02 is set
  `[inferred-static]`. Who sets that bit is not found. Not a new picture: the Darkness "filler" case.
- **Every VdSwap goes through `sub_8265F2B0`** (the only `__imp__VdSwap` call), from exactly two
  places: FlipScreen, return address **0x82122F00**, and the keep-alive thread, **0x826719F4**
  `[verified-numerically 2026-10-12]` (the `ctx.lr = ...` constants in the generated code).
- `sub_8265F2B0` **skips VdSwap when `device+21532 != 0`** (same guard as The Darkness's `+21508`)
  `[inferred-static]`. No direct store to it was found.
- Consequence: patch 07's swap-parity rule slips by one for good after any keep-alive or skipped swap,
  putting one eye on both halves `[hypothesis]` until a run counts them.

## Clocks the game reads

- `sub_82650F50` = **QueryPerformanceCounter(LARGE_INTEGER\*)** (mftb, re-read if the low word is 0);
  5 callers, chiefly the wrapper `sub_822CE2D0` (returns the 64-bit count, 14 callers, including the
  frame start in `sub_821802A8`) `[inferred-static 2026-10-12]`.
- `sub_82650F40` = **GetTickCount** (KeTimeStampBundle +16); it is the ONLY reader of the bundle
  pointer at 0x82000830 `[inferred-static]`. Wrappers: `sub_822CE290` (ms since a base at 0x829326E0,
  55 callers, mostly engine/network code 0x821D-0x821F) and `sub_822CE230` (seconds, 11 callers,
  including the object interpolation `sub_821DCD88`, which clamps a negative step to 0).
- Raw `mftb` also sits inline in 215 functions, nearly all D3D/library (0x825-0x828: fences,
  profiling) plus a renderer profiler `sub_8212C488`; not game time `[inferred-static]`.
- **Frame limiter**, end of `sub_821802A8`: if the float at 0x8292864C (likely `MaxFPS`) is non-zero,
  it loops `Sleep(1)` (`sub_822CE208`, return address **0x821806D4**) until `sub_822CE230` says the
  frame is long enough. A pinned clock there would spin until the cap: it must be exempt
  `[inferred-static]`. Also a configurable Sleep at the frame START (0x82928688, lr 0x82180368).
- The resource loader `sub_821AFD48` (3 ms budget, QPC) runs inside the frame; with a pinned clock it
  simply drains its queue `[inferred-static]`.
- Strings `MaxFPS`, `TimeScale`, `SimulationTimeScale`, `PlayerTimeScale`, `MinFrameRate` exist.

## Design (written, `C2_EYE_SYNC=1`)

- Eye = parity of the count of FlipScreen presents that really reached VdSwap; a skipped present
  redraws the same eye.
- Every VdSwap is labelled before it is issued, in a 1024-slot table read by SDK patch 08 through the
  exported `dk_sbs_swap_eye` (same name and encoding as The Darkness): FlipScreen swaps carry the eye
  and sequence, keep-alive swaps carry -4 (repeat the pair, patch 08c).
- Freeze (period 1): the right-eye frame sees both clocks as they were at the first read of its
  left-eye frame, +1 tick (20 ns) per read; window = from the right frame's entry into
  `sub_821802A8` to its present, so the limiter never runs pinned (`C2_FREEZE_FROM=present` is the
  fallback, with the limiter Sleep releasing the clock). Only the frame thread is pinned; 250 ms cap.
- `[compile-verified 2026-10-12]` (syntax only, llvm-mingw, mingw target). Not built, not run.

## Open

- Whether the simulation reads time only on the frame thread (`other-threads qpc= tick=` in the log).
- Whether anything advances per frame regardless of time (a frame counter, a minimum step), which a
  clock freeze cannot hold `[hypothesis]`; the same open item as The Darkness's cause 4.
- What sets the keep-alive bit, and whether keep-alive swaps happen in gameplay at all.

### From the reader: 2026-10-12-lm-reader-camera-seam

From: `/lm` reader (static only; nothing launched). Code: `staging/condemned-2-vr/camera-seam-2026-10-12/`.

## How it was read

- `default.xex` decrypts with The Darkness's `dev-archive/tools/xex_unpack.py` (retail key, basic
  compression). ⚠️ The output is a **memory image** (guest A = file offset A - 0x82000000), not a file
  layout: `.text` sits at file 0x110000 where the PE header says raw 0x108600 `[verified-numerically 2026-10-12]`.
  The Darkness's `img.py` maps through the PE raw offsets and reads `.data`/`.text` wrongly on this image;
  patch `off()` to `a - 0x82000000` before reuse.
- The Darkness's `idx.py`/`callers.py`/`xref.py`/`fn.py` work on Condemned's generated code once
  `GEN` points at it: 39,803 functions in 130 files.

## The engine names itself

Interface instances with names (`.data` 0x829258CC on): `ILTRenderer.Default` (vtable 0x82091E28),
`ILTCustomRender`, `ILTVideoTexture`, `ILTTextureMgr`, `ILTDrawPrim.Default` (class `CD3DDrawPrim`,
vtable 0x82092278). MSVC RTTI is present (`CPlayerCamera`, `PlayerDamageFXMgr`, ...). This is
Monolith's LithTech Jupiter EX (the F.E.A.R. engine) `[inferred-static]`.

## Frame end

- **`sub_82122EA8` = ILTRenderer::FlipScreen** (vtable slot 9). Returns 0 unless the render state
  0x82925950 == 1; else calls `sub_8265F2A8` and **`sub_8265F2B0`** (the D3D swap), which calls
  **VdSwap at 0x8265F4FC** (lr 0x8265F500). Front buffers at 0x82970BA0+448, index at +456, flipped
  after each present `[inferred-static 2026-10-12]`.
- The only other `sub_8265F2B0` caller is `sub_82671908`, reached only through a function pointer:
  it presents while the game waits on a lock/event (loading) `[inferred-static]`. Not a new picture.

## The view build: one function every 3D view passes through

- **`sub_8212BAB0` "set view"** (4 call sites, all through the weak symbol; `REX_HOOK_RAW` works):
  r3 = render state RS (0x82925950), r4 = destination view block V, **r5 = camera transform (pos xyz
  + quaternion xyzw, 7 floats)**, r6 = FOV pair (-> V+48/52, used as 1/x in P, so tan of half-angles),
  r7 = 2 floats -> V+56/60 (projection offset), r8 = viewport rect (-> V+4..16), r9 = mode (-> V+0:
  0 perspective, 1 ortho, 2 screen), f1/f2 = near/far (-> V+64/68). Normalises the quaternion, then
  calls `sub_8212B010` (its only caller) `[inferred-static 2026-10-12]`.
- **`sub_8212B010` builds the matrices** into V: +72 world->camera (3x4), +120 projection, +184
  world->clip, +248 world->screen, +312 a 3x4 screen matrix `[inferred-static]`.
- **The current view is V = RS+8 = 0x82925958.** `sub_821474C8` (a save / render / restore pass)
  also writes RS+416 = 0x82925AF0 `[inferred-static]`.
- **The quaternion is the camera orientation:** `sub_8212B7F0` uploads q*(0,0,1) as the shader
  constant `k_vWorldSpaceCameraDir` (and writes `k_fScene_NearZ`/`FarZ` from V+64/68). So
  **right = q*(1,0,0) = (1-2(y²+z²), 2(xy+zw), 2(xz-yw))** `[inferred-static]`.
- Callers of the view set: ILTRenderer slot 14 `sub_82123090` (transform, FOV, rect; near/far from
  globals at 0x82925E14/0x82925DFC; needs state == 3) and slot 15 `sub_82123170`, slot 16 via
  `sub_8212B700`, slot 28 `sub_82123538`, CD3DDrawPrim slots 59/60, a post-process chain
  (`sub_8212F160` -> `sub_8212F538`) and `sub_821474C8`. **Which caller is the player's camera is not
  found statically** `[hypothesis]`; the probe logs every distinct one with its lr.
- Shader constants are named objects in `.data` (`k_mObjectToClip` at 0x82926140, data at +28;
  `k_mObjectToClip_Previous`, `k_vObjectSpaceEyePos`, ...), written host-side by
  `sub_8213F3C8`/`sub_82125A88` (ObjectToClip) `[inferred-static]`.

## Game side

- `CPlayerCamera` (RTTI), vtable 0x8201AD04. **`sub_82344590` = its per-frame update**: mode at
  +604 (0..6) picks one of `sub_82341F78`, `sub_8233FAC0`, `sub_8233FD88`, `sub_82342238`,
  `sub_82340820`, `sub_8233DFE8`; then `sub_8233CA88` (fog blend). `sub_82343C30` registers its
  console variables (CameraClipDist, ChaseCam*, ConsoleFog*, FovAspectRatioScale, CameraSway*)
  `[inferred-static]`. Not needed for the eye offset; useful later for head tracking.

## Hook plan (the DK_CAM_EYE equivalent)

`REX_HOOK_RAW(sub_8212BAB0)`: before the original, for V == 0x82925958 and mode 0, add
sign x e x right to the 3 position floats at r5; after the original, put them back (r5 may be the
camera object's own transform). Everything downstream (world->camera, culling planes, eye position
for lighting) then sees the shifted eye, which is the lesson from The Darkness. Flip the sign at
FlipScreen. Written as `vr_camera_probe.cpp` (`C2_CAM_EYE`, `C2_CAM_EYE_LR`, `C2_STEREO_PERIOD`)
`[compile-verified 2026-10-12]` (syntax only, mingw target).

Open, for the first run: how many view sets per frame and which lr is the player; whether set view
and FlipScreen run on the same thread (the probe logs both thread ids); the unit size (LithTech
Jupiter EX is usually 1 unit = 1 cm, so half an IPD would be about 3.2 `[hypothesis]`).

### From the reader: 2026-10-12-lm-reader-music-and-screen-fx

From: `/lm` reader. Nothing launched. Code: `staging/condemned-2-vr/music-and-screen-fx-2026-10-12/`.
All addresses are for the pre-title-update `default.xex` that our build uses. ⚠️ Upstream 0.2.0
switched to the title-update xex (`condemned2recomp_config_tu.toml`), so its addresses are about
0x90-0xC8 off from ours: upstream "EnableScreenEffects" is at 0x820249F0, ours at 0x82024A80.

## Console variables and autoexec.cfg

- At start-up the engine loads `d:\autoexec.cfg` ("config"), `d:\display.cfg` and `d:\settings.cfg`
  (`sub_8217CD78`). Our `Assets\autoexec.cfg` already holds `"Name" "value"` console variables
  `[inferred-static 2026-10-12]`.
- Games register their variables with `sub_82175B90(handle, client, name, string default, float
  default)`, which creates the variable **only if it does not already exist** (GetConsoleVariable
  first). So a value set in autoexec.cfg survives registration `[inferred-static]`.
- Helpers: `sub_826A7BC0(name, int default)` = get int, `sub_826A7C80` = get bool,
  `sub_826A7EF8(name, bool)` = set bool, `sub_826A7F90(name, float)` = set float `[inferred-static]`.

## Music

- Sound apply `sub_823C7578`: music = `MusicEnable` (console int, default 1) ? profile+60 : 0. It then
  calls `sub_822DD240(f1 effects, f2 music, f3 speech)`, the mixer push. Defaults come from the game
  database record `Profile/Sound` (`SoundVolume`/`MusicVolume`/`SpeechVolume` -> profile +56/+60/+64,
  loaded by `sub_823C79B0`) `[inferred-static 2026-10-12]`.
- **Permanent, with no sign-in:** `"MusicEnable" "0"` in `Assets\autoexec.cfg` `[inferred-static]`, not
  yet heard. Fallback hook: force f2 = 0 in `sub_822DD240` (`C2_MUSIC_OFF=1`).
- Signed-in profile: `Documents\condemned2recomp\B13EBABEBABEBABE\534507EF\00000001\Profile.ltp\checkpoint.sav`,
  604 bytes, big-endian. Floats at 0x38 / 0x3C / 0x40 = 0.75 / 0.75 / 1.0, which matches the menu's
  75 / 75 / 100 `[measured 2026-10-12]`. That they are effects / music / speech is `[inferred-static]`.
  No checksum seen. The file dates from 2026-09-16.
- The SDK toml has only `audio_mute`, which mutes everything (from the Darkness read; same SDK).

## Screen effects: what exists, and the switch for each

| Effect | Evidence | Switch | Confidence |
| --- | --- | --- | --- |
| **Full-screen effect system** (the 0.2.0 "Screen Effects" option) | `CScreenEffectFX`, DB `FX/ScreenEffect`, `ScreenEffectType`; console var `EnableScreenEffects` (default 1) registered by `sub_823F67D0` at handle 0x8293D4DC, read by `sub_823F5D00` (the effect's gate) | cfg `"EnableScreenEffects" "0"`; hook gives it a default of 0 | gate `[inferred-static]`; which effects it covers (hurt, focus, SCI) `[hypothesis]` |
| **Damage FX** | `EnableDamageFX` registered in `sub_82382C68` (0x8293B15C); `sub_82381DA8` clears all damage FX and returns when it is < 1. `DamageFxDB`, `TakingHealthFXName` | cfg `"EnableDamageFX" "0"`; hook default 0 | `[inferred-static]` |
| **Motion blur** | renderer reads `MotionBlur_Enable` via `sub_8212D328` in `sub_8212D5C8`, along with `MotionBlur_Scale` / `AfterImage` / `PassCount`; the level object `MotionBlurProperties` (`sub_824EFDE0`) sets it back on through `sub_826A7EF8` | cfg `"MotionBlur_Enable" "0"` + hook on `sub_826A7EF8` holding it at 0 | `[inferred-static]` |
| **Depth of field** | material float `k_fDepthOfField` (0x82017744) set every frame by `sub_82309E98` (player; strength = `DOFBlur` console var, lerped toward an aim value) and `sub_823044B0`, both through ILTRenderer slot 51 `sub_82123AA0` | hook: that parameter forced to 0 | `[inferred-static]` |
| **Camera kick on hit** | console var `CamDamage` (+ Pitch/Yaw/Roll/Time), registered in `sub_82310DA8`; read in `sub_8230BEA0` | cfg `"CamDamage" "0"`; hook default 0 | `[inferred-static]`; whether melee kicks (`CameraMeleeDamage*`) obey it `[hypothesis]` |
| **Head bob** | profile option written as `HeadBob` = value x 0.1 by `sub_823C7D18` via `sub_826A7F90`; `UseHeadBob`, `Client/Headbob` DB | hook sets 0 (`C2_KEEP_HEADBOB=1` to keep it) | `[inferred-static]`. Camera motion, not a picture effect: Tefa's call |
| Camera sway / idle shake | `CameraSway*` (CPlayerCamera console vars), `IdleShakePct`, `ZoomShake` | not switched yet; could be zeroed the same way | `[hypothesis]` |
| Letterbox bars (cutscenes) | `LetterBoxDisabled` console var (`sub_82351190`) | optional cfg `"LetterBoxDisabled" "1"` | `[inferred-static]` |
| Bloom / HDR glow | `HDR_Blur`, `HDR_Glow` (renderer, `sub_8212D3C0`), `HDR_Enable` | leave on unless it smears | `[inferred-static]` |
| Film grain, vignette, blood on the lens | `k_fNoiseIntensity` belongs to volumetric light (in the world); no screen-grain or lens-blood name found | probably inside the screen-effect or damage FX records | `[hypothesis]` |

Proof is a run with the cfg lines and/or `C2_VR_SCREEN_FX_OFF=1`: take a hit, aim, and walk through a
motion-blur zone, then compare against a run without them. The hook file logs every value it overrides.

## Inbox folds, 2026-09-29

**`/gs` 2026-09-17 on the OPEN block: already resolved.** The block was re-audited and re-dated on 2026-09-21, and `gate-scan.sh --check` no longer flags it (2026-09-29 sweep).

**The `openxr_loader.dll` string is probably SDL3's dormant XR module (`/gr` 2026-09-29).** ReXGlue has no OpenXR code of its own but bundles SDL3, whose GPU API has an OpenXR module that loads `openxr_loader.dll` by that name; ReXGlue uses SDL only for the window and draws with its own D3D12 renderer `[inferred-static]`. To close the board row: look for SDL's other XR strings beside it in `rexruntime.dll`. Topic: `external-research/topics/2026-09-29-the-openxr-loader-string-is-sdl3s-dormant-xr-module.md`.

## Inbox folds, 2026-10-12

**Condemned2Recomp 0.2.0 / 0.2.1 (from `/gr`, 2026-10-04).** From: `/gr`, 2026-10-04. Our board and dossier are written against **0.1.0** (2026-08-05).

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

**Dev PC, 2026-10-12 (`/lm`):** our own build (on 0.1.0 game code) still starts and plays the opening film on the dev PC `[verified-live 2026-10-12, n=1]`, which corrects the board's old Ivy Bridge row: only the official prebuilt release needs AVX2.
