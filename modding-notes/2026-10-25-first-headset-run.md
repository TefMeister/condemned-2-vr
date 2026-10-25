# 2026-10-25 — first run of the VR build in the real headset (home PC)

- Build: the 2026-10-15 files (exe `1b7e00d69d46`), `c2_vr_headset.ps1`, Quest 3 over Virtual Desktop, Xbox pad.
- Tefa: "this really just works in VR, just played it for some time with the controller ... there are some visual things
  that need attention but for the most part, it's like playing a vr game with a controller" `[verified-live 2026-10-25, n=1]`.
- Log 17:43-17:54: 47,100 headset frames, not_rendered 0, draw_errors 0, wait 4.3 ms mean; headset tangents 1.19 / 1.19 /
  0.67 / 1.04, eyes 0.0603 m `[measured 2026-10-25]`. No freeze, so the threaded fallback is not needed.
- `"MusicEnable" "0"` added to `Assets/autoexec.cfg` (backup kept).
- First VR build released as a pre-release `v0.1.0-first-vr`, on top of upstream Condemned2Recomp 0.1.0 (BSD licences in the zip).

## Not established
- The "visual things": "minor stuff we'll iron out later on" (Tefa), not described yet.
