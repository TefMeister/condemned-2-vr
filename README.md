# Condemned 2: Bloodshot — PC and VR project

Notes, build fixes and tooling for getting **Condemned 2: Bloodshot** running on PC, and
— eventually — in a VR headset.

Condemned 2 never had a PC release. It shipped on Xbox 360 and PlayStation 3 in 2008 and
stayed there. The PC route is **[Condemned2Recomp](https://github.com/psxrestore/Condemned2Recomp)**,
a *static recompilation* built on the **[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk)**:
the original Xbox 360 PowerPC code is translated into a native x64 Windows program rather
than emulated. All the hard work there belongs to those projects — see [Credits](#credits).

This repository holds our own work around that: what we had to fix to build it, what we
learned about the renderer, and the VR effort built on top.

## What is here

| Folder | What it holds |
| --- | --- |
| [`patches/`](patches/) | Build fixes we found, as applyable patches, plus a report written to be sent upstream |
| [`dev-archive/`](dev-archive/) | The live working source and reverse-engineering evidence |
| [`engine-research/`](engine-research/) | Distilled notes on how the engine and the recompilation runtime behave |

## Status

**The game builds from source and runs on hardware the official release cannot run on.**

The published 0.1.0 binaries require **AVX2**, a CPU feature introduced with Intel's Haswell
in 2013. On an older machine they fail at startup with a misleading
`0xc0000142` dialog — the real fault is an illegal-instruction crash. Building from source
using the project's own baseline produces a binary that runs correctly on a 2012
**Intel Core i7-3770S** (Ivy Bridge). Details and measurements are in
[`patches/UPSTREAM-REPORT.md`](patches/UPSTREAM-REPORT.md).

VR work has not started. The renderer is **D3D12**, and because the whole program is rebuilt
from readable source, stereo rendering can be added at the source level rather than injected
from outside — which is a far better starting position than most flat-to-VR projects get.

## What this is not

- **This is not the port.** It is not a fork, a rewrite, or a replacement for
  Condemned2Recomp or ReXGlue. It is a set of notes and small fixes around them.
- **It contains no game content.** No assets, no executables, no disc images, nothing
  extracted from the game. You need your own legitimate copy of Condemned 2: Bloodshot.
- **It is unfinished.** Nothing here is a supported product.

## Caution

This work is **unfinished and experimental**. Any VR build that comes out of it may cause
**severe motion sickness and discomfort**. This warning will only be removed for a given
build once it has been played through and confirmed comfortable.

## Credits

Everything here stands on other people's work:

- **[psxrestore](https://github.com/psxrestore)** and the **Condemned2Recomp** contributors,
  who did the thing that actually matters — making Condemned 2 run on PC at all.
- **The [ReXGlue](https://github.com/rexglue/rexglue-sdk) project**, for the Xbox 360
  recompilation runtime and toolkit the port is built on.
- **Monolith Productions**, who made the game, and **SEGA**, who published it.
- **[Redump](http://redump.org/)**, whose drive-compatibility and firmware documentation is
  the only reason the disc could be read at all, and the **Kreon** firmware authors.
- **[redumper](https://github.com/superg/redumper)** (superg) for disc dumping, and
  **[xdvdfs](https://github.com/antangelo/xdvdfs)** (antangelo) for reading the disc image.
- **[Xenia](https://github.com/xenia-project/xenia)**, whose architecture is visible in the
  recompilation runtime's design and naming.

If you should be credited here and are not, or you would like something changed or removed,
please get in touch and it will be corrected as quickly as we can manage.

## Legal

A non-commercial fan project. It requires you to own a legitimate copy of the game and
redistributes no original assets. The techniques used are ordinary reverse-engineering
applied to software we own, for personal use.

## Licence

**BSD 3-Clause** — see [`LICENSE`](LICENSE). Deliberately the same licence as the
[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) and the recompilation projects it supports, so
that fixes can move freely in either direction without a licensing conversation.

It covers the code and notes in this repository only. It grants **no rights in any game or asset** —
none are included here, and you need your own legitimate copy of the game.
