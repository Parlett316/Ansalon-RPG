# Current work

Nothing in flight.

Milestone 50 (Kitiara) shipped: resolves the "Dragon Highlords" backlog
item. The open design question (adversarial character on the friendly
talk/topic picker, or retrospective dialogue inside an existing Hero's
own `TOPIC`) was raised via `AskUserQuestion` and answered: retrospective
dialogue, the same technique already used for Raistlin's Neraka reunion
via Caramon. No new `CHARACTER kitiara` block, no new `PRESENCE`, no new
`LOCATION` -- she never becomes directly talkable. Four content changes
in `data/timeline.txt`, each re-sourced directly from a fresh
`pdftotext -layout` extraction before writing anything: Tanis's new
`TOPIC "The Crown of Power"` at `neraka 105 107`; Laurana's `TOPIC "What
the Dragon Highlord Said"` at `high_clerist_tower 81 81` got a
light-touch name edit (same technique as the Fizban/Laurana anonymous-tag
fixes) plus a new second `TOPIC "The Dragonlance Returned"`; Caramon's
new `TOPIC "His Sister"` at `neraka 105 107`. Kitiara is named throughout
-- the blanket "keep major recurring canon characters unnamed" precedent
was already retired at Milestone 49, and every sourced scene has a
tracked Hero saying or hearing her name directly in the source text.
Tanis's existing `TOPIC "A Debt He Won't Name"` at `kalaman 100 100` was
deliberately left untouched (an in-character refusal to name her, not a
narrator placeholder). Ariakas and Lord Soth remain off-stage; Verminaard/
Feal-thas/Fewmaster Toede's off-stage treatment was already confirmed
correct by the Milestone 48/49 research pass. Full sourcing and reasoning
in `docs/TIMELINE_NOTES.md`'s "Kitiara" section.

Verified via the piped character-creation smoke test (real `save.txt`
moved aside and restored around the run, as always) -- pure data + prose
edits, no new grammar, so no throwaway self-test or forced full rebuild
was needed, same call as Milestones 47-49. Docs updated:
`docs/TIMELINE_NOTES.md` (new "Kitiara" section + a touch-up to the
Laurana section's forward pointer), `docs/MILESTONES.md` (new Milestone
50 entry + NEXT UP renumbered now that "Dragon Highlords" is resolved),
`README.md`'s Status paragraph. No `docs/ZONE_NOTES.md` changes needed --
nothing there became inaccurate.

**Not yet committed** -- ask the user before committing.

Next: `docs/MILESTONES.md`'s "NEXT UP" has two live options left:

1. **Terrain-specific monster pools** -- encounter chance already varies
   by terrain (Milestone 27); which monster you fight is still
   uniform-random. See `docs/COMBAT_NOTES.md`'s "Extending this later."
2. **More monsters** -- Bozak/Sivak/Aurak Draconians, Thanoi, and other
   untouched Monstrous Manual entries. Higher-tier draconians are
   spellcasters/shapeshifters -- real mechanics not modeled yet. See
   `docs/COMBAT_NOTES.md`'s "Extending this later."

Or something else -- ask the user before starting.
