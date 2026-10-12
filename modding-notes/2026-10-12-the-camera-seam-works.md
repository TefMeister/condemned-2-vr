# The camera seam works (2026-10-12)

`/lm` on the dev PC, Claude driving on Opus, one background reader.

## In one paragraph

Our own Condemned 2 build still runs on the dev PC (Tefa was right: only the official download needs the
newer processor). It was rebuilt on today's shared SDK and reaches gameplay. The reader found the one
function every 3D view goes through ("set view"); a probe that moves the camera sideways there, together
with The Darkness's side-by-side picture, shows the alley from two viewpoints, so the eye shift reaches the
screen. The picture in play is heavily blurred by the game's own screen effects, which come off next.

## What was measured

- Runs as installed, and again after the rebuild: logo, film, menus, Rock Bottom, gameplay `[verified-live 2026-10-12, n=4]`.
- Every 3D view arrives through return address 0x8212B854; the player view is mode 0, block 0x82925958, 1280x720.
- Side by side with the eye shift: ~2 px between halves at 5 units, ~17 px at 40 units, by eye `[verified-live 2026-10-12, n=2]`.
- Music line added to `Assets/autoexec.cfg`; not heard yet.

## Not established

- Whether both halves are always the same moment (The Darkness needed a freeze for that).
- Whether the screen-effects switches (cfg lines + the reader's draft hook) remove the blur. Drafted, not built.
- What a world unit is (centimetres is a guess).

## Later: the screen effects switch off

A second `/lm` built the reader's effects hook into the game. With `C2_VR_SCREEN_FX_OFF=1` the same alley is
crisp (edge sharpness 0.66 → 2.08; motion blur, screen effects, damage effects, camera kick and head bob all
forced off) `[verified-live 2026-10-12, n=1]`. Off unless switched. In that sharp picture the walking figure
seems to be at a different moment in each half, so a freeze like The Darkness's is likely needed next.
