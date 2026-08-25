# Current work

Nothing in flight.

Milestone 71 ("Ask about anything" -- free-text `SUBJECT`/`SUBJECT_UNKNOWN`
conversation subjects, new grammar in both `data/timeline.txt` and
`data/zones/*.txt`, a new `Console::readLine` raw-`_getch()` text-entry
primitive, and one proof-of-concept window on Raistlin's `PRESENCE solace
0 1`) just shipped -- see `docs/MILESTONES.md`'s Milestone 71 entry and
`docs/TIMELINE_NOTES.md`/`docs/ZONE_NOTES.md`'s "Ask about anything"
sections for the full mechanism and rationale. Verified via a throwaway
self-test (tokenizing/keyword-matching), a clean `/W4` rebuild (zero new
warnings), and the piped smoke test (confirms the new timeline grammar
parses). **Interactive verification of the free-text prompt itself still
needs the user's own keyboard** -- typing "Kitiara" at Raistlin, typing
something unrelated (e.g. "Master of Past and Present") to confirm the
fallback, Backspace/Enter/Esc while typing, and confirming the existing
curated `TOPIC` menu still works unchanged alongside the new "Ask about
something else..." row.
