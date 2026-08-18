# Milestones

A numbered shipping history, reconstructed from `README.md`'s Status
paragraph and the explicit "Milestone N" references scattered across
`docs/*_NOTES.md` and `docs/ARCHITECTURE.md`. This repo has no git
history to cross-check against, so a handful of numbers below (marked
*inferred*) are placed by dependency order and cross-reference rather
than an explicit in-doc citation — treat those as best-effort, not
forensic fact. Going forward, log new milestones here as they ship so
this stays authoritative.

1. Text-adventure prototype — named locations connected by a road graph,
   moved between with typed `go <place>` commands. Superseded by
   Milestone 2 and no longer in the code.
2. Caves-of-Qud pivot — persistent, colored ASCII overworld walked in
   real time with immediate keypresses; the 480×320 terrain grid
   generated from the reference map image; `GameState` begins tracking
   `x`/`y`/`hoursElapsed`.
3. Walkable zone interiors — `Zone`/`ZoneLoader`/`ZoneCatalog`, starting
   with Solace's town square.
4. Character creation — race/class/ability-score/alignment rules and the
   interactive `CharacterCreator` wizard (3d6-down-the-line with free
   rerolls).
5. *(inferred)* Save/load — `SaveGame`, continuous per-keypress
   autosave, `save.txt`, continue-prompt on launch.
6. *(inferred)* Timeline engine v1 — the chance-encounter schedule
   introduced with Tanis and Raistlin's first `PRESENCE` content across
   four locations, plus the base `t` talk action for a single present
   character.
7. Dragonlance-specific character depth — mandatory Elf/Dwarf subraces,
   Tinker Gnome as a hard-enforced real class, Knights of Solamnia
   (Order of the Crown), Wizards of High Sorcery scoped correctly to
   "nothing to model at 1st level."
8. *(inferred)* Full 2e ruleset accuracy audit — PHB Table 2/3/8/43/60
   corrections, Constitution-scaled magic resistance, the Elf
   sleep/charm-resistance fix, Half-Orc replaced by Kender.
9. Combat system — attack/damage/initiative math, an initial monster
   roster, Attack/Flee actions, steel+XP rewards, and the "knocked out,
   not killed" death rule.
10. *(inferred)* Leveling — HP/THAC0/saving-throw progression (PHB
    Tables 53/60) through level 20, Knight of Crown → Sword flavor nod,
    Mage Test of High Sorcery / Robe assignment at level 3.
11. *(inferred)* Spellcasting — Mage and Cleric each gain one real,
    PHB-sourced known spell (Magic Missile / Cure Light Wounds) with
    real per-day slot counts.
12. *(inferred)* Equipment — Solace's General Store, sourced armor
    tiers, one weapon upgrade per class, shield.
