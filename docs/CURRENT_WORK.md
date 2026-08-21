# Current work

Nothing in flight.

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

**Not yet verified**: real interactive playthrough -- walking to Pax
Tharkas, talking to the Ore Cart, carrying the ore to Solace, confirming
the journal shows progress and the quest turns in and consumes the item
-- `_getch()` can't be piped, the same limitation flagged for every quest/
combat milestone so far. Milestone 56's Webnet/Brooch and Milestone 57's
terrain-pool interactive verification are also still outstanding from
earlier sessions.

NEXT UP (`docs/MILESTONES.md`) now offers two candidates: more monsters
(Bozak/Sivak/Aurak Draconians, Thanoi, etc.) or more of DLA's "Magical
Items of Krynn" chapter. Ask the user before starting either.
