# Building Condemned2Recomp on a pre-AVX2 CPU — four findings and three patches

**Date:** 2026-09-16
**Reporter:** TefMeister (with Claude)
**Projects:** [Condemned2Recomp](https://github.com/psxrestore/Condemned2Recomp) ·
[rexglue-sdk](https://github.com/rexglue/rexglue-sdk)

First, thank you for this. Condemned 2 has been stranded on 2008 consoles for eighteen years
and it now runs on a PC, which is a genuinely remarkable thing to have done.

This report covers four issues found while getting the game running on a 2012 CPU, and three
small patches. Two are one-line build-configuration fixes; one is a portability fix. All three
are in this folder and were tested together on a clean configure.

---

## Summary

| # | Project | Issue | Patch |
| --- | --- | --- | --- |
| 1 | Condemned2Recomp | The 0.1.0 release binaries require **AVX2**; the project built from source does not | — (rebuild suggestion) |
| 2 | Condemned2Recomp | `CMakePresets.json` sets no `-march` for amd64, so the bundled SDK fails to compile | `03-…` |
| 3 | rexglue-sdk | The libc++ `clock_cast` shim is gated on `__APPLE__`, but every libc++ needs it | `01-…` |
| 4 | rexglue-sdk | `imgui` is linked `PRIVATE`, but a public header includes `<imgui.h>` | `02-…` |

**Test machine:** Intel Core i7-3770S (Ivy Bridge, 2012 — AVX yes, **AVX2 no**),
GeForce GTX 1660 SUPER, Windows 10 22H2, clang 22.1.8 targeting `x86_64-pc-windows-msvc`,
MSVC 14.44 toolchain, Windows SDK 10.0.26100, CMake 4.4.3, Ninja 1.13.2.

---

## 1. The 0.1.0 release binaries require AVX2 — the project itself does not

This is the headline, because it costs you users for no benefit.

Running `condemned2recomp.exe` from the 0.1.0 release on this machine produces:

> The application was unable to start correctly (0xc0000142).

That dialog is misleading — `0xc0000142` normally means a DLL failed to load, and people will
chase missing runtimes for hours. The Windows Event Log records the real fault:

```
Faulting module name: rexruntime.dll
Exception code: 0xc000001d          (STATUS_ILLEGAL_INSTRUCTION)
Fault offset: 0x000000000001d1e4
```

Disassembling that offset in the shipped `rexruntime.dll`:

```
0x0001d1df:  vmovq          xmm0, rdi
0x0001d1e4:  vpbroadcastq   ymm0, xmm0     <-- faults here
0x0001d1e9:  vmovdqu        ymmword ptr [rbx], ymm0
```

`vpbroadcastq` with a `ymm` destination is **AVX2**, which arrived with Haswell in 2013. Every
Intel CPU older than that, and AMD before Excavator, cannot execute it.

**But the SDK's own Windows presets target `-march=x86-64-v2`**, which is SSE4.2 and does not
include AVX2. Only `mac-base` uses `x86-64-v3`. So the published binaries appear to have been
built above the project's own baseline.

Counting AVX2-only instructions across every executable section (shipped release vs. a build
made here from source):

| Binary | Shipped 0.1.0 | Built from source |
| --- | ---: | ---: |
| `condemned2recomp.exe` | 64 | **8** |
| `rexruntime.dll` | 5,192 | **22** |
| `rexgpu-xenos.dll` | 958 | **9** |
| **Total** | **6,214** | **39** |

The 39 that remain are `vpermd`, `vpsllvd`, `vpermq` and `vpmaskmovd` — the shape of
runtime-dispatched SIMD rather than ordinary compiler output — and in practice they are never
reached: **the self-built game launches and plays correctly on this CPU.** Performance is poor,
but that is a 2012 machine doing an honest day's work, not a bug.

**Suggestion:** rebuild the release binaries at the project's own `x86-64-v2` baseline. It
appears to cost nothing and it restores every pre-2013 machine. If AVX2 is deliberate, a line
in the README would save people a long and misleading debugging session.

---

## 2. `CMakePresets.json` sets no `-march` for amd64 → the bundled SDK will not compile

**Patch:** `03-condemned2recomp-march-baseline.patch`

Building with `-DREXSDK_DIR=…` (the SDK from a source tree, as `generated/rexglue.cmake`
invites) fails:

```
rexglue-sdk/src/core/memory.cpp:70:22: error: always_inline function '_mm_shuffle_epi8'
requires target feature 'ssse3', but would be inlined into function
'copy_and_swap_16_aligned' that is compiled without support for 'ssse3'
```

The cause is that `windows-base` and `linux-base` in Condemned2Recomp's `CMakePresets.json`
set a compiler but no `-march`, so the build falls back to the plain `x86-64` baseline
(SSE2 only). The SDK's own presets set `-march=x86-64-v2`, and its sources assume at least
that. `windows-arm64-base` correctly sets `-march=armv8-a`, so this looks like an oversight on
the amd64 path rather than a deliberate choice.

The patch adds `-march=x86-64-v2` to `windows-base` and `linux-base`, matching the SDK.

---

## 3. The libc++ `clock_cast` shim is gated on `__APPLE__` instead of on libc++

**Patch:** `01-rexglue-sdk-libcpp-chrono-guard.patch` (rexglue-sdk)

`include/rex/chrono/chrono.h` provides a shim with this comment:

```cpp
#ifdef __APPLE__
// Apple libc++ does not expose clock_time_conversion or clock_cast.
```

The comment is right about the cause but the guard names the wrong thing. It is **libc++**
that lacks `std::chrono::clock_time_conversion` and `clock_cast`, not macOS — so any libc++
build on another platform hits it:

```
include/rex/chrono/chrono.h:131:8: error: explicit specialization of undeclared
template struct 'clock_time_conversion'
```

The patch widens the guard to `#if defined(__APPLE__) || defined(_LIBCPP_VERSION)`. It is a
no-op on MSVC's STL and libstdc++, which both provide the real thing.

---

## 4. `imgui` is linked `PRIVATE`, but a public header includes `<imgui.h>`

**Patch:** `02-rexglue-sdk-imgui-public.patch` (rexglue-sdk)

Consuming the SDK via `REXSDK_DIR` fails while compiling the SDK's *own* source into the
consumer target:

```
include/rex/ui/style.h:18:10: fatal error: 'imgui.h' file not found
```

The chain is:

- `src/kernel/CMakeLists.txt` links `imgui` into `rexruntime` as **`PRIVATE`**, so imgui's
  (correctly `PUBLIC`) include directories do not propagate to consumers.
- `rex/ui/style.h` is a **public** header and includes `<imgui.h>`.
- `rexglue_configure_target()` adds `rex_app.cpp` to the **consumer's** target, which links
  only `rex::runtime` — and therefore never sees imgui's include directories.

When a public header includes a dependency's header, that dependency is part of the interface.
The patch links `imgui` as `PUBLIC`, with a comment explaining why.

---

## Notes for other Windows builders — not bugs in your projects

Recorded because they cost time and the error messages point nowhere useful.

- **`clang-XX: error: unable to execute command: program not executable`** when linking.
  CMake's `Windows-Clang` module hard-codes `-fuse-ld=lld`, which needs the COFF-flavoured
  driver named `lld-link`. LLD selects its flavour from the name it is invoked under, and some
  distributions (LLVM-MinGW among them) ship `ld.lld` without a `lld-link` alias. Copying
  `ld.lld.exe` to `lld-link.exe` is enough.
- **`lld-link: error: could not open 'libcmt.lib'`.** Clang finds MSVC's headers by itself but
  not its libraries; run the build from a `vcvars64.bat` environment.
- **`libmspack/cabextract/mspack/lzxd.c:1:1: error: expected identifier or '('`.** Git on
  Windows without symlink support checks out mode-`120000` entries as small text files
  containing the link target, so the compiler reads a path as source. Enable
  `core.symlinks`, or replace those entries with real copies. Fifteen files in the submodule
  tree are affected; the seven that cannot be resolved are all MoltenVK and are not built on
  Windows.

---

## The patches

```
01-rexglue-sdk-libcpp-chrono-guard.patch     rexglue-sdk        1 line
02-rexglue-sdk-imgui-public.patch            rexglue-sdk        1 line + comment
03-condemned2recomp-march-baseline.patch     Condemned2Recomp   2 lines
```

Apply with `git apply <file>` from the relevant repository root. They are offered as-is — take
them, rewrite them, or ignore them, whichever is most useful. If you would prefer pull requests
against either repository, say the word and they will be opened.

Thanks again for the port.
