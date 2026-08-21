# Current work

Nothing in flight.

Milestone 59 (Alhana Starbreeze) just shipped: a new `CHARACTER alhana`
block in `data/timeline.txt`, sharing the existing `silvanesti 25 30`
window with the 5 Heroes already there via the zone's existing
`TIMELINE_ANCHOR` picker -- pure data, no `.cpp`/`.h`/zone-grammar
changes. See `docs/MILESTONES.md`'s Milestone 59 entry and
`docs/TIMELINE_NOTES.md`'s "Alhana Starbreeze" section for the full
sourcing and scope notes.

Verified via a clean rebuild (zero new warnings) and the piped
character-creation smoke test (confirms `TimelineLoader` parses the new
block without throwing); no throwaway self-test needed (pure data, no new
grammar). **Not yet interactively verified** in a real playthrough --
the user can visit Silvanesti (`silvanesti 25 30`) in their real save to
read Alhana's dialogue live if they want to confirm it.

NEXT UP (`docs/MILESTONES.md`) now offers three candidates: more monsters
(Bozak/Sivak/Aurak Draconians, Thanoi, etc.), more of DLA's "Magical
Items of Krynn" chapter, or more off-stage canon characters (Derek
Crownguard/Lord Gunthar, Ariakas/Lord Soth -- retrospective-dialogue
treatment, same as Kitiara). Ask the user before starting any of them.

Milestone 58 (`DELIVER`/item objectives) just shipped: a real, granted-
in-the-world quest-item concept (`character::ItemKind::QuestItem`), a new
`quest::ObjectiveKind::Deliver`, and a `GRANTS_ITEM` zone-POI mechanism
mirroring `BOAT` -- plus one proof-of-concept quest, `ore_for_the_forge`
(Pax Tharkas's new Ore Cart POI grants `raw_tharkadan_ore`; Solace's
Flint's Smithy, previously scenery, now has a talkable journeyman who
wants it delivered). See `docs/MILESTONES.md`'s Milestone 58 entry,
`docs/QUEST_NOTES.md`'s "DELIVER"/"Shipped: ore_for_the_forge", and
`docs/ZONE_NOTES.md`'s "Quest items: POIs that grant a DELIVER object"
for the full design and sourcing.

Milestone 58 is now **interactively verified** (2026-08-20): the user
played it live, including an out-of-order edge case (grabbed the ore
from Pax Tharkas's Ore Cart before ever talking to Solace's journeyman)
that still turned in cleanly on first contact. See `docs/QUEST_NOTES.md`'s
"Shipped: ore_for_the_forge".

Milestones 56 (Webnet/Brooch) and 57 (terrain pools) are now also
**interactively verified** (2026-08-20) by the user. See
`docs/MILESTONES.md`'s Milestone 56/57 entries for what was specifically
confirmed. Nothing left outstanding from earlier sessions.

NEXT UP (`docs/MILESTONES.md`) now offers two candidates: more monsters
(Bozak/Sivak/Aurak Draconians, Thanoi, etc.) or more of DLA's "Magical
Items of Krynn" chapter. Ask the user before starting either.
