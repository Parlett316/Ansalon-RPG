# Current work

Nothing in flight.

Milestone 97 (2026-08-26) confirmed Milestone 92's boat decline flow and
Sancrist Isle -> Palanthas leg via real interactive playtesting for the
first time -- closing NEXT UP's longest-open item. Walked the full chain
(Tarsis's Knight's Runner, Ice Wall's Ice Barbarian Guide, Southern
Ergoth's Silvanesti Sentry, Sancrist Isle's Embarkation Officer) on the
user's real save slot 2: decline-then-topics-then-accept confirmed
correct at each stop, and the final leg's "northeast" direction line and
96-hour clock advance to Palanthas both confirmed correct. Along the
way, the user flagged the Embarkation Officer's dialogue as feeling thin
(7 named proper nouns, only 2 authored `SUBJECT` topics/3 replies) --
working as designed but worth expanding, so `derek,alfred,brightblade,
sturm` was split into three separate topics, each re-sourced from the
same trial/muster passage Milestone 92 already cited. Verified via a
clean rebuild, the piped smoke test against an isolated scratch copy,
and real interactive confirmation of the three new distinct replies.
Real saves backed up before the session began. See `docs/MILESTONES.md`
entry 97 and `docs/ZONE_NOTES.md`'s "Sancrist Isle" section.

Milestone 96 (2026-08-26) added Port Balifor and Flotsam: closes the
second real gap the Milestone 95 research pass turned up, and a
deliberate reversal of a "don't invent to fill a gap" call this project
made three separate times (Milestones 39, 49, 50) about this exact
stretch of story. Tanis, Raistlin, Caramon, Goldmoon, and Riverwind's
tracked schedule used to jump straight from `silvanesti 25 30` to
`palanthas 83 83`/`kalaman 100 100`, skipping ~70 in-game days that the
novels spend on real content: a month at Port Balifor (Raistlin's "Red
Wizard" illusion act, Goldmoon's healing reputation starting to spread),
then Flotsam, where Tanis is taken captive by a Dragon Highlord who turns
out to be Kitiara while the others wait out his absences. New
`LOCATION port_balifor`/`flotsam` (both directly legible on the reference
map), a small 3-POI Port Balifor zone and a fuller 4-POI Flotsam zone
(Saltbreeze Inn deliberately flavor-only -- no NPC stands in for
Kitiara, per Milestone 50's precedent), and matching
`PRESENCE port_balifor 35 64`/`flotsam 65 82` windows for all five
Heroes. `("neraka", "flotsam")`/`("flotsam", "port_balifor")` added to
`ROAD_PAIRS`; 5 true-water tiles patched via `MANUAL_TERRAIN_OVERRIDES`
as short fords, re-verified at 0 after regenerating. Confirmed (not
assumed) that Ice Wall's old glacier-patch-reapplication caveat no longer
applies -- retired at Milestone 85. Verified via a throwaway self-test
(49 assertions), a clean `/W4` rebuild, and the piped smoke test with the
user's real saves confirmed byte-identical afterward. Interactive
verification of Port Balifor/Flotsam's own new roads and POIs (as
distinct from Milestone 97's playtest, which covered the older Tarsis ->
... -> Palanthas boat chain) still needs the user's own keyboard. See
`docs/MILESTONES.md` entry 96, `docs/MAP_NOTES.md`/`docs/ZONE_NOTES.md`/
`docs/TIMELINE_NOTES.md`'s "Port Balifor and Flotsam" sections.

**Related, not started**: Laurana's own later Flotsam/Dargaard Keep
captivity (between `kalaman 90 92` and `neraka 105 107`) still has no
`LOCATION`/`PRESENCE` window -- a real, still-open gap flagged in
`docs/TIMELINE_NOTES.md`'s Laurana section, not resolved by this
milestone.
