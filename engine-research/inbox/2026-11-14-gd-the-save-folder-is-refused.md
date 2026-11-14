# Gold Digger: the game's save folder is refused at start (2026-11-12)

From the full scan of 2026-11-14 (Keeper PC). Shelf `logs`, the recomp's own logs
(`logs__condemned2recomp#.log`), 2026-11-12 18:51 to 18:58.

Most of the warnings are the usual file probing of a recomp (a missing `display.cfg`, `settings.cfg`, `Game.ini`,
`HostOptions.map`, the `UPDATE:` device; `0xc000000f` = no such file) and are harmless. One is a different code:

- `[NtCreateFile] FAILED: path='D:\Save' -> 0xc0000022` — **access denied**, not "not found" `[measured 2026-11-12,
  the log]`. If the game's saves go there, saving or loading may silently fail.
- Also: `SDL GameControllerDB: file 'gamecontrollerdb.txt' does not exist` (the pad mapping database; the Quest
  controllers already work as the pad since b002, so probably harmless).

Proposed board row: `[PD]` where the recomp maps `D:\Save` on disk, and whether that folder is read-only or locked;
then one save and load in a flat run to see whether saving works.
