# Upstream check: Condemned2Recomp has been quiet since 2026-08-07; ReXGlue SDK nightlies continue

**Status:** 🆕 new · **Priority:** low — a status check.

## What is public

- **psxrestore/Condemned2Recomp**: one release, `0.1.0` (2026-08-05); last commit 2026-08-07 (README and
  config edits) `[reported 2026-09-17, GitHub API]`. Press covered it as the first playable PC build,
  complete but buggy `[reported]`.
- **rexglue/rexglue-sdk** publishes nightlies; the latest is `0.10.0.8-dev` (2026-09-15), after
  `0.10.0.7` (09-11) and `0.10.0.5` (09-04) `[reported 2026-09-17]`.
- **furqanagwan/recomp-framework** (BSD-3-Clause, 2026-09-15) shares installer, menu and packaging code
  across several recompilations, with its own ReXGlue fork "with the fixes these games need" `[reported]`.
- No public source addresses building ReXGlue projects without **AVX2**, the requirement recorded on the
  board `[reported 2026-09-17, n=1 search]`.

## Why it matters here

Upstream is not moving, so fixes this project makes are its own. If the SDK's newer nightlies change
code generation, a rebuild against them is the likeliest source of changed behaviour.

## Sources

- <https://github.com/psxrestore/Condemned2Recomp>
- <https://github.com/rexglue/rexglue-sdk/releases>
- <https://github.com/furqanagwan/recomp-framework>
- DSOGaming, "Condemned 2: Bloodshot Finally Comes to PC After 18 Years" — <https://www.dsogaming.com/news/condemned-2-bloodshot-finally-comes-to-pc-after-18-years/>
