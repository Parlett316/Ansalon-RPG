# Current work

Nothing in flight.

Milestone 61 (Ariakas and Lord Soth) just shipped: a sourcing pass that
overturned its own premise. `docs/MILESTONES.md`'s NEXT UP had listed them
as "the last still-off-stage canon-character pair," needing the same
retrospective-`TOPIC` treatment Kitiara/Derek/Gunthar got. Re-checking
`.research/dosd_full.txt`'s Crown of Power sequence at Neraka (the
already-existing `neraka 105 107` window) found they aren't off-stage at
all -- Tanis personally kills Ariakas on-page, and Lord Soth appears in
person in the same scene. The existing `TOPIC "The Crown of Power"`
(Tanis, Milestone 50) already dramatized part of this scene but omitted
both, and its ending overstated a clean resolution when what actually
happens is Laurana breaking free on her own. This milestone corrected
that `TOPIC` in place and added one new `TOPIC` each for Tanis
("What Kitiara Asked For") and Laurana ("Free By My Own Hand"), plus an
in-place edit to Laurana's existing `TOPIC "Before the Dark Queen"`. Pure
data, no `.cpp`/`.h`/grammar changes. See `docs/MILESTONES.md`'s
Milestone 61 entry and `docs/TIMELINE_NOTES.md`'s "Ariakas and Lord Soth"
section for the full sourcing and scope notes.

Verified via a clean rebuild (zero new warnings) and the piped
character-creation smoke test (confirms `TimelineLoader` parses the
edited/new `TOPIC` lines without throwing; the user's real `save.txt` was
moved aside before the test and restored after). No throwaway self-test
needed (pure data, no new grammar). **Not yet interactively verified** in
a real playthrough -- the new/edited content lives in the day-105-107
`neraka` window, not readily reachable without a long playthrough; the
user can check it live if/when their save reaches that point.

The off-stage canon-character roster is now fully closed out (Kitiara,
Fizban, Laurana, Derek Crownguard, Lord Gunthar, Ariakas, Lord Soth all
sourced). NEXT UP (`docs/MILESTONES.md`) offers two remaining candidates:
more monsters (Bozak/Sivak/Aurak Draconians, Thanoi, etc.), or more of
DLA's "Magical Items of Krynn" chapter. Ask the user before starting
either.
