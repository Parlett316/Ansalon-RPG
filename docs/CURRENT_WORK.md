# Current work

Nothing in flight.

Milestone 66 (Aftermath dialogue) just shipped -- new `TALK_AFTER` zone
grammar plus `timeline::Timeline::latestDayEnd`, so a zone NPC (Otik at the
Inn of the Last Home, the one proof-of-concept POI) can react the first
time it's talked to once the Heroes' schedule at that location has fully
closed, correctly firing even if the player met the NPC before the Heroes'
window ever opened. See `docs/MILESTONES.md`'s Milestone 66 entry and
`docs/ZONE_NOTES.md`'s "Aftermath dialogue" section for the full design.
No save-format change. Verified via a throwaway self-test (`latestDayEnd`
plus `TALK_AFTER` parsing/fail-fast cases), a clean `/W4` rebuild, and the
piped smoke test. Interactive verification (talking to Otik before/after
day 2, confirming the aftermath line fires once then falls back to
`TALK_AGAIN`) still needs the user's own keyboard. Widening `TALK_AFTER` to
other zones is `docs/MILESTONES.md`'s NEXT UP item 5, not started.
