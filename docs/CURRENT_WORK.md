# Current work

Nothing in flight.

Milestone 60 (Derek Crownguard and Lord Gunthar) just shipped: two new
`TOPIC` entries in `data/timeline.txt` -- Laurana's `TOPIC "The Writ of
Vindication"` and Sturm's `TOPIC "Derek's Last Charge"`, both in the
already-existing `high_clerist_tower 76 80` window -- the Kitiara treatment
(retrospective dialogue, no talkable `CHARACTER` of their own) applied to
the next pair on the off-stage roster. Pure data, no `.cpp`/`.h`/grammar
changes. See `docs/MILESTONES.md`'s Milestone 60 entry and
`docs/TIMELINE_NOTES.md`'s "Derek Crownguard and Lord Gunthar" section for
the full sourcing and scope notes.

Verified via a clean rebuild (zero new warnings) and the piped
character-creation smoke test (confirms `TimelineLoader` parses the new
`TOPIC` lines without throwing); no throwaway self-test needed (pure data,
no new grammar). **Not yet interactively verified** in a real playthrough --
both new `TOPIC`s live in the day-76-80 `high_clerist_tower` window, not
readily reachable without a long playthrough; the user can check them live
if/when their save reaches that point.

NEXT UP (`docs/MILESTONES.md`) now offers two candidates: more monsters
(Bozak/Sivak/Aurak Draconians, Thanoi, etc.), or more of DLA's "Magical
Items of Krynn" chapter. Ariakas/Lord Soth (the remaining off-stage
canon-character pair) is still on the table too but needs its own sourcing
pass first, same as Derek/Gunthar did this session. Ask the user before
starting any of them.
