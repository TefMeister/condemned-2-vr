# The `openxr_loader.dll` string in `rexruntime.dll` is almost certainly SDL3's built-in XR module, not a VR path in the recompiler

**Found:** 2026-09-29, `/gr` estate sweep (CHECK-IN).
**Answers:** board `[PD]` row "trace the unexplained `openxr_loader.dll` string in `rexruntime.dll` before designing anything".

## What was found

1. **The ReXGlue SDK itself contains no OpenXR code.** Its full source tree (1,471 paths, tree listing
   not truncated) has no file or folder with `openxr`/`xr_` in its name, and none of its 22 git
   submodules is an OpenXR project. A GitHub code search for `openxr` inside the repo returns 0,
   while the same search for a known-present word (`simde`) returns 15, so the search could have
   found a positive `[reported]` (checked 2026-09-29 against the repo's `HEAD`, last pushed
   2026-09-25).
2. **One of those submodules is SDL (libsdl-org/SDL, i.e. SDL3), and SDL3 now ships OpenXR support
   inside its GPU API.** SDL's own `docs/README-xr.md` describes creating an XR-enabled GPU device
   (Vulkan, D3D12, Metal back ends) that manages the OpenXR instance, session and swapchains for
   the app. SDL's loader shim `src/gpu/xr/SDL_openxrdyn.c` holds a table of library names to load
   at run time, and on Windows that name is literally `openxr_loader.dll` `[reported]`.
3. **ReXGlue uses SDL only for its window, not for drawing.** Code search finds SDL window
   creation in `src/ui/window_sdl.cpp`, its own D3D12 device creation in
   `src/ui/d3d12/d3d12_provider.cpp`, and **no** call to SDL's GPU-device creation function
   (0 hits) `[reported]`.

Put together: a statically linked SDL3 carries its XR module's library name into the binary even
though nothing in the recompiler ever calls it. That explains the string with no VR path behind it
`[inferred-static]`. It is not yet confirmed against our own `rexruntime.dll` (see next step).

## Why it matters for this project

The board row warned that "if a bundled dependency already has an OpenXR path, the work starts
somewhere else entirely". The answer looks like **no**: the dependency that has an OpenXR path
(SDL3's GPU API) is not the one that draws this game — ReXGlue's own D3D12/Vulkan renderer is. So
the design starts where it would have without the string: our own OpenXR submission beside
ReXGlue's renderer. SDL's XR module would only matter if someone ported the renderer onto SDL's GPU
API, which is not a small job and not proposed here `[hypothesis]`.

## Next step (static, `[PD]`, a few minutes)

Confirm on our own binary: look for other SDL XR strings next to `openxr_loader.dll` in
`rexruntime.dll` (for example SDL's own XR error messages, or the name of its OpenXR library hint,
which the SDL docs call `SDL_HINT_OPENXR_LIBRARY`). If they sit together, the row can close as
"SDL3's dormant XR module, not a lead". If the string sits somewhere unrelated to SDL, this topic is
wrong and the row stays open.

## Sources

- ReXGlue SDK source tree and submodule list — https://github.com/rexglue/rexglue-sdk (viewed on
  GitHub, not cloned)
- SDL, "OpenXR / VR Development with SDL" — https://github.com/libsdl-org/SDL/blob/main/docs/README-xr.md
- SDL, OpenXR loader shim — https://github.com/libsdl-org/SDL/blob/main/src/gpu/xr/SDL_openxrdyn.c

No code was copied; the mechanism is described in our own words.
