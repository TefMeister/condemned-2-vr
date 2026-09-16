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
