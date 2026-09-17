# /gs 2026-09-17: the OPEN block is unreadable to /gates and says itself it is stale

**From:** `/gs` sweep, home PC, 2026-09-17. **Owner:** modding (`/pd` or `/lm` on condemned-2-vr).

## 1. `claude-memory/status/condemned-2-vr.md` OPEN block

`gate-scan.sh --check` reports: `condemned-2-vr  OPEN block has no rows (write ": none" if idle)`
`[verified-numerically 2026-09-17, n=1 scan]`.

The block does carry `[USER]` rows, but the first line under the header is the prose warning
"⚠️ STALE BLOCK — flagged 2026-09-16 night, NOT re-audited", which is not a tagged row, so the parser
reads the block as empty. `/gates` therefore shows nothing for this project.

The warning itself says the rows are overtaken: the game now runs on the dev PC from a self-build
(the "CPU cannot run it / copy to RTX" row is obsolete) and the repo exists (the "no repo yet" note is
wrong). `[reported 2026-09-16, by the status file itself]`

**Fix:** re-audit the block and re-date it, as the warning asks. Keep any caution as a tagged row or
move it below the block, so the first line after `OPEN (...):` is a row.

## 2. `engine-research/inbox/README.md` is the short template

Check 5 flags it as missing the `Supersedes:` protocol and the confidence-tag rules. Copying
`unreal-gold-vr/engine-research/inbox/README.md` (the unified version) fixes it.
