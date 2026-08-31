# Current work

Nothing in flight. Milestone 127 (Foghaven Vale -- Huma's Tomb / the
Dragon Mountain) is implemented, documented, and verified -- see
`docs/MILESTONES.md` entry 127 and `docs/TIMELINE_NOTES.md`'s "Foghaven
Vale" section for the full sourcing.

Reviewed the world map against a newly-supplied reference,
`References/TSR 9400 TM3 World Of Krynn Trailmap.pdf`, which pointed at
two of Milestone 126's six unbuilt Atlas-named waypoints: Skullcap and
Dragon Mountain. Research against the three tracked Chronicles novels
found Skullcap isn't actually sourced to any of them (it's *Legends*-only
content, the ruin of Zhaman) -- dropped, with the doc corrected rather
than carried forward. Dragon Mountain checked out and shipped: a new
portal-nested zone, `data/zones/foghaven_vale.txt` (no new overworld
`LOCATION`, reached via a new `PORTAL M foghaven_vale` POI off `data/
zones/southern_ergoth.txt`), plus five new `PRESENCE foghaven_vale 69
72` windows (Laurana, Flint, Tasslehoff, and a second window each for
Fizban and Silvara).

Verified via a clean `/W4` rebuild (zero new warnings, no `.cpp`/`.h`
diff), `--check-timeline` (zero new keyword-collision warnings beyond the
one pre-existing, already-documented Raistlin/Kitiara override), and the
piped character-creation smoke test (real save1-3.txt moved aside and
restored afterward) -- confirms `WorldLoader`/`ZoneCatalog`/`Timeline`
all load the new zone/portal/PRESENCE content without throwing.
**Not yet interactively walked** -- reaching the new zone requires real
keypresses (`_getch()` can't be piped), so the actual in-game text/flow
(taking the new portal, standing on the `TIMELINE_ANCHOR`, talking to all
five canon characters) hasn't been read back by a human yet. Worth doing
on the next play session that reaches Southern Ergoth.
