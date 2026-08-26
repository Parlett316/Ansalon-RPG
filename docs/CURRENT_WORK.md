# Current work

Nothing in flight.

Milestone 94 (2026-08-26) added explicit save-slot deletion: the launch
menu now accepts `d1`/`d2`/`d3` to immediately delete a save (with its own
`y/n` confirmation), separate from the existing decline-continue/confirm-
overwrite flow, which only replaces a slot's file on the next autosave.
New `game::SaveGame::remove`; `main.cpp`'s `promptSlotChoice` extended to
a `SlotChoice{slot, deleteRequested}`. Verified via a clean `/W4` rebuild
and a piped interactive test in an isolated `build/Debug` copy (the
save-slot menu, like `CharacterCreator`, runs on plain `std::cin`/
`std::cout` and is pipeable) covering delete-confirmed, delete-declined,
and delete-on-empty-slot; plus the standard piped smoke test with the
user's real `save1.txt`/`save2.txt`/`save3.txt` moved aside and restored
(unchanged). See `docs/MILESTONES.md` entry 94 and `docs/ARCHITECTURE.md`'s
"Save/load" section.
