# Current work

Nothing in flight. Milestone 133 (Astinus follow-up: comprehensive
NPC/place/event `SUBJECT` coverage, an Intelligence+Wisdom check that can
extend his 5-question patience to a hard cap of 10, a harder
`SUBJECT_ENDS`-based refusal on direct identity questions that also locks
him out for the rest of the day, a new `ASK_ANYTHING` flag that suppresses
the hint list for a POI meant to feel like it can answer anything, a new
`ASK_LIMIT_LOCKED` field so a named Aesthetic -- not Astinus himself --
turns the player away once today's audience is used up, and a follow-up
fix adding a missing `SUBJECT L laurana` entry the user caught by trying
it) is implemented, documented, and verified (clean `/W4` rebuild x4, an
extended throwaway `ZoneLoaderSelfTest.cpp` confirming the new grammar and
every fail-fast pairing case, piped character-creation smoke test passed
x4) -- see `docs/MILESTONES.md` entry 133 and the matching sections in
`docs/ZONE_NOTES.md` for the full writeup.

The user expects to keep expanding Astinus's `SUBJECT` pool incrementally
across future sessions (more NPCs/places/events as gaps are noticed) --
treat a request like "add X to Astinus" as expected, routine content work
on an already-designed system, not a new feature needing its own
architecture pass.

**Not yet interactively walked** -- same standing `_getch()` limitation
this project always discloses. Worth doing on the next playthrough: talk
to Astinus at the Great Library in Palanthas and try asking about a
Hero (e.g. "tanis"), a place (e.g. "thorbardin"), and Kitiara, to confirm
the expanded pool answers and that the ask prompt shows no suggestion
list at all; ask 5 questions and confirm the INT/WIS check attempt (roll
outcome varies by character stats); if extended, keep asking to 10 and
confirm the hard stop fires with no further check; then walk back into
the library that same day and confirm an Aesthetic (not Astinus) turns you
away with no ask option offered at all. Separately, in a fresh
conversation, try "are you Gilean", "are you a god", or "who are you" to
confirm each ends the conversation immediately *and* that walking back in
the same day gets the Aesthetic's turned-away line too, while "tell me
about Gilean" and "tell me about the gods" still get real answers.
