# Current work

Nothing in flight. Milestone 134 (World Map screen -- NEXT UP item 8: a
read-only, zoomed-out overview of the whole continent, `'o'`, with a new
`SIZE MEDIUM/LARGE` location grammar field, sourced this session against
`References/portcities.txt`/the novels/this project's own already-
researched location descriptions rather than fresh Atlas image research)
is implemented, documented, and verified (a throwaway `WorldLoaderSelfTest
.cpp` confirmed the new `SIZE` grammar and its fail-fast validation, clean
`/W4` rebuild with zero new warnings, piped character-creation smoke test
passed) -- see `docs/MILESTONES.md` entry 134 and `docs/MAP_NOTES.md`'s
"World Map screen" section for the full sourcing writeup, including a real
geography-driven layout problem (11 of the 25 locations cluster too
tightly for inline map labels) resolved with the user's input as a side
legend panel instead.

**Not yet interactively walked** -- same standing `_getch()` limitation
this project always discloses. Worth doing on the next playthrough: press
`o` and confirm the continent's silhouette reads correctly, the side
legend lists all 25 locations legibly, Palanthas/Thorbardin/Tarsis show a
visibly bigger footprint than an ordinary town, Kalaman/Neraka/Port
Balifor show a smaller (but still widened) footprint, the player's own
position shows as `@`, and any key closes the screen cleanly back to the
live view. Milestone 133's own still-unwalked ask-anything checklist
(Astinus's INT/WIS check, the 10-question hard cap, the identity-question
lockout, the Aesthetic's turn-away line -- see `docs/MILESTONES.md` entry
133) remains open too; both are good candidates for the same play session.
