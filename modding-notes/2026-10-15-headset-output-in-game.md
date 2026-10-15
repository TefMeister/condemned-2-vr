# 2026-10-15: the headset output, in the game for the first time (`/lm`, dev PC, OpenXR simulator)

Five launches of our own build, each driven from the title to the Rock Bottom alley with a virtual
pad. No headset: the OpenXR simulator stands in for it. Recordings (game window and game sound only)
are in the MEGA transfer folder under `Videos\condemned-2-vr\`: `xr-head-first-run_2026-10-15_14-07-31.mp4`,
`xr-fov-run_2026-10-15_14-19-07.mp4`, `music-off-run_2026-10-15_14-23-02.mp4`, plus a 20-second sound
clip of the main menu, `main-menu-sound-check_2026-10-15.mp3`. Pictures and log lines:
`dev-archive/recon/2026-10-15-xr-first-game-run/`.

## Run 1: patch 09 as built by the 2026-10-15 /pd (`DK_XR=1 C2_XR_HEAD=1`)

- **The headset gets the game.** `[XR] runtime up: 11 formats offered, using DXGI 29`, session reached
  FOCUSED, eye images 1280x720, gamma undone. 4,200 headset frames with `draw_errors=0` and
  `not_rendered=0`; menus, films and loading went to the headset as well. The simulator preview shows
  the alley in both eyes `[verified-live 2026-10-15, n=1]`.
- **No freeze.** `xrWaitFrame` inside the presenter's callback did not stall the game or the window;
  the mean wait was 0.2 ms, because the game makes far fewer pairs a second than the headset's 90
  (about 6 a second on this PC, which says nothing about the home PC). The "move OpenXR to its own
  thread" fallback is not needed so far `[verified-live 2026-10-15, n=1]`; it may still matter when the
  game outruns the headset on the home PC `[hypothesis]`.
- **Head samples arrive in gameplay only**, as designed: `sample_hits` stays 0 through menus and films
  and climbs one per pair once the player view exists (2,880 hits of 4,200 frames by the end).
- **The head turns the camera, all three axes the right way**, judged by eye against the head-straight
  picture `[verified-live 2026-10-15, n=1 each]`:
  - head yaw +30 (left in OpenXR's convention): the scene moves right in both eyes = the view turned left;
  - head pitch +20 (up): the scene moves down = the view looked up;
  - head roll +20 (left ear down): vertical edges lean clockwise in the picture = the head tilted left,
    while the on-screen text stays level.
  This rests on the simulator using OpenXR's standard signs (yaw + = left, pitch + = up), which is
  `[inferred-static]` (the simulator's own source was not read). The headset at home settles it.
- **The submitted view matches the picture:** the projection layer carried the game's own tangents
  (±0.894 rad wide, ±0.611 rad tall = the game's 1.2448 / 0.7002) and the head pose of the sample the
  pair was drawn with (read from the simulator's projection log).
- Not yet the headset's own view: the game's 16:9 picture covers only ±35° vertically, while the
  simulator's Quest 3 profile shows +44° / -54°. Run 2 deals with that.

## Run 2: the game draws the headset's own view (`C2_XR_FOV=1`, new)

- **SDK patch 09b** (`patches/09b-rexglue-sdk-openxr-headset-tangents.patch`): each head sample also
  stores the headset's view for both eyes together (largest left/right/up/down tangent of either eye),
  exported as `rex_xr_headset_tangents`. Logged once:
  `headset view (both eyes): tangents left 1.3764 right 1.3764 up 0.9650 down 1.3901`.
- **Game side** (`vr_head_pose.cpp` `HeadsetFov`, `vr_camera_probe.cpp`): with `C2_XR_FOV=1`, the player
  view is drawn at one tangent per axis that covers the headset (x = 1.3764, y = 1.3901), the game's
  own FOV buffer is given back afterwards, and the same tangents go to the headset through
  `dk_xr_eye_fov`. `[compile-verified 2026-10-15]`
- **It works:** log `XR fov: player view drawn at the headset's tangents x=1.3764 y=1.3901`; the
  projection layer then carried ±0.9425 / ±0.9472 rad, exactly those tangents; the flat picture shows
  much more above and below, squashed upright far more than sideways (a vent's height-to-width fell to
  about two thirds), which is what a near-square view drawn into a 16:9 image looks like on the monitor
  `[verified-live 2026-10-15, n=1]`. In the headset that stretch is undone by the compositor.
- ⚠️ Symmetric, so a little is wasted: the headset looks 44° up but 54° down, and the picture covers 54°
  both ways (about a fifth of the rows go unseen). The view-set call takes a second pair of floats
  (`r7`, stored at view +56/+60) that looks like a projection offset; if it shifts the view centre, it
  would make the picture fit exactly `[hypothesis]`.
