# Inspector notes

Written by the lanes Inspector after code edits. **It never edits code; it only writes this
folder.** Each file holds one kind of note:

| File | What is in it |
| --- | --- |
| `this-session.md` | noted during the session in progress, not answered yet |
| `waiting.md` | left unanswered by an earlier session that ended first: **answer these first** |
| `decided.md` | answered, with the reason; a note here is only raised again if it gets clearly worse |
| `already-there.md` | what was in the code before the Inspector first looked at each file |
| `cleared.md` | notes that have left the code, newest first |

**To answer a note**, replace `waiting` on its Verdict line with one of:

- `fix now`: real; fix it before the session ends. **Only for LOOSE-NUMS, LOOSE-ADDRESS and DEAD-CODE**,
  whose fix cannot change behaviour. Anything structural (splitting, moving, re-binding a key,
  de-duplicating) is `fix later`, done on its own and tested.
- `fix later: <the board row that carries it>`
- `keep: <why it has to stay this way to work>`

Also check by eye, since no script can: a log limit that also switches off the behaviour it logs, and a
helper that exists in near-identical form in another game's code.

**Emergency save:** a session that has to end with notes unanswered commits with `inspector: carry over`
in the message; the notes wait in `waiting.md` and the next session is told about them first.
