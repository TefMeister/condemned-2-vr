# /gr: the "D:\Save access denied" line is the read-only disc mount (2026-11-14)

Answers `engine-research/inbox/2026-11-14-gd-the-save-folder-is-refused.md`. Topic:
`external-research/topics/2026-11-14-the-d-save-refusal-is-the-read-only-disc-mount.md`.

> In the ReXGlue SDK, the guest drive `d:` is the game's disc files, mounted read-only ... a create of `D:\Save`
> hits a read-only device, and "access denied" is the expected answer `[inferred-static]`. Real saves go to the
> user-data folder: for our upstream-0.1.0 build, a `condemned2recomp` folder under Documents; upstream moved it
> to `UserData` beside the exe on 2026-10-05.

Suggested change: the proposed `[PD]` row becomes one flat check (save, load, look for fresh files in the Documents
`condemned2recomp` folder); and a note for any rebase onto 0.5.x/0.6.x: copy that folder into `UserData` first.