13. *(inferred)* Zones for Haven and Xak Tsaroth, and the base zone-NPC
    `TALK` grammar (e.g. Haven's Seeker Guard).
14. Timeline/lore research pass confirming Tarsis and Plains of Dust's
    route material — found DL3 establishes Que-Shu as already destroyed,
    shaping later zone and timeline scope cuts.
15. *(inferred)* Zones for Darken Wood, Qualinesti, Pax Tharkas, Plains
    of Dust, and Tarsis — every named overworld location now has a
    walkable interior.
16. *(inferred)* The Inn of the Last Home's interior and the
    zone-nesting `PORTAL` mechanism (`GameState::zoneStack`).
17. All eight Heroes of the Lance added to the shared timeline schedule;
    the multi-candidate `pickAndTalk` picker introduced since more than
    one Hero can now share a stop.
18. Real "have I met them" tracking — `GameState::metCharacters`, a
    `MET` save line, shared by timeline characters and zone NPCs.
19. Reactive dialogue and branching topics (`SAY_IF`/`TOPIC`) at Solace,
    plus real `SAY_AGAIN`/`TALK_AGAIN` repeat-visit lines everywhere;
    the Chronicles and Legends novels added to the reference library.
20. Reactive dialogue widened to Xak Tsaroth and Qualinesti; Haven
    explicitly left out of scope (the party never actually enters it in
    *Dragons of Autumn Twilight*).
21. Carried inventory and equip/unequip screen (`i`) — purchases land in
    inventory instead of auto-equipping.
22. Currency renamed from Gold to Steel Pieces, Krynn's real
    post-Cataclysm currency, with a legacy-save compatibility fallback.
23. Zone-interior chance encounters — `TIMELINE_ANCHOR`, letting a
    canon character be found and talked to inside a zone (the Inn's
    fireplace, Haven's market stalls, etc.), not just on the overworld
    tile.
24. Darken Wood and Pax Tharkas added to the shared timeline schedule
    (six of eight locations now scheduled); Plains of Dust and Tarsis
    deliberately left out since *Dragons of Autumn Twilight* never
    actually goes there.
25. Reactive dialogue (`SAY_IF`/`TOPIC`) widened to Darken Wood and Pax
    Tharkas — 5 of 8 Heroes gained new content at the former, 2 of 8 at
    the latter; Haven and zone NPCs deliberately stayed out of scope (see
    `docs/TIMELINE_NOTES.md`).
26. Reactive dialogue (`SAY_IF`/`TOPIC`) widened to Haven — all 8 Heroes
    gained a `TOPIC`, 4 gained a `SAY_IF`, grounded in real *Dragons of
    Autumn Twilight* dialogue about the Highseekers/Seekers rather than
    an invented Haven visit — and, for the first time, to zone-native
    NPCs (Otik, Tika, the Seeker Guard, the Forestmaster, the Fortress
    Guard), which needed new `ZoneLoader`/`PointOfInterest`/`GameLoop`
    code since the grammar didn't exist for zones before this milestone.
    See `docs/TIMELINE_NOTES.md` and `docs/ZONE_NOTES.md`.
27. Three new monsters (Bugbear, Ogre, Kapak Draconian — the latter
    Krynn-specific, from *Dragonlance Adventures* like the Baaz), and
    random-encounter chance changed from a flat constant to a new
    per-terrain `TerrainInfo::encounterChancePercent` field (roads
    safest, forest/mountains riskiest). Along the way, found and fixed a
    stale doc comment in `combat::Monster` claiming stats were invented
    rather than sourced (a leftover from before the Milestone-era
    sourcing pass), and re-enabled random encounters at the user's
    request — they'd been live-disabled (`kEncounterChancePercent = 0`)
    since 2026-08-18, a temporary state that had drifted out of sync
    with `docs/COMBAT_NOTES.md`'s documented 8%. See
    `docs/COMBAT_NOTES.md`.
28. Equipment/shop expansion: sell-back (half the item's real shop price,
    an invented-but-flagged convention -- the actual PHB/DMG were checked
    and print no mundane-equipment resale rule) via a new "buy/sell"
    toggle (`i`) inside the existing shop screen, plus two new shops --
    Haven's Market Stalls and Tarsis's Old Sailor, both already-existing
    POIs that just gained a `SHOP` line rather than new, ungrounded
    merchant characters. All three shops share the identical catalog. See
    `docs/CHARACTER_NOTES.md` and `docs/ZONE_NOTES.md`.
29. Caves of Qud-style presentation overhaul: a wide (120x30) fixed frame
    replacing the old 78x24 one, with a top HUD (name/day-hour/steel, an
    ASCII HP bar, AC/THAC0) and a persistent, word-wrapped, scrolling
    40-column event log panel beside the map -- `GameLoop::message_` (one
    transient line, shown once then cleared) became `log_`
    (`std::vector<std::string>`, capped at 300 entries), rendered by a new
    `MapRenderer::buildLogPanel` helper. Applies to the two exploration
    frames only (overworld and zone) -- combat/shop/inventory/sheet/
    dialogue/picker keep their existing simple screens. A zone smaller
    than the viewport now wall-pads out to the full frame size rather
    than rendering at its native smaller size (relies on
    `Zone::tileCodeAt`/`poiAt`'s existing bounds-safe out-of-range
    behavior). Explicitly does not add dynamic terminal-resize handling
    -- still a fixed default size, just a wider one. See
    `docs/ARCHITECTURE.md`.
30. Folded the "standing here" description block (a Location's or zone
    POI's name/description plus any canon-character presence text) into
    the scrolling log panel instead of redrawing it full-width below the
    map every frame -- new `GameLoop::announceOverworldTile`/
    `announceZoneTile` push it once per arrival, and `MapRenderer` lost
    the `timeline::Timeline` parameter it no longer needs. Deliberately
    dropped the per-step "You are in grassland." terrain line and the
    always-redrawn zone-name heading rather than porting them into the
    log (would've either spammed every wilderness step or been fully
    redundant with the "You step into/back out into X." lines already
    logged). See `docs/ARCHITECTURE.md`.
31. Fixed the live log panel's biggest usability gap: it was tail-only,
    with no way to scroll back and re-read anything that had aged off.
    Bumped `MapRenderer::kViewportHeight` 20->30 (growing the live panel
    and the map camera together, since both already keyed off the one
    constant), and added a new dedicated, pageable full-history screen
    (`v`/`V`, `GameLoop::handleLog`/`MapRenderer::drawLogFrame`) that
    wraps and scrolls through the *entire* `log_`, not just the live
    panel's tail. `README.md`'s minimum terminal size moved to 120x36.
    See `docs/ARCHITECTURE.md`.
32. Plain-ASCII (`+`/`-`/`|`) window border on every screen, so the game
    feels like a contained app rather than raw text in a console.
    Unicode box-drawing was considered and rejected as a real
    compatibility risk (`Console.cpp` never enables UTF-8 output). Two
    families of screen (`MapRenderer.cpp`'s new `writeBorder`/
    `writeBoxed` helpers): the map/log frames and the log pager already
    build exact-width content (some of it carrying ANSI color codes) and
    border it directly; every other screen hugs its own content width via
    a new prose-wrapping pass. Entirely contained inside
    `MapRenderer.cpp` -- no signature changes, so `GameLoop` needed no
    changes at all. `README.md`'s minimum terminal size moved to 124x38.
    See `docs/ARCHITECTURE.md`.
33. Adaptive layout: the fixed frame size finally got checked against a
    real terminal and, predictably, didn't fit -- Milestones 29-32 had
    grown it (78x24 -> 120x30 -> 120x36 -> 124x38) without ever querying
    an actual console. New `Console::currentWindowSize()` reads the
    visible window (`srWindow`, deliberately not the taller scrollback
    `dwSize`) and `MapRenderer::configureLayout` sizes the frame to fit
    it once at startup, shrinking the map viewport toward a measured real
    floor (44x16, the widest/tallest authored zone) before ever shrinking
    the log panel below a 20-column minimum. Fails fast with a clear
    message below the absolute minimum (70x23) rather than rendering
    something broken. Adapts once at launch, not continuously -- live
    mid-session resize stays deferred. `docs/ZONE_NOTES.md`'s authoring
    ceiling corrected to the new fixed floor accordingly. See
    `docs/ARCHITECTURE.md`.

## NEXT UP

Not yet started — a short menu of well-grounded backlog candidates, not
a commitment. Pick one (or something else) before starting the next
session's work.

1. **More ordinary monsters** — the roster is at 9; the Monstrous Manual
   has more of Krynn's actual bestiary untouched (Gnolls, Ghouls,
   Skeletons/Zombies). See `docs/COMBAT_NOTES.md`'s "Extending this
   later."
2. **Terrain-specific monster pools** — encounter *chance* now varies by
   terrain (Milestone 27), but which monster you fight is still
   uniform-random regardless of terrain. See `docs/COMBAT_NOTES.md`'s
   "Extending this later."
