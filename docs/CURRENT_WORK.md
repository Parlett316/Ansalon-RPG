# Current work

Nothing in flight.

Milestone 49 (Laurana) shipped: the largest single character addition
this project has made, at the user's explicit request. Nine `PRESENCE`
windows in a new `CHARACTER laurana` block in `data/timeline.txt`, all at
locations/day-ranges already modeled (`qualinesti 7 9`, `pax_tharkas
10 12`, `tarsis 20 22`, `ice_wall 38 42`, `high_clerist_tower 76 80` and
`81 81`, `palanthas 83 89`, `kalaman 90 92`, `neraka 105 107`) — pure
data, no new `LOCATION`, no loader changes. Re-verified the `ice_wall
38 42` roster directly against a fresh `Dragons_of_Winter_Night`
extraction before touching it (now saved as `.research/dwn_full.txt`):
"The Song of the Ice Reaver" names her among the party outright and
gives her the window's central beat, so Milestone 36's existing roster
was correct, just incomplete — nothing needed correcting, only adding.
Her `high_clerist_tower 81 81` window (the day Sturm dies) gets a real
`SAY`, not the established "no SAY" device, since she survives it and
carries the rest of the war. Two anonymous speaker tags standing in for
her in already-shipped Hero dialogue ("a golden-haired elfwoman") were
updated in place to read "Laurana"; plain pronouns elsewhere were
deliberately left alone. Full reasoning and sourcing in
`docs/TIMELINE_NOTES.md`'s "Laurana" section.

This milestone also retired this project's long-standing "keep major
recurring canon characters unnamed" precedent as a blanket rule —
Milestone 48 (Fizban) was the first reversal; this one confirms it
wasn't a one-off. `docs/TIMELINE_NOTES.md` gained a new "Named vs.
off-stage canon characters" note (right before the milestone-by-milestone
history begins) explaining what the project actually does now: a
character gets a real `CHARACTER` block only when the source gives them
genuine on-page, talk-shaped scenes at locations/day-ranges the game
already models, never invented to fill a gap — the same restraint that
was always underneath the old blanket rule. Every per-milestone
write-up that specifically claimed Laurana (or, at Milestone 45/46/48,
Fizban) "stays unnamed" was updated to say she/he stayed unnamed *at that
milestone* and note where they're named now; write-ups about characters
still genuinely off-stage (Kitiara, Alhana Starbreeze, Derek Crownguard,
Lord Gunthar, Ariakas, Lord Soth, Feal-thas, Fewmaster Toede, Verminaard)
were left as accurate history plus a pointer to the new note.

Verified via a clean rebuild (zero new `/W4` warnings) and the piped
character-creation smoke test (real `save.txt` moved aside and restored
around the run, as always). Docs updated: `docs/TIMELINE_NOTES.md` (new
"Named vs. off-stage" note + "Laurana" section + per-milestone
touch-ups), `docs/MILESTONES.md` (new Milestone 49 entry + NEXT UP
touch-ups), `docs/ZONE_NOTES.md` (four per-zone touch-ups — Kalaman,
Palanthas, Godshome, Neraka — that also stated the retired precedent),
`README.md`'s Status paragraph.

Next: pick from `docs/MILESTONES.md`'s "NEXT UP" — one live option left:

1. **Dragon Highlords** — mostly validates the existing restraint
   (Verminaard/Feal-thas/Toede confirmed correctly off-stage). Kitiara is
   the one exception with real talk-shaped material at `neraka 105 107`,
   and Milestone 49 added two more scenes that put her on-page in
   someone else's dialogue (`high_clerist_tower 81 81`, `neraka 105 107`
   again from Laurana's side) — but it needs a design decision first:
   adversarial character on the same friendly talk/topic picker, or stay
   retrospective dialogue inside an existing Hero's `TOPIC`? Raise via
   `AskUserQuestion` before scoping content.

Or something else — ask the user before starting.
