# Current work

Nothing in flight.

Milestone 89 (2026-08-25) replaced the single hardcoded `save.txt` with a
3-slot save/load system (`save1.txt`/`save2.txt`/`save3.txt`), at the
user's request. `game::SaveGame`/`game::GameLoop` needed zero changes
(already parameterized by path); the whole feature lives in `main.cpp`
plus a `CMakeLists.txt` define rename. See `docs/MILESTONES.md` entry 89
for the full writeup and `docs/ARCHITECTURE.md`'s "Save/load" /
`docs/GOTCHAS.md`'s "Save/load" section for how the new mechanism works.

Verified via piped scripted input against an isolated scratch copy of the
built exe (never the user's real `build\Debug\`): empty-slot menu reaches
character creation, decline-continue/decline-overwrite loops back to the
menu correctly, a deliberately corrupted slot shows as unreadable without
aborting the other slots, and a dropped-in legacy `save.txt` migrates to
`save1.txt`, loads, and renders the real overworld frame correctly. Clean
`/W4` rebuild, zero new warnings. The user's actual `build\Debug\save.txt`
was backed up before any of this and confirmed byte-identical afterward
(it was never touched — the scratch-copy approach kept the real directory
out of the loop entirely).

The real migration was also exercised directly: the piped smoke test was
run once against the actual `build\Debug\` (per `CLAUDE.md`'s "real
playthrough check against the user's actual save" step, since this change
touches the save format/flow for their in-progress character). It printed
the migration notice and renamed `build\Debug\save.txt` to
`build\Debug\save1.txt`; a diff confirmed the migrated file is
byte-identical to the pre-migration backup
(`build\Debug\save.txt.bak-milestone89`, kept as a safety net). **Not yet
done**: the user hasn't yet played the new slot menu themselves with their
own keyboard -- only the migration and the menu's non-interactive prefix
have been confirmed; picking Slot 1 and actually continuing into the game
world with real keypresses still needs their own hands-on check.

Also worth knowing for a fresh session: `git status` shows Milestones
85-89 as uncommitted working-tree changes -- nothing has been committed
to git yet, since no session this far has been asked to commit.