- Objects at the new top and bottom edges did not visibly pop in or out; the game's culling seems to use
  the same view `[hypothesis, n=1 picture]`.

## Run 3: music

- With `C2_MUSIC_OFF=1` the mixer push logs `music=0 -> 0`: the music volume was **already 0** when it
  reached the mixer, so `"MusicEnable" "0"` in `Assets\autoexec.cfg` does its job on the music volume
  `[verified-live 2026-10-15, n=1]`.
- **But the main menu still plays a steady sound** at about -32 dB, the same as in run 1 (-27 to -31 dB),
  and in-game quiet play sits near -55 dB `[measured 2026-10-15]`. Whether the menu sound is music (on
  another path, such as a streamed track the volume does not cover) or menu ambience cannot be told from
  levels. A 20-second clip is in the transfer folder for Tefa to listen to.

## Run 4: the eye spread comes from the headset (`C2_CAM_EYE=auto`, new)

- The reader read the game's movement database and found **1 game unit = 1 cm** (player box 190 cm
  tall, walk 2.35 m/s, gravity 20 m/s²) `[inferred-static 2026-10-15]`, and that `C2_CAM_EYE` is the
  shift of ONE eye. So the `5` used until now was a 10 cm spread, about 1.5 times a real one.
- SDK patch 09b now also exports `rex_xr_eye_distance` (the distance between the two eye poses the
  headset reports). With `C2_CAM_EYE=auto` each eye moves half of that times `C2_XR_UNITS_PER_M` (100);
  until the headset reports, 3.2. Live: `eyes 0.0640 m apart -> each eye 3.200 units`, and the eye
  shift lines read `e=±3.2` `[verified-live 2026-10-15, n=1]`. Following a changed IPD setting during
  play is not tested.

## Run 5: the view fits the headset exactly, off-centre

- The reader read the matrix builder `sub_8212B010`: set view's second pair (r7, view +56/+60) is the
  picture's centre in tangent units, `ndc = (tan - centre) / half-width`, + = right/up
  `[inferred-static 2026-10-15]`. So any headset box (left L, right R, up U, down D) fits exactly with
  half-widths (L+R)/2, (U+D)/2 around a centre (R-L)/2, (U-D)/2.
- `C2_XR_FOV=1` now does that (the game's own pair is written back after the call), and tells the
  headset the same uneven box. Live: `game offset was (0.0000 0.0000); drawn half-widths (1.3764 1.1776)
  centre (0.0000 -0.2125)`; the projection layer carried left/right ±0.9425, up 0.7676, down -0.9472 rad,
  exactly the headset's own view; the picture's centre moved down against run 2 (things sit higher in
  the frame) `[verified-live 2026-10-15, n=1]`. No rows are wasted now.
- Still open: whether the game's culling uses the same shifted view (objects entering from the bottom
  edge would pop in if not) `[hypothesis]`; no frustum builder was found statically. Also open: a second
  view (block 0x82925AF0, `sub_82123538`) copies the current centre with its own field of view, possibly
  the held weapon `[hypothesis]`; check the weapon in the headset.

Recordings for runs 4 and 5: `eye-auto-run_2026-10-15_14-33-44.mp4`, `off-centre-run_2026-10-15_14-39-21.mp4`
(same transfer folder).

## Smaller things learned

- The OpenXR simulator's preview window belongs to the game's own process. The pad driver picked it as
  "the game window", so its first screenshots and focus went to the preview. Fixed in the session copy
  of `c2_padd.py` (picks the window whose title does not start with "OpenXR"), kept beside the pictures.
- The route to gameplay is now in `ai-game-control-profiles/routes/condemned-2/KNOWN.md`.

## Not established

- Real headset: comfort, scale, whether the picture sits still in the world while the head turns
  between pairs (the headset gets each pair's own pose, so it should).
- Head movement (`C2_XR_HEAD_POS=1`, now with a measured 100 units per metre) was not switched on.
- Whether the menu sound is music.

## Run line for the home PC (real headset: leave out `XR_RUNTIME_JSON`)

`DK_SBS=1 DK_SBS_HOLD=1 C2_EYE_SYNC=1 C2_CAM_EYE=auto C2_STEREO_PERIOD=1 C2_VR_SCREEN_FX_OFF=1
C2_MUSIC_OFF=1 DK_XR=1 C2_XR_HEAD=1 C2_XR_FOV=1`, script `dev-archive/recon/2026-10-15-xr-first-game-run/c2_xr_launch.ps1`.
