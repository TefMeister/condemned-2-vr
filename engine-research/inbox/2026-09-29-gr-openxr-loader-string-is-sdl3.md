# inbox: the `openxr_loader.dll` string in `rexruntime.dll` is probably SDL3's unused XR module

From: `/gr` estate sweep, 2026-09-29.
For: board `[PD]` row "trace the unexplained `openxr_loader.dll` string in `rexruntime.dll`", and
dossier line 50 (the loader listed beside `renderdoc.dll`, `steam_api64.dll`, `gameinput.dll`).

Short version: the ReXGlue SDK has no OpenXR code of its own, but it bundles SDL3, whose GPU API
now has an OpenXR module that loads `openxr_loader.dll` by that exact name on Windows. ReXGlue uses
SDL for its window only and draws with its own D3D12 renderer, so the string is most likely carried
in unused `[inferred-static]`. Not yet checked on our binary: look for SDL's other XR strings next
to it in `rexruntime.dll`; if they are there, the row closes as "not a lead".

Full write-up and sources:
`external-research/topics/2026-09-29-the-openxr-loader-string-is-sdl3s-dormant-xr-module.md`
