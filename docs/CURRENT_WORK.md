# Current work

Nothing in flight.

Milestone 72 (character-level subject pools and day-gated knowledge) just
shipped -- see `docs/MILESTONES.md`'s Milestone 72 entry and
`docs/TIMELINE_NOTES.md`'s "Ask about anything" section for the full
mechanism, content, and citations. In brief: `SUBJECT_WHEN` grammar plus
character-level `SUBJECT`/`SUBJECT_UNKNOWN`, two new `Timeline` queries
(`subjectsFor`/`subjectUnknownFor`), a keyword-collision warning in
`TimelineLoader`, and a full Raistlin content pass -- his Solace-only
subjects promoted to character-level, fourteen new always-true subjects,
and gated Khisanth/Verminaard/Disks-of-Mishakal/true-gods content at the
`xak_tsaroth` day-4 boundary, plus an always-true Takhisis entry. Verified
via a throwaway self-test (deleted after use), a clean `/W4` rebuild (zero
new warnings), and two piped smoke tests (before and after the gated
content, confirming clean parses and exactly one -- deliberate --
collision warning each time). **Interactive verification of the free-text
prompt itself still needs the user's own keyboard** -- typing "Kitiara" at
Solace vs. at one of Raistlin's other seven windows to confirm the Solace
override still wins and the pool answer shows up elsewhere; typing
"Khisanth"/"Verminaard"/"the Disks" before day 4 (e.g. at Solace or Haven)
vs. after (Xak Tsaroth onward) to confirm the reaction differs; and
confirming the curated `TOPIC` menu and "Ask about something else..." row
both still behave.

A second Raistlin content group (`draconians`, a dedicated `fistandantilus`
name subject, `bupu`, `cyan`/`bloodbane`, `lorac`, `alhana`/`starbreeze`)
was deliberately left for a later pass at the user's own call -- already
sourced with citations, not a sourcing gap. See `docs/MILESTONES.md`'s
NEXT UP item 6 for the citations on file for each, and item 6 more broadly
for widening this mechanism to the other seven Heroes (Milestone 73).
