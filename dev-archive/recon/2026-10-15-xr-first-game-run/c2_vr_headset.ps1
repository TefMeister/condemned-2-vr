# Condemned 2 VR, headset run (2026-10-15 build). Put this script beside condemned2recomp.exe and run it.
# Uses the PC's active OpenXR runtime (the real headset); no simulator.
$env:DK_SBS = "1"
$env:DK_SBS_HOLD = "1"
$env:C2_EYE_SYNC = "1"
$env:C2_CAM_EYE = "auto"
$env:C2_STEREO_PERIOD = "1"
$env:C2_VR_SCREEN_FX_OFF = "1"
$env:C2_MUSIC_OFF = "1"
$env:DK_XR = "1"
$env:C2_XR_HEAD = "1"
$env:C2_XR_FOV = "1"
Remove-Item Env:XR_RUNTIME_JSON -ErrorAction SilentlyContinue
$p = Start-Process -FilePath (Join-Path $PSScriptRoot "condemned2recomp.exe") -WorkingDirectory $PSScriptRoot -PassThru
"pid=$($p.Id)"
