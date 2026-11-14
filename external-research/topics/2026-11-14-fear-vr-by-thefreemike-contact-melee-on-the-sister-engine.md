# A second F.E.A.R. VR mod (TheFreeMike) does contact melee and leaves the hit to the game's own melee rules

**Status:** 🆕 new · **Priority:** medium (prior art for step 4 of the hands-and-melee plan: physical melee).

## What is public

A free, closed-source F.E.A.R. (Monolith, 2005) VR mod by **TheFreeMike** (`thefreemike31/fear-vr` on GitHub:
documentation, releases and licences only, no source; 0.1.0 on 2026-09-11, 1.8.x by 2026-11-01). It is a different
project from DR-89's fear-vr already in our 2026-09-23 topic. Read from its README, CONTROLS, HISTORY and RELEASE
NOTES, and a vr.org write-up of 2026-11-10 `[reported 2026-11-14]`:

- **Empty-hand melee is physical**: an open hand or a closed fist striking enemies, breakable glass and planks.
  The striking hand must be free: holding a prop, supporting a gun, reloading or climbing takes priority.
- **Weapons strike physically too**, with one guard written down: firing a held gun does not also add a physical
  strike's damage.
- **The game stays in charge of the result**: kicks and slides "follow normal collision and native melee rules; an
  enemy hit is not guaranteed"; for scripted vent covers "native scripts decide when it breaks", and one fix note
  says "native locks, collision and scripted interactions remain authoritative".
- A lesson from its history: physical melee against **scripted** breakables was unreliable until strikes had to
  be distinct (pull the hand back between hits); a grounding-state check once made a vent ignore punches.
- Kicks are not physical (a stick flick); bHaptics melee impacts; the author credits OpenAI's Codex as a
  substantial part of the engineering.

## Why it matters here

1. It is the same shape as our step 4: **the controller's swing starts the game's own melee**, whose hit test,
   damage, sounds and scripts stay `[inferred-static]`. Their notes say where that shape bites: scripted
   breakables (doors, boards, vents) and state checks (on the ground, hands busy).
2. For Condemned 2 that suggests two guards from day one `[hypothesis]`: (a) a swing must start from a pulled-back
   hand (a minimum travel or a re-arm after each hit), so one long wave is not three attacks; (b) no swing attack
   while the hand is busy (a ladder, a finishing move, a pickup, a reload).
3. Engine caution: F.E.A.R. is LithTech Jupiter EX; Condemned 2's own recomp author says the game's code base
   differs a lot from Jupiter EX (Scaleform replacing the stock interface, among other changes), so expect the
   ideas to carry over, not the addresses or class layouts `[reported 2026-10-22, Condemned2Recomp issue #51]`.

## Sources

- TheFreeMike, F.E.A.R. VR Mod (README, CONTROLS, HISTORY, RELEASE NOTES) — <https://github.com/thefreemike31/fear-vr>
- vr.org, "F.E.A.R. VR mod" article, 2026-11-10 —
  <https://vr.org/articles/fear-vr-mod-thefreemike-free-pc-vr-slow-motion-wrist-2026>
- psxrestore, reply on Condemned2Recomp issue #51 (code base vs Jupiter EX) —
  <https://github.com/psxrestore/Condemned2Recomp/issues/51>
