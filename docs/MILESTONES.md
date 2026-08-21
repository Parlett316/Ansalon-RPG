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

34. Four more ordinary Monstrous Manual monsters -- Gnoll, Ghoul,
    Skeleton, Zombie -- bringing the roster to 13. Sourced from
    `Monster Manual (2nd ed).pdf`, but with a real caveat: no PDF-page-
    image renderer was available that session, so these four were
    cross-checked between two independent text extractions
    (`pdftotext -table` and `-raw`) instead of the usual visually-
    confirmed page image. Each monster's real standout trait (the
    Ghoul's paralyzing touch, the Skeleton's edged-weapon resistance,
    the Zombie's spell immunities) is left unmodeled as flavor-only text,
    same restraint already applied to the Baaz's magic resistance and
    the Kapak's paralysis-poison bite -- no status-effect system exists
    for anyone yet. Pure data addition to `data/monsters.txt`, no source
    changes. See `docs/COMBAT_NOTES.md`.

35. The High Clerist's Tower -- the first content beyond *Dragons of
    Autumn Twilight*, starting *Dragons of Winter Night*'s Solamnia arc
    (Sturm's Knighting and death). New `LOCATION high_clerist_tower`
    (Solamnia, reached by a new long Solace-rooted road through the
    Vingaard Mountains) and its own zone (a courtyard/chapel/battlements/
    sealed-inner-doors layout, a generic Garrison Knight NPC). All 8
    Heroes gain a shared `tarsis` window (day 20-22) for the first time,
    since Autumn Twilight never reaches Tarsis but Winter Night opens
    there -- the party then genuinely splits for the first time in this
    project: only Sturm, Flint, and Tasslehoff continue on to the Tower
    (day 76-80); Tanis, Raistlin, Caramon, Goldmoon, and Riverwind are
    carried to Silvanesti instead, not modeled yet, so their schedules
    simply end at Tarsis rather than inventing a stop. Sancrist Isle
    (where Sturm's actual Knights' Trial happens) stays unmodeled too --
    an ocean-locked island with no sea-travel mechanic in this engine --
    folded into the Tower's Knight NPC as retrospective dialogue instead.
    Verified via clean rebuild, the piped character-creation smoke test,
    and a throwaway self-test asserting `Timeline::presentAt`'s day-range
    correctness (no in-game way to fast-forward to day 76+ to check this
    live). See `docs/MAP_NOTES.md`, `docs/ZONE_NOTES.md`,
    `docs/TIMELINE_NOTES.md`.

36. Ice Wall Castle -- the second Winter Night arc, and the first
    milestone to add a real engine feature rather than pure content: sea
    travel. Feasibility check (cropping the reference map, same method as
    Milestone 35) found both Ice Wall and Southern Ergoth genuinely
    sea-locked, the same situation Milestone 35 hit with Sancrist Isle --
    presented to the user as a choice, and the user chose to build a real
    `GameState::hasBoat` mechanic rather than fold the content into
    dialogue. New `LOCATION ice_wall` (no `ROAD_PAIRS` entry -- sea-only
    access, the whole point), a small hand-painted glacier patch in
    `data/overworld.grid` (the classifier has never produced glacier
    anywhere), and its own zone (an ice-bound silver dragon with a
    mysterious rider, left as unresolved foreshadowing same as the novel;
    where the dark elf Feal-thas fell; a generic Knight NPC). A new
    `data/zones/tarsis.txt` POI (`R`, "A Knight's Runner") grants the boat
    -- deliberately not the existing Old Sailor, whose dialogue already
    establishes Tarsis's harbor as dead. Southern Ergoth stays unmodeled:
    checking the actual text found the ship only sails *past* it, never
    lands -- folded into the Ice Wall Knight's dialogue as one flavor
    line instead of inventing a landing that doesn't happen in the book.
    Sturm/Flint/Tasslehoff gain an `ice_wall 38 42` window between their
    existing `tarsis`/`high_clerist_tower` stops. Verified via clean
    rebuild (this milestone touches `.cpp`/`.h` files, not just data),
    the piped smoke test, a throwaway self-test, and confirming the
    user's real save (predating the new `BOAT` save line) still loads.
    See `docs/MAP_NOTES.md`, `docs/ZONE_NOTES.md`,
    `docs/TIMELINE_NOTES.md`, `docs/ARCHITECTURE.md`.

37. Silvanesti -- the third and final Winter Night arc, completing the
    party split Milestone 35 started: Tanis, Raistlin, Caramon, Goldmoon,
    and Riverwind's own destination after Tarsis, griffon-carried east
    into a second dragon-orb crisis (the elf-king Lorac Caladon, trapped
    and tormented by an orb he couldn't control, guarded by the green
    dragon Cyan Bloodbane). Unlike Ice Wall, no new engine feature was
    needed -- checking the reference map found Silvanesti is
    river-bounded, not ocean-locked, matching the source text's own ferry
    crossing on foot. New `LOCATION silvanesti` (a ~127-tile invented
    road east from Solace -- the biggest gap yet between "sourced" and
    "drawn," since the party actually arrives by griffon, not any
    walkable route) and its own zone depicting Silvanost (the Ferry
    Landing, the Tower of the Stars, the nightmare-corrupted Twisted
    Gardens, a generic Warder NPC -- deliberately not Alhana Starbreeze
    by name, same reasoning that's kept Derek Crownguard and Gunthar
    off-stage). Regenerating the overworld grid for the new road silently
    wiped Milestone 36's hand-painted Ice Wall glacier patch, exactly the
    caveat `docs/MAP_NOTES.md` already documented -- reapplied
    identically afterward. The five Heroes gain a `silvanesti 25 30`
    window right after their shared `tarsis` stop. Verified via clean
    rebuild (pure data again, no `.cpp`/`.h` changes), the piped smoke
    test, and a throwaway self-test. See `docs/MAP_NOTES.md`,
    `docs/ZONE_NOTES.md`, `docs/TIMELINE_NOTES.md`.

38. The siege of the High Clerist's Tower and Sturm's death -- the bridging
    event between *Dragons of Winter Night* (this project's existing
    content) and *Dragons of Spring Dawning* (the next arc), needed because
    Spring Dawning opens with Sturm already dead but the timeline had no
    death event modeled. Sourced directly from
    `Dragons_of_Winter_Night_-_Margaret_Weis.pdf` (not from Spring Dawning's
    own backward references to it, and not from memory): Sturm draws the
    attacking dragons onto himself alone on the Tower's high wall, buying
    Laurana and Tasslehoff the seconds needed to spring the dragon-orb
    ambush that breaks the siege, and falls to a Dragon Highlord's spear.
    Pure data again -- a single-day `PRESENCE high_clerist_tower 81 81`
    window added for Sturm, Flint, and Tasslehoff in `data/timeline.txt`,
    no `.cpp`/`.h`/zone changes. Sturm's window deliberately has no `SAY`,
    the first deliberate use of the existing "no SAY = not talkable"
    mechanic to represent a character unavailable because he's dying in the
    scene, and it's also his schedule's permanent last window -- no new
    "character is dead" state was needed, since having no further
    `PRESENCE` entries already means "no longer encounterable." The Dragon
    Highlord who kills him (Kitiara, in the source text) stays unnamed, same
    off-stage-major-character precedent as Alhana/Derek/Gunthar. Verified
    via a throwaway self-test asserting the day-81 window's contents and
    that Sturm's schedule truly ends there, a clean rebuild (zero new
    warnings, no source changes), and the piped smoke test. See
    `docs/TIMELINE_NOTES.md`.

39. Kalaman -- the first *Dragons of Spring Dawning* content, and the
    biggest single content milestone to date: a new `LOCATION kalaman`
    (`POS 262 73`, `REGION Estwilde`, placed by cropping the reference map
    at full resolution and cross-checking against the already-known
    High Clerist's Tower position -- see `docs/MAP_NOTES.md`), a new
    Tarsis-scale zone (harbor, market square, a locked cartographer's
    stall grounding a real Tasslehoff pickpocketing beat, a stair to the
    city wall, the Lord's Keep, one generic City Watchman -- see
    `docs/ZONE_NOTES.md`), and `data/timeline.txt` windows for six of the
    eight Heroes across two sourced story beats: the Spring Dawning
    festival (`kalaman 90 92`, Flint and Tasslehoff only) and the reunion
    with Tanis/Caramon/Goldmoon/Riverwind under a Dragon Highlord's
    ultimatum (`kalaman 100 100`, all six). Raistlin gets no window here --
    he escapes to Palanthas via the dragon orb and is never physically at
    Kalaman in this arc. The `("high_clerist_tower", "kalaman")` road
    addition required regenerating `data/overworld.grid`, which wiped Ice
    Wall's hand-painted glacier patch again (as documented, and already hit
    once at Milestone 37) -- this time its 46 tile coordinates were
    captured to a scratch file before regenerating and reapplied with a
    byte-for-byte diff check afterward. The Dragon Highlord delivering the
    ultimatum (Kitiara) and the captured Golden General (Laurana) both
    stayed unnamed at this milestone -- Laurana is named as of Milestone
    49, including a `kalaman 90 92` window of her own; Kitiara remains
    off-stage (see NEXT UP). Verified via a throwaway self-test, a visual check of the
    regenerated grid (road connectivity, glacier patch intact), a clean
    rebuild (zero new warnings, no `.cpp`/`.h` changes), and the piped smoke
    test. See `docs/MAP_NOTES.md`, `docs/ZONE_NOTES.md`,
    `docs/TIMELINE_NOTES.md`.

40. Rest and spell memorization -- the first HP-recovery mechanic outside
    combat healing/leveling, and the first explicit spell-memorization
    action. A new `render::Key::Rest` (`'r'`/`'R'`, `GameLoop::handleRest`)
    rests once per in-game day (`Character::lastRestDay`): advances
    `hoursElapsed` by 8 and heals 1 hp capped at `maxHp` -- the 2nd ed. DMG's
    base natural-healing rate (p.74, "Characters heal naturally at a rate
    of 1 hit point per day of rest"), not its faster 3 hp/day
    "complete bed-rest" tier (left for a future Inn-gated variant). For a
    Mage or Cleric, the same keypress also (re-)memorizes their one known
    spell via the new `character::memorizeSpells`, folding the PHB's
    real two-step requirement -- a restful night's sleep (p.107, Wizard;
    p.111, Priest: "identical to those needed for the wizard's studying")
    then 10 minutes of study per spell level -- into one action, since
    with only one known spell per caster there's nothing to actually
    *select*. `character::hasSpellSlotAvailable` lost its old silent
    auto-refill-on-a-new-day behavior and is now a pure query: no slots
    are available at all until memorization has actually happened that
    day. The character sheet's Spells line reflects this ("not memorized
    today -- rest to prepare" vs. "N/N remaining today"). Verified via a
    throwaway self-test (14 assertions covering Mage/Cleric/Fighter across
    memorize/cast/day-rollover), a clean rebuild (zero new warnings), and
    the piped smoke test. See `docs/CHARACTER_NOTES.md`'s "Spellcasting"
    section for the full sourcing and scope cuts.
41. Inn-gated complete bed rest -- the faster healing tier Milestone 40
    deferred. A new `render::Key::BedRest` (`'z'`/`'Z'`, not `'b'` --
    already `SouthWest` in the `yubn` diagonal-movement scheme;
    `GameLoop::handleBedRest`) works only standing on a zone POI newly
    markable `BED <char>` (`world::PointOfInterest::isBed`, parsed in
    `ZoneLoader` exactly like `SHOP`, no `TALK` prerequisite). Shares
    `Character::lastRestDay` with ordinary Rest (one overnight action per
    day, whichever kind), advances `hoursElapsed` by the same 8 hours, but
    heals fully to `maxHp` instead of 1 hp. This is a deliberate
    simplification of the 2nd ed. DMG's literal "complete bed-rest" rule
    (p.74: 3 hp/day, plus a Constitution hit-point bonus per full week) --
    taken literally that's a multi-day-to-multi-week grind, a poor fit for
    a game that tracks canon characters moving on a real schedule the
    player can walk past and miss (see `docs/TIMELINE_NOTES.md`); one
    full-heal action at an Inn was chosen instead, deviation called out
    explicitly rather than presented as a transcription. `data/zones/
    solace_inn.txt`'s existing `U "The Stairs Up"` POI (already
    flavor-texted as leading to private rooms, already noted as "not a
    modeled zone yet") carries the new `BED U` line -- no other zone has an
    authored Inn/lodging POI, so no other zone got one. Verified via a
    clean rebuild (zero new warnings) and the piped smoke test (confirms
    the new `BED` grammar still parses); no throwaway self-test, since
    `handleBedRest` has no new pure/extractable logic beyond what the
    build and a real playthrough already cover. See
    `docs/CHARACTER_NOTES.md`'s "Rest and spell memorization" and
    `docs/ZONE_NOTES.md`'s "Beds" section.
42. Healing potions -- on-demand healing, including mid-fight, beyond Rest/
    Bed Rest's once-a-day limits. A new `ItemKind::Potion` (one real item,
    Potion of Healing, `character::Equipment`), sold at every shop's
    already-shared catalog: 2d4+2 hp (2nd ed. DMG p.142, clearly legible
    in the scan) for 200 stl (the DMG's own Magical Items treasure table,
    p.134, "Healing" row, 200gp, applied via the established Gold -> Steel
    convention). Potion of Extra Healing was deliberately left out -- its
    dice were present in the same scan but badly OCR-garbled and not
    independently confirmable (`pdftoppm` isn't installed in this
    environment, so page-image rendering wasn't available to double-check
    it). **Lore framing, not a new mechanic**: Dragonlance canon has real
    clerical healing magic gone from Krynn until Goldmoon's Disks of
    Mishakal, early in *Dragons of Autumn Twilight*'s own timeline, so the
    shop's potion is framed as a scavenged pre-Cataclysm relic rather than
    a merchant's own brew -- the DMG item itself is unmodified. Stackable
    (buying a second is allowed on purpose, unlike armor/weapons) and
    sellable back at the usual invented half-price convention. Drinkable
    two ways: `'i'` + `Enter` on a carried potion in the inventory screen
    (which now also shows an `HP: current/max` line, previously absent, so
    the effect is visible immediately), or `'i'` mid-combat
    (`GameLoop::runCombat`, reinterpreted locally as "drink the first
    potion carried" instead of opening the inventory screen -- the same
    "local key reinterpretation" trick `handleShop` already uses for this
    exact key elsewhere) as the round's action instead of attacking, same
    shape as `Cast`. Verified via a throwaway self-test (21 assertions
    covering heal-amount range, maxHp capping, invalid-index handling, the
    shop catalog's index arithmetic for a class with a weapon upgrade and
    one without, and resale value; deleted after passing), a clean
    rebuild (zero new warnings), and the piped smoke test. See
    `docs/CHARACTER_NOTES.md`'s "Potions" and `docs/COMBAT_NOTES.md`'s
    "Player actions" section for full sourcing.
43. Frameless overworld/zone layout + NPC "Look" -- a visual restyle
    (`drawOverworldFrame`/`drawZoneFrame` only) matching a reference
    screenshot the user provided: no outer box border, a single-line
    header (title left, name/class/HP bar/day-hour right) instead of the
    old 2-line HUD, `=`/`-` rule dividers instead of `+--+`/`| |`, and a
    labeled status panel (`MODE`, `Standing On:`, `Position:`, the
    relocated AC/THAC0/Steel figures, then a headed `ACTION LOG:`) instead
    of a bare log tail. `MapRenderer::buildStatusPanel`/`writeHeaderLine`/
    `colorLine` are new; `writeHud` is gone. `buildLogPanel` now prefixes
    each log entry `"> "` (continuation lines `"  "`). Layout constants
    changed (`kChromeRows` 7->4, `kChromeColumns` 4->1, `kLogPanelGap`
    2->3, `kMinLogPanelWidth` 20->24), moving the absolute minimum terminal
    size from 70x23 to 72x20. Bundled with an unrelated but overlapping-
    scope request: an NPC no longer auto-prints its full description on
    arrival, just `"<Name> is here."` -- the description now shows via
    Look (`;`), and Look offers a `drawPickerFrame` picker ("Look at
    whom?") when more than one NPC is present, via a new
    `GameLoop::pickAndLook`/`LookCandidate` pair mirroring `pickAndTalk`/
    `TalkCandidate`. Scenery zone POIs (no `TALK` line) are unaffected --
    still described immediately, since Look gives them no other reveal
    path. Verified via a throwaway self-test capturing the restyled frame
    at both a comfortable size and the new 72x20 floor (visually confirmed
    the header/rules/status-panel/log-prefix alignment), a clean rebuild
    (zero new warnings), and the piped smoke test. See
    `docs/ARCHITECTURE.md`'s "Frameless overworld/zone layout + NPC
    'Look'", `docs/ZONE_NOTES.md`'s "NPCs: POIs you can talk to", and
    `docs/TIMELINE_NOTES.md`'s "How presence is shown".
44. Palanthas -- the second *Dragons of Spring Dawning* content milestone,
    resolving a gap Milestone 39 left open: Raistlin escapes the Blood Sea
    maelstrom alone via the dragon orb with nowhere modeled to land until
    now. A new `LOCATION palanthas` (`POS 167 85`, `REGION Solamnia`,
    placed by a calibrated-gridline crop cross-checked against the already-
    placed `high_clerist_tower`, which surfaced a previously undocumented
    (+5,+4) bias between that location's recorded `POS` and its actual map
    icon -- see `docs/MAP_NOTES.md`), a new 40x16 zone (the Great Library,
    the Tower of High Sorcery, the Shoikan Oak Grove, Lord Amothus's map
    room, the Old City Wall, the Harbor, and a Knight of the Watch -- see
    `docs/ZONE_NOTES.md`), and `data/timeline.txt` windows for three
    characters sharing one sourced arrival scene: Raistlin
    (`palanthas 83 83`, no `SAY`, deliberately mirroring Sturm's own death
    window -- his on-page fate stays as ambiguous here as in the book) and
    Flint/Tasslehoff (`palanthas 83 89`, filling the one open gap in their
    existing schedule between the Tower and Kalaman). All three windows
    anchor at the Great Library because the source text puts them there
    together (Tasslehoff witnesses Raistlin carried in); Astinus is a
    separate, permanent, timeline-independent NPC on that same tile,
    sourced from both the novel and the Players Guide. Caramon, Tanis,
    Goldmoon, and Riverwind get no window -- confirmed absent from
    Palanthas by direct text search. The `("high_clerist_tower",
    "palanthas")` road addition required regenerating `data/overworld.grid`
    (Ice Wall's 46-tile glacier patch captured before regenerating and
    diff-confirmed byte-for-byte identical afterward, same procedure
    Milestone 39 established); the new road turned out to cross ordinary
    terrain the whole way, clearing a nearby Blood-Sea-classified pocket
    without needing any special handling. Verified via a throwaway
    self-test (17 assertions covering location/zone loading and
    `Timeline::presentAt` across the day-83/85/82/90 boundaries), a clean
    rebuild (zero new warnings, no `.cpp`/`.h` changes -- pure data content,
    same as Kalaman), and the piped smoke test. See `docs/MAP_NOTES.md`,
    `docs/ZONE_NOTES.md`, `docs/TIMELINE_NOTES.md`.

45. Godshome -- the fourth *Dragons of Spring Dawning* content milestone,
    and Flint Fireforge's death: the last living member of the party's
    original trio (with Tanis and Tasslehoff) since Milestone 1. A new
    `LOCATION godshome` (`POS 267 139`, `REGION Taman-Busuk`, placed by
    the first fully-legible reference-map read yet -- no pixel-bias
    correction needed, see `docs/MAP_NOTES.md`) and a new, deliberately
    sparse 40x16 zone (three POIs: the Narrow Cleft entrance, the Circle
    of Standing Stones, and Where Flint Fell -- the `TIMELINE_ANCHOR` --
    see `docs/ZONE_NOTES.md`). Sourced from a fresh `pdftotext -layout`
    extraction of the actual Godshome chapter: Flint's death is a heart
    attack, not violence, foreshadowed earlier the same chapter and
    misread by Tanis in the moment as an attack by Berem (who is actually
    catching Flint as he falls) -- Tanis stabs the immortal Berem first in
    blind grief-rage, the first mention in this project of his nature as
    "the Everman," established only as far as the scene requires. A
    single-day `PRESENCE godshome 103 103` window was added to Tanis,
    Caramon, Flint, and Tasslehoff (the four Heroes confirmed present;
    Goldmoon and Riverwind are confirmed staying behind at Kalaman in the
    same scene, Raistlin already resolved separately at Palanthas) --
    Flint's window has no `SAY`, the third use of this project's
    established "no SAY = scripted death" device after Sturm and
    Raistlin, and his schedule's permanent last entry. The old mage who
    carries Flint's body away (Fizban, in the source text) and Berem both
    stayed unnamed at this milestone -- the old mage is named as of
    Milestone 48. The `("kalaman", "godshome")` road
    addition required regenerating `data/overworld.grid`; Ice Wall's
    46-tile glacier patch was captured before regenerating and reapplied
    afterward, same procedure as every prior `ROAD_PAIRS` change since
    Milestone 37. Verified via a throwaway self-test (zone/location load,
    `Timeline::presentAt` day-102/103/104 boundary correctness, Flint's
    empty `SAY`), a clean rebuild (zero new warnings, no `.cpp`/`.h`
    changes -- pure data content), and the piped smoke test. See
    `docs/MAP_NOTES.md`, `docs/ZONE_NOTES.md`, `docs/TIMELINE_NOTES.md`.

46. Neraka -- the climax and denouement of *Dragons of Spring Dawning*,
    compressed from the entire back half of the novel (infiltration,
    Tanis's captivity with Kitiara, Caramon's solo trial in the dungeons,
    Berem's final death, the Temple's destruction, the war's end, the
    party's reunion -- `pdftotext -layout` lines ~10440-15532) into one
    walkable zone and one multi-day `PRESENCE` window, the same
    "restraint over completeness" compression already used for Pax
    Tharkas, Ice Wall, and Godshome. A new `LOCATION neraka` (`POS 273
    140`, `REGION Taman-Busuk`, ~6 grid-units east of Godshome, matching
    the book's own geography -- see `docs/MAP_NOTES.md`) and a new,
    fully-walled 40x16 zone (four POIs: the Temple of the Dark Queen, the
    Dungeons, a talkable Deserting Guard reflecting the dragonarmies'
    collapse, and the Temple Square -- the `TIMELINE_ANCHOR` -- see
    `docs/ZONE_NOTES.md`). A `PRESENCE neraka 105 107` window (an
    ordinary multi-day window, not the single-day "no SAY" device -- all
    three survive) was added to Tanis, Caramon, and Tasslehoff, each
    closing a thread seeded in an earlier milestone: Tanis's Godshome-era
    "The Man Who Wouldn't Die" (Berem's death is now final and complete),
    Caramon's Kalaman-era "The Brother He Can't Watch" (a real, direct
    reunion with Raistlin in the dungeons -- described in Caramon's own
    dialogue, not added as a `PRESENCE` entry to Raistlin's own schedule,
    which stays exactly where Milestone 44 left it), and Tasslehoff's
    grief over Flint from Godshome (a warm, invented-in-character story
    about Flint waiting patiently by Reorx's forge). Kitiara, Laurana,
    Ariakas, and Lord Soth stayed off-stage at this milestone, referenced
    only descriptively -- Laurana is named as of Milestone 49, with her
    own account of this same `neraka 105 107` window; Kitiara, Ariakas,
    and Lord Soth remain off-stage (see NEXT UP). "Fizban" (named as of
    Milestone 48) stayed unnamed here even though the source text
    explicitly confirms his identity, keeping this project's own
    established restraint rather than retroactively naming a god the game
    had never named. The
    `("godshome", "neraka")` road addition required regenerating
    `data/overworld.grid`; Ice Wall's 46-tile glacier patch was captured
    before regenerating and reapplied afterward, same procedure as every
    prior `ROAD_PAIRS` change since Milestone 37. Verified via a
    throwaway self-test (21 assertions covering location/zone loading,
    `TIMELINE_ANCHOR` resolution, `Timeline::presentAt` across the
    day-103/104/105-107/108 boundaries, and confirming Raistlin's and
    Flint's schedules are untouched), a clean rebuild (zero new warnings,
    no `.cpp`/`.h` changes -- pure data content), and the piped smoke
    test. See `docs/MAP_NOTES.md`, `docs/ZONE_NOTES.md`,
    `docs/TIMELINE_NOTES.md`.
47. The ridge farewell -- Milestone 46's own NEXT UP entry pitched a
    closing `PRESENCE solace` window for Tanis, Caramon, and Tasslehoff,
    based on a summary-level read ("the survivors talk about going back
    to Solace"). Re-reading the actual scene (`.research/dosd_full.txt`
    lines ~15165-15290, the ridge above the ruined Temple right after
    the climax) found that doesn't hold up: only Caramon (with Tika,
    untracked) is actually headed to Solace, to help rebuild; Tanis is
    confirmed returning to Kalaman instead, in the same scene as the
    real ring-exchange/reconciliation beat with the elfwoman -- unnamed
    at this milestone, named as of Milestone 49 (Laurana); Tasslehoff is
    peeling off to his own never-named kender homeland, an off-page
    location this game doesn't model. None of it
    happens anywhere but the Neraka ridge itself -- all still spoken
    future intent the same night the Temple explodes -- so no new
    `PRESENCE` window or `LOCATION` was added. Instead, three new
    `TOPIC` entries went into the existing `PRESENCE neraka 105 107`
    window: Tanis's "A Ring of Gold and Steel", Caramon's "Going Home",
    and Tasslehoff's "A Hero's Welcome" (which folds in a retrospective
    of his whole tracked arc -- the Ice Wall dragon orb, the Tower
    siege, the Neraka rescue -- via the elfwoman's own unnamed dialogue
    at the time; Laurana's own account of the same window followed in
    Milestone 49). Tanis's private knowledge that the "Dragon Highlord" the
    kenders killed was actually the cowardly Fewmaster Toede was left
    out deliberately -- it's his own aside about a scene he isn't part
    of, and doesn't cleanly attach to any one character's own `TOPIC`.
    Pure data content, no `.cpp`/`.h` changes: verified via a clean
    rebuild (zero new warnings) and the piped smoke test only, no
    throwaway self-test needed since no new grammar, `PRESENCE`
    boundary, or zone/location was introduced. See
    `docs/TIMELINE_NOTES.md`'s "The ridge farewell" section.
48. Fizban -- the first canon character added beyond the 8 Heroes of the
    Lance, reversing this project's long-standing "keep major recurring
    canon characters unnamed" precedent at the user's explicit request. A
    research pass across all three Chronicles novels first scoped Fizban,
    Laurana, and the Dragon Highlords together (see
    `docs/TIMELINE_NOTES.md`'s "Fizban" section for the full findings);
    Fizban was picked to build first since his arc is small and clean --
    real, talkable, on-page presence at two locations this game already
    has scheduled windows for (`qualinesti 7 9`, the Speaker's feast where
    he's entrusted with the Disks; `pax_tharkas 10 12`, where he's the
    central actor of the fortress's chain-room climax) -- plus naming him
    at the two places he was already present but unnamed
    (`godshome 103 103`, `neraka 105 107`). New `CHARACTER fizban` block,
    pure data, no new `LOCATION`. One existing-content edit: Tasslehoff's
    `godshome` `SAY` line named him ("a strange old man" -> "Fizban"),
    since he'd have known Fizban by name since day 7. His apparent death
    at Pax Tharkas (a botched featherfall spell) is real on-page content
    but deliberately *not* the established "no SAY = permanently gone"
    device -- unlike Sturm/Raistlin/Flint, his story genuinely continues
    past that window. His true identity stays exactly as unnamed/
    ambiguous as every prior milestone left it -- this names the
    character "Fizban," not the "Draco Paladin" reveal the source text
    itself makes explicit at Neraka. Verified via clean rebuild (zero new
    warnings) and the piped smoke test; no throwaway self-test needed
    (pure data, no new grammar). See `docs/TIMELINE_NOTES.md`.
49. Laurana -- the largest single character addition this project has
    made, at the user's explicit request: nine `PRESENCE` windows (
    `qualinesti 7 9`, `pax_tharkas 10 12`, `tarsis 20 22`,
    `ice_wall 38 42`, `high_clerist_tower 76 80` and `81 81`,
    `palanthas 83 89`, `kalaman 90 92`, `neraka 105 107`), all at
    locations/day-ranges already modeled -- pure data, no new `LOCATION`.
    Also confirms Milestone 48's reversal wasn't a one-off: the "keep
    major recurring canon characters unnamed" precedent is now retired
    as a blanket rule (see `docs/TIMELINE_NOTES.md`'s "Named vs.
    off-stage canon characters"). Re-verified the Ice Wall roster
    directly against a fresh `Dragons_of_Winter_Night` extraction before
    touching it: "The Song of the Ice Reaver" names her among the party
    outright and gives her the window's central beat (frozen wolf-magic
    holds every swordsman still except her; she takes up the Ice Reaver
    and kills Feal-thas herself) -- Milestone 36's existing roster was
    correct, just incomplete, nothing needed correcting. Her schedule
    follows the Ice Wall/Tower sub-group (Sturm/Flint/Tasslehoff), not
    the Silvanesti sub-group, confirmed by the same song. Her
    `high_clerist_tower 81 81` window gets a real `SAY`, not the
    established "no SAY" device -- she survives the day Sturm dies and
    carries the rest of the war, unlike the three characters that device
    is reserved for. One already-shipped scene (the Neraka ring-exchange,
    Milestone 47) gets a second, named account of the same window rather
    than a rewrite of the original; two anonymous speaker tags standing
    in for her ("a golden-haired elfwoman") were updated in place to read
    "Laurana," same light touch as Milestone 48's Tasslehoff edit --
    plain pronouns elsewhere were deliberately left alone. Her captivity
    at Flotsam/Dargaard Keep between `kalaman 90 92` and `neraka 105 107`
    has no existing `LOCATION` and stays unmodeled, same "don't invent to
    fill a gap" restraint as ever. Verified via a clean rebuild (zero new
    warnings) and the piped smoke test; no throwaway self-test needed
    (pure data, no new grammar). See `docs/TIMELINE_NOTES.md`'s "Laurana"
    section.
50. Kitiara -- resolves the Dragon Highlords backlog item. A design
    question the prior research pass had left open (adversarial character
    on the same friendly talk/topic picker the 8 Heroes use, or
    retrospective dialogue inside an existing Hero's own `TOPIC`) was
    raised via `AskUserQuestion` and answered: retrospective dialogue.
    No new `CHARACTER kitiara` block, no new `PRESENCE`, no new
    `LOCATION` -- she never becomes directly talkable. Four content
    changes, each re-sourced directly from a fresh `pdftotext -layout`
    extraction: Tanis's new `TOPIC "The Crown of Power"` at
    `neraka 105 107` (the Crown-of-Power confrontation, where he crowns
    himself instead of her and tests her word); Laurana's `TOPIC "What
    the Dragon Highlord Said"` at `high_clerist_tower 81 81` gets a
    light-touch name edit (same technique as the Fizban/Laurana
    anonymous-tag fixes) plus a new second `TOPIC "The Dragonlance
    Returned"`; Caramon's new `TOPIC "His Sister"` at `neraka 105 107`
    (a Blood Sea storm scene where he and Raistlin both recognize their
    half-sister, unstaged since no `LOCATION` models a sea voyage, same
    retrospective-material technique as Tanis's own unstaged Flotsam
    content). Kitiara is named throughout, since the blanket
    "keep major recurring canon characters unnamed" precedent was already
    retired at Milestone 49 and every sourced scene has a tracked Hero
    saying or hearing her name directly in the source text. Tanis's
    existing `TOPIC "A Debt He Won't Name"` at `kalaman 100 100` was
    deliberately left untouched -- an in-character choice not to name
    her, not a narrator placeholder. Ariakas and Lord Soth remain
    off-stage; no tracked Hero witnesses their scenes with Kitiara.
    Verified via the piped smoke test; no throwaway self-test needed
    (pure data, no new grammar). See `docs/TIMELINE_NOTES.md`'s "Kitiara"
    section.
51. Quest system (engine) -- the world had places, people, and monsters
    but nothing to actually do; this milestone answers that with glue over
    what already existed rather than a new subsystem. New `quest/` module
    (`Quest`/`QuestCatalog`/`QuestLoader`, zero in-project dependencies,
    same shape as `timeline::Timeline`) parses a new `data/quests.txt`.
    `VISIT`/`TALK` objectives are pure queries over `visitedLocations`/
    `metCharacters` (tracked since Milestones 3/18, no new state needed);
    `SLAY` needed one new field, `GameState::monsterKills`, a lifetime
    tally `runCombat` increments on every kill. Zone files gained a
    `QUEST <char> <quest-id>` POI keyword, modelled on `PORTAL` (an id
    payload needing cross-file validation) with `BOAT`'s "must already
    have a TALK line" prerequisite; the quest id itself is cross-checked
    against the loaded catalog in `main.cpp`. Turn-in hooks into
    `GameLoop::talkTo` in the same slot the `grantsBoat` precedent
    (Milestone 36) established. New `g` key (`j`/`q`/`l` were all already
    taken) opens a one-keypress quest journal. `conditionMatches` gained a
    `knight` condition so a future quest can gate on Knights of Solamnia
    without a grammar change. One proof-of-concept quest ships,
    `road_wolves` (kill 3 timber wolves, 40 steel + 90 XP), offered by
    Solace's Notice Board -- its "armies on the move in the east" flavor
    text, originally authored as a nod to a future rumor/quest engine,
    turned out to be exactly that hook. Decided with the user before
    building: quest givers are ordinary NPCs, the Notice Board, and Knight
    of the Sword advancement (never the canon Heroes -- they're weather in
    this project's pitch, not employers); `DELIVER`/item objectives and
    item rewards were explicitly deferred (a real quest-item subsystem is
    the riskiest thing to build near the user's real save). Verified via a
    throwaway self-test (`QuestLoader` against the real data file plus
    malformed-input cases; `SaveGame` `QUEST`/`KILL` round-trip and
    backward compatibility) and the piped smoke test; a second throwaway
    check confirmed the user's actual `save.txt` still loads clean with
    empty quest/kill maps. **Interactive UI (dialogue boxes, the
    Accept/Decline picker, the journal screen) was not verified via real
    keypresses** -- no piping or tmux/PTY driver works for this project's
    `_getch()`-based input on this box -- so that path still needs a human
    playtest. See `docs/QUEST_NOTES.md`.
52. Quest system (real content) -- the follow-up content pass NEXT UP
    named after Milestone 51: four more quests from ordinary NPCs, picked
    to prove the objective kinds and `REQUIRE` vocabulary `road_wolves`
    hadn't exercised yet rather than to pad out the quest count.
    `inn_supply_run` (Otik, `solace_inn:O`, SLAY 3 goblins),
    `word_for_the_tower` (the Garrison Knight, `high_clerist_tower:K`,
    this project's first VISIT-only quest -- carry word to `palanthas`),
    `bazaar_road_raiders` (the City Watchman, `kalaman:G`, SLAY 2
    hobgoblins), and `kin_beyond_the_border` (the Silvanesti Warder,
    `silvanesti:W`, this project's first TALK-only quest and first
    `REQUIRE` other than `knight` -- `REQUIRE elf`, TALK `qualinesti:E`).
    Every quest reframes a hook already present in that POI's own `TALK`/
    `TOPIC` flavor text (stalled Vingaard supplies, a watchman worried
    about more than pickpockets, Silvanesti's own "we shut the gate even
    to our Qualinesti kin" line) rather than inventing new lore. Pure data
    content -- zero `.cpp`/`.h` changes, since every objective kind and
    `REQUIRE elf` already existed in the engine. Two backlog items were
    deliberately left for a future milestone rather than bundled in: Knight
    of the Sword advancement (real DL Adventures pp.18-19 research done
    during planning -- needs a new `KnightOrder::Sword` value and a new
    level-gated condition, bigger than a content-only pass) and `DELIVER`/
    item objectives (none of these four quests needed one). Verified via a
    throwaway `QuestLoader` self-test against the real, five-quest
    `data/quests.txt`, a clean `/W4` rebuild, and the piped smoke test
    (proves `main.cpp`'s cross-validation accepts all four new `QUEST
    <char> <quest-id>` zone bindings). Interactive verification (accepting
    each quest, confirming `REQUIRE elf` gates `kin_beyond_the_border`, the
    "ready to turn in" notification firing for a VISIT/TALK quest for the
    first time) still needs the user's own keyboard, same limitation
    Milestone 51 flagged. See `docs/QUEST_NOTES.md`'s "Shipped quests".
53. Knight of the Sword advancement -- the NEXT UP item Milestone 52
    deliberately deferred. Real DL Adventures pp.17-19 requirements,
    reconfirmed via rendered page images rather than trusting `pdftotext`
    OCR alone: this session's re-render turned up a genuine 1987 book
    erratum (p.18's Sword minimums box is printed with a "Rose Knight
    Minimum Scores" header, contradicted by the correctly-labeled Rose box
    on p.19 with different values). A new `character::KnightOrder::Sword`
    value (append-only-safe), `character::meetsKnightOfSwordRequirements`
    (Str12/Int9/Wis13/Dex9/Con10), and a new `game::conditionMatches`
    token, `sword_eligible` (Crown + level>=3 + those minimums) -- this
    project's first compound, non-single-word `REQUIRE` condition. The
    real quest, `named_in_fact`, is given by a new POI (`S`, "A Sword
    Knight") at High Clerist's Tower's Muster Yard -- reframing that POI's
    own pre-existing "names a new Knight in fact as well as blood" flavor
    line rather than inventing a new hook, since the existing Garrison
    Knight POI already carries `word_for_the_tower` and v1 allows only one
    quest per POI. This project's first quest to mix two objective kinds
    (`VISIT plains_of_dust` for the book's 500-mile/30-day journey,
    `SLAY baaz 1` for its single combat against an evil opponent -- Baaz
    Draconians are already flavored as a blade-drawn duelist, and this
    project's combat is already "knocked out, not killed", which happens
    to satisfy the book's "victorious ... without necessarily killing"
    clause for free); the book's other four required elements (three tests
    of wisdom, one of generosity, one of compassion, restoring something
    lost) have no corresponding trackable state, so they're narrated in the
    `COMPLETE` text instead, the same "narrated, not tracked" treatment
    Milestone 47's ridge farewell gave untracked lore. Turning the quest in
    sets a new bare `REWARD_KNIGHT_SWORD` quest-file flag, promoting
    `knightOrder` to Sword; `SaveGame.cpp`'s `KNIGHTORDER` bound moved from
    2 to 3 values (append-only-safe). Verified via a throwaway self-test
    (ability-score boundaries, `QuestLoader` parsing the new reward flag
    and its fail-fast trailing-argument case), a clean `/W4` rebuild, a
    direct check that the user's real save still loads under the new
    `KNIGHTORDER` bound, and the standard piped smoke test. Interactive
    verification (reaching level 3 as a Crown Knight, confirming the quest
    gate, completing the VISIT+SLAY mix) still needs the user's own
    keyboard. See `docs/CHARACTER_NOTES.md`'s "Knights of Solamnia" and
    `docs/QUEST_NOTES.md`'s "Shipped quests".

54. Dragonlance magical items -- introduces real magic items to the game
    world, sourced from *Dragonlance Adventures* (TSR 2021)'s own
    "Magical Items of Krynn" chapter (pp.91-99, visually confirmed via
    rendered page images, `pdftoppm`) and the 2nd ed. DMG's "Magical Item
    Tables." Asked the user how items should be obtained before building
    anything; the answer was both a shop item and a quest reward, so this
    ships exactly one of each rather than a generic item subsystem. A
    "+1" enchanted weapon per class (`character::magicWeaponFor`, DMG
    Table 109 pricing: 400stl for a sword, 500 for other weapon types),
    sold at every shop alongside the existing mundane upgrade -- the
    first thing in this project to add a to-hit bonus distinct from
    Strength, needing a new `Character::weaponMagicBonus` field threaded
    through `combat::resolvePlayerAttack`. Mage and Tinker, who have no
    mundane weapon upgrade at all, get their first-ever upgrade this way,
    via magic rather than smithing. Solamnic Armor (`ArmorId::
    SolamnicArmor`, AC 0, sourced directly from DLA p.93-94: "equal to AC
    0 (plate +1 and shield +1)... only granted to those Knights who have
    demonstrated the finest qualities of Knighthood") is a new quest
    reward, `data/quests.txt`'s `solamnic_armor` -- this project's first
    item-granting quest reward, via a new `REWARD_SOLAMNIC_ARMOR`
    compile-time flag mirroring `REWARD_KNIGHT_SWORD`'s exact shape
    rather than reopening the generic item-reward mapping
    `docs/QUEST_NOTES.md` had already deliberately rejected. Gated by a
    new `sword_knight` condition (currently *is* a Sword Knight, distinct
    from `sword_eligible`'s advancement check) since the book ties this to
    the title "Lord," a rank this project doesn't model -- scoped instead
    to the already-shipped Sword rank. Offered by a new POI, `data/zones/
    high_clerist_tower.txt`'s `L` ("A Knight of the Circle"), since `K`
    and `S` already carry a quest each. The chapter's unique, canon-owned
    artifacts (Wyrmslayer, Staff of Magius, the Hammer of Kharas, the
    Dragonlances themselves) are deliberately *not* made player-obtainable
    -- same restraint that's kept Alhana/Derek/Gunthar off-stage and
    Sturm's/Raistlin's/Flint's own story beats unreplayable by the player
    character; Solamnic Armor was chosen specifically because the book
    frames it as a replicable rank grant, not a one-of-a-kind relic.
    `SaveGame.cpp` touches: `ArmorId`'s bound widened 4->5 (append-only-
    safe, same precedent as `KnightOrder` at Milestone 53), and the
    equipped/inventory weapon line migrated from `WEAPON sides bonus
    name` to `MAGICWEAPON sides bonus magicBonus name` (a new field
    couldn't be inserted into the old greedy-to-end-of-line format
    without corrupting it) -- `save()` now writes `MAGICWEAPON`
    unconditionally, `load()` accepts both it and the legacy `WEAPON`
    keyword, the same migration shape as `GOLD`->`STEEL`. Verified via a
    throwaway self-test (shop-catalog shape/index arithmetic, purchase/
    equip/sell round-trip including newly-unsellable `SolamnicArmor`,
    `resolvePlayerAttack`'s magic-bonus wiring, `QuestLoader` against the
    real seven-quest `data/quests.txt`, and a save round-trip covering
    `MAGICWEAPON`/legacy `WEAPON`/the widened `ARMOR` bound), a clean
    `/W4` rebuild, a direct check that the user's real `save.txt` still
    loads cleanly under the new save format, and the standard piped smoke
    test. Interactive verification (buying/equipping the magic weapon,
    reaching Sword and completing `solamnic_armor`) still needs the
    user's own keyboard, the same `_getch()` limitation flagged for every
    quest milestone so far. See `docs/CHARACTER_NOTES.md`'s "Magic
    items", `docs/QUEST_NOTES.md`'s "Shipped quests", `docs/ZONE_NOTES.md`.

55. Order of the Rose advancement -- the capstone of the Knights of
    Solamnia chain, completing Crown (character creation) -> Sword
    (Milestone 53) -> Rose. Re-confirmed the sourcing directly against
    rendered page images of DL Adventures pp.18-19 rather than trusting
    Milestone 53's earlier partial pass from memory, per this project's
    accuracy discipline -- and turned up a second real book inconsistency
    alongside the p.18/p.19 Sword erratum Milestone 53 already found: the
    Rose "Minimum Requirements" prose says a candidate needs "two levels as
    Crown, then Sword and two additional levels" (arithmetically level 5)
    but the very next sentence says "sufficient hit points to become 4th
    level" -- resolved via the **Rose Knight Advancement Table** itself,
    which starts at level 4 ("Novice of Roses"), the same hard-table-over-
    loose-prose tie-break Milestone 53 used for its own erratum. A new
    `character::KnightOrder::Rose` value (append-only-safe),
    `character::meetsKnightOfRoseRequirements` (Str15/Int10/Wis13/Dex12/
    Con15, correctly-labeled on p.19, distinct from the p.18 box that's
    actually mislabeled Sword data), and `game::conditionMatches`'s
    `rose_eligible` token (`knightOrder==Sword && level>=4 &&
    meetsKnightOfRoseRequirements`), the same compound-condition shape as
    `sword_eligible`. The real quest, `measure_of_roses`, is given by a new
    POI (`R`, "A Rose Knight") at High Clerist's Tower's Muster Yard --
    `K`/`S`/`L` there already carry a quest each, so this needed its own
    POI, same reasoning Milestone 54 used for `L`. Objectives mix `VISIT
    plains_of_dust` (reusing `named_in_fact`'s "farthest mapped location
    from the Tower" target, since the book's 500-mile/30-day journey
    requirement is *identical* text between Sword and Rose) and `SLAY ogre
    1` (the book's "evil opponent of equal or higher level... without
    killing the foes" -- Ogre is the highest-XP, clearly-evil single
    monster in the roster, deliberately distinct from Sword's Baaz duel so
    the two quests don't feel identical); the book's other four elements
    (one test of wisdom, three of generosity, three of compassion,
    restoring something lost) have no corresponding trackable state, same
    as Sword's four, and are narrated in `COMPLETE` text only. Turning it
    in sets a new bare `REWARD_KNIGHT_ROSE` flag, promoting `knightOrder` to
    Rose; reward is 100 steel/250 XP, above Sword's 60/150 since Rose is the
    capstone rank. `SaveGame.cpp`'s `KNIGHTORDER` bound moved from 3 to 4
    (append-only-safe, same precedent as Milestone 53's 2->3 move); a new
    `Leveling.cpp` flavor line foreshadows Rose eligibility at level 4,
    mirroring the existing level-3 Crown->Sword line. Verified via a
    throwaway self-test (ability-score boundary cases including confirming
    Sword's own minimums don't accidentally satisfy Rose's higher bar,
    `QuestLoader` against the real eight-quest `data/quests.txt` including
    `REWARD_KNIGHT_ROSE`'s fail-fast case, and a save round-trip covering
    both the widened `KNIGHTORDER` bound and its new exclusion boundary), a
    clean `/W4` rebuild, a direct check that the user's real save (the
    executable-relative `build\Debug\save.txt`, not the stale repo-root
    copy -- see `docs/GOTCHAS.md`) still loads cleanly under the new bound,
    and the standard piped smoke test. Interactive verification (reaching
    level 4 as a Sword Knight, confirming the Rose Knight only offers the
    quest once eligible, completing the VISIT+SLAY mix) still needs the
    user's own keyboard, the same `_getch()` limitation flagged for every
    quest milestone so far. See `docs/CHARACTER_NOTES.md`'s "Knights of
    Solamnia" and `docs/QUEST_NOTES.md`'s "Shipped quests".

56. Webnet and Brooch of Imog -- the NEXT UP item Milestone 55 pointed at
    (DLA's "Magical Items of Krynn" chapter), continuing Milestone 54's
    magic-item work. Re-reading the actual chapter (rendered page images,
    book pp.91-94, a confirmed +1 PDF-page offset) found it thinner than
    expected: almost every entry needs a subsystem this engine doesn't
    have yet -- charges, creature command/charm, translation flags, a
    plot-key/door mechanic -- and inventing one just to place a single
    item would be the "premature abstraction" CLAUDE.md warns against.
    Asked the user how to scope it; they chose to build one small,
    genuinely reusable subsystem rather than drop the milestone or
    cherry-pick a single item. Two items share the same shape and both
    solve it honestly: **Webnet** (p.93, Miscellaneous Magic, Mage-only
    per its own text) is consumed on use and negates the monster's next
    attack; **Brooch of Imog** (p.92, Crystals and Gems, also Mage-only)
    is not consumed, gated to once per real in-game day exactly like
    `Character::lastRestDay` already gates Rest (a new
    `Character::lastBroochUseDay`), and negates *all* the monster's
    remaining attacks for the rest of the current fight -- a deliberate,
    flagged simplification of the book's "10 rounds" (this engine has no
    round-duration tracker outside a single `runCombat` call, and fights
    are short enough that "this fight" and "10 rounds" are functionally
    the same thing). Both "does the monster's next attack land" effects
    are resolved as plain local variables inside `GameLoop::runCombat`,
    exactly like `monsterHp` and the combat log already are -- only the
    Brooch's daily charge needs to survive to the save file. Sold at every
    shop, Mage-only (`character::availableShopItems`), at two invented,
    explicitly flagged prices (neither item has a book-printed Steel Piece
    value, unlike the Potion/magic weapons' DMG-table reuse): 150stl and
    500stl. Used via the same `'i'`-in-combat local-key-reinterpretation
    `GameLoop::runCombat` already uses for drinking a potion, now a
    priority chain (potion, then Webnet, then Brooch). `SaveGame.cpp`
    touches: a new `BROOCHDAY` line (optional on load, defaulting to -1,
    same backward-compatibility shape `RESTDAY` already has) and two new
    bare-keyword inventory lines, `WEBNET`/`BROOCH`. The rest of the DLA
    chapter (Rods/Staves/Wands' Staff of Striking/Curing and Diviner of
    Life, Crystals and Gems' remaining three entries, Miscellaneous
    Magic's remaining three, and all of Armor and Shields/Weapons beyond
    the already-shipped Solamnic Armor) was deliberately left out --
    either needs an unbuilt subsystem, is antagonist-only, is a whole
    quest's own goal (Plate of Solamnus), or is a unique named artifact
    (Dragonlance, Mantooth, Nightbringer, Wyrmsbane, Wyrmslayer, Shield of
    Huma), same restraint as every other named-artifact exclusion since
    Milestone 54. Verified via a throwaway self-test (shop
    eligibility per class, purchase/sell round-trip including the
    stackable-Webnet-vs-single-Brooch distinction, `useWebnet`'s index
    validation, `activateBrooch`'s day-gate across two different days, and
    a `SaveGame` round-trip covering `BROOCHDAY`/`WEBNET`/`BROOCH` plus
    loading an old save with the `BROOCHDAY` line stripped out), a clean
    `/W4` rebuild, a direct check that the user's real `save.txt` (the
    executable-relative `build\Debug\save.txt`) still loads cleanly under
    the new format, and the standard piped smoke test. **Interactively
    verified** (2026-08-20) by the user: bought and used a Webnet mid-
    fight and confirmed the monster's attack was really skipped; used the
    Brooch and confirmed its once-per-day gate blocked a second use the
    same day. See `docs/CHARACTER_NOTES.md`'s "Magic items".

57. Terrain-specific monster pools -- the NEXT UP item the user picked to
    build next. `combat::MonsterCatalog::randomMonster` had been uniform-
    random regardless of terrain since encounters existed, even though
    `encounterChancePercent` (Milestone 27) already varied *whether* an
    encounter happens by terrain. Before writing code, re-sourced the real
    2nd-edition Monstrous Manual "Climate/Terrain" field for all 11 roster
    monsters via rendered page images (OCR text proved unreliable for this
    book's multi-column shared stat tables) -- this overturned the premise
    of the backlog note itself: almost every monster's real Climate/Terrain
    is "Any land" or "Any non-arctic land," and **Giant Spider is explicitly
    not forest-locked** in the book, contrary to the "spiders in forest"
    example the note had speculated. Real hard differentiation turned out to
    be thin: nothing in the roster is Arctic-flavored, Gnoll excludes desert
    ("non-desert"), Bugbear leans subterranean, Timber Wolf is "non-
    tropical." Asked the user how to proceed given how little the source
    material actually supported the original pitch (`AskUserQuestion`); they
    chose a hybrid: real Climate/Terrain as a hard exclusion only where the
    book supports one, plus clearly-flagged *invented* flavor weighting on
    top, informed by (not transcribed from) each monster's Habitat/Society
    prose -- the same "tuned, not sourced" honesty `encounterChancePercent`
    already gets. Two new optional `data/monsters.txt` grammar lines:
    `EXCLUDE_TERRAIN <codes>` (only Gnoll uses it, excluding salt flat, this
    project's closest terrain analog to desert) and `TERRAIN_BIAS <codes>`
    (Goblin/Kobold lean hills+forest, Timber Wolf leans forest+grassland,
    Giant Spider leans forest+bog, Bugbear leans hills+mountains, Gnoll leans
    forest+hills+bog; Hobgoblin/Ogre/Baaz/Kapak/Ghoul/Skeleton/Zombie stay
    deliberately uniform -- Ogre's own book text says "found anywhere," the
    undead have no ecological terrain link, and Baaz/Kapak's real
    differentiator is faction/location, which this terrain-code system can't
    represent honestly). `combat::Monster` gained
    `excludedTerrain`/`terrainBias` (`std::vector<char>`, both
    `world::TerrainInfo::code` values, no new vocabulary needed);
    `MonsterCatalog::randomMonster` changed signature to take the triggering
    `char terrainCode`, builds an eligible list (excluding hard exclusions,
    with a defensive uniform-roster fallback if that's ever empty -- can't
    happen with current data), then does a weighted pick (`kBiasWeight = 3`)
    using the same `character::roll` RNG primitive already used for dice
    elsewhere -- no new randomness machinery. `GameLoop::tryMoveOverworld`
    already had `terrain` in scope at the encounter-roll call site, so the
    change was a one-line call-site update. Glacier deliberately has no
    exclusions written for it even though nothing in the roster is
    Arctic-flavored -- rather than hand-excluding all 13 monsters, this is
    left as a documented, honest gap that falls back to the full uniform
    pool, matching the project's existing "not invented to fill a gap"
    restraint (e.g. Ocean's "no sea monsters yet" note in `Terrain.cpp`).
    While re-sourcing, also found and fixed a real citation bug unrelated to
    the terrain work: 9 of 11 monster page citations in `docs/COMBAT_NOTES.md`
    were off by exactly +3, citing the PDF's internal page count instead of
    the book's printed folio (visually confirmed against every page's actual
    footer number) -- Bugbear's and Ogre's were already correct. Verified via
    a throwaway self-test (`MonsterLoader` parses both new keywords from the
    real 13-monster `data/monsters.txt` and fails fast on a malformed token;
    3000 rolls confirm Gnoll never appears on excluded terrain; 12000 rolls
    on hills confirm a biased monster (Bugbear) is picked roughly 3x as often
    as an unbiased one (Hobgoblin), matching `kBiasWeight` almost exactly in
    practice), a clean `/W4` rebuild (zero new warnings), and the piped smoke
    test. **Interactively verified** (2026-08-20) by the user: terrain-
    appropriate monsters visibly turned up while walking different terrain
    types in a live playthrough. See
    `docs/COMBAT_NOTES.md`'s "Terrain-specific monster pools".

58. `DELIVER`/item objectives -- the NEXT UP item the user picked to build
    next, resolving the last deferred piece of the original quest-engine
    design (deferred since Milestone 51, flagged both times as "the
    riskiest thing to build near the user's real save" -- see
    `docs/QUEST_NOTES.md`'s former "Deliberately not in v1" entry). Asked
    the user how the item side should work before building anything
    (`AskUserQuestion`, same practice as Milestones 54/56): reuse an
    existing item kind (lower risk, weaker flavor) or build a real
    quest-item concept; they chose the richer path. A new
    `character::ItemKind::QuestItem` carries its own `questItemId`/
    `questItemName` directly (the same "id + free display text" shape
    `InventoryItem::weaponName` already has for a `Weapon`, not a fixed
    enum + lookup table) -- never equippable, never sellable, removed from
    inventory on turn-in. A new `quest::ObjectiveKind::Deliver` (grammar:
    `DELIVER <item-id> <count> <label>`, parsed exactly like `SLAY`) reads
    `character::Character::inventory` directly, the same "objective is a
    query over existing state, not a counter" shape as `VISIT`/`TALK`/
    `SLAY`. The item's origin is a new zone-file line, `GRANTS_ITEM <char>
    <item-id> <display-name...>`, a one-line mirror of the existing `BOAT`
    mechanism (Milestone 36): granted the first time that POI's `TALK`
    fires, same "must already have a TALK line" validation. Deliberately
    **not** a courier/two-location handoff mechanic -- turn-in still
    happens at a single POI (the giver), matching every quest shipped so
    far; `DELIVER` is a fetch objective ("possess it when you return"),
    and the grant/the quest that wants it are two independently-existing
    pieces of state, not a special-cased link between them. Ships one
    proof-of-concept quest, `ore_for_the_forge`: a new POI,
    `data/zones/pax_tharkas.txt`'s `O "An Ore Cart"` (grounded in the
    zone's existing "war ... over who controls what's dug from it"
    flavor, kept off the zone's `TIMELINE_ANCHOR` tile since no zone
    shipped so far combines an anchor with its own zone-native `TALK`
    line), grants `raw_tharkadan_ore`; `data/zones/solace.txt`'s `POI S`
    ("Flint's Smithy," pure scenery until now) gains a `TALK S` for an
    unnamed journeyman keeping the forge running -- written evergreen,
    deliberately never claiming to *be* Flint, whose own tracked schedule
    may have him elsewhere or already dead depending on the game day.
    `SaveGame.cpp` touches: a new, purely additive `QUESTITEM <item-id>
    <display-name...>` inventory-entry keyword -- not a widened enum
    bound, since `ItemKind` was never itself a raw serialized int.
    Verified via a throwaway self-test (`QuestLoader` parsing the real
    `DELIVER` line plus a malformed-line failure case; `ZoneLoader`
    parsing `GRANTS_ITEM` plus its "no TALK line" failure case; the real
    edited zone files loading clean; `findQuestItemIndex`/
    `inventoryItemLabel`/`sellableItems` on a constructed inventory; a
    `SaveGame` round-trip covering `QUESTITEM`), a clean `/W4` rebuild, a
    direct check that the user's real `save.txt` still loads cleanly
    under the new inventory format, and the standard piped smoke test.
    Interactive verification (walking to Pax Tharkas, picking up the ore,
    carrying it to Solace, confirming the journal and turn-in) still
    needs the user's own keyboard, the same `_getch()` limitation flagged
    for every quest milestone so far. See `docs/QUEST_NOTES.md`'s
    "DELIVER"/"Shipped: ore_for_the_forge" and `docs/ZONE_NOTES.md`'s
    "Quest items: POIs that grant a DELIVER object". **Interactively
    verified** (2026-08-20) by the user, including an out-of-order
    pickup (ore grabbed before the journeyman was ever talked to) that
    still turned in cleanly on first contact.

59. Alhana Starbreeze -- the first canon character named since Milestones
    48-49 (Fizban, Laurana), at the user's request for more canon NPCs.
    Of the still-off-stage roster, Alhana was the clean pick: sustained,
    talking, on-page presence throughout the already-modeled
    `silvanesti 25 30` window (she pilots the griffons in, leads the
    party to her father, is the emotional center of the Tower of the
    Stars climax) -- no new `LOCATION`, no new zone POI, no `.cpp`/`.h`
    changes, pure data appended to `data/timeline.txt`'s existing
    `CHARACTER` roster. Re-sourced directly from `.research/dwn_full.txt`:
    the griffon-flight exchange with Tanis (lines 3770-3990, including her
    guarded regret over the Starjewel given to Sturm at Tarsis, used only
    for her guardedness, not an explanation of the jewel itself, since the
    source never explains it at this point either) and the Tower of the
    Stars climax (lines 4847-4995, including her address for Tanis,
    "Half-Elven!", grounding a `SAY_IF halfelf`). Derek Crownguard, Lord
    Gunthar, Ariakas, and Lord Soth stay out of scope for this milestone
    specifically -- not a new restraint call, the same one already on
    record: Derek/Gunthar's on-page scenes are at Sancrist Isle, not a
    modeled `LOCATION`, and Ariakas/Lord Soth are never witnessed directly
    by a tracked Hero, so either would need the Kitiara treatment
    (retrospective `TOPIC` in an existing Hero's dialogue, no `CHARACTER`
    block) rather than this one's. `data/zones/silvanesti.txt`'s generic
    "Silvanesti Warder" POI is untouched -- a separate, still-anonymous
    border sentinel, not Alhana. Verified via a clean rebuild (zero new
    warnings, no `.cpp`/`.h` changes) and the piped smoke test; no
    throwaway self-test needed (pure data, no new grammar, same call as
    Milestones 47-49). See `docs/TIMELINE_NOTES.md`'s "Alhana Starbreeze"
    section for the full sourcing and scope notes.

## NEXT UP

Not yet started — a short menu of well-grounded backlog candidates, not
a commitment. Pick one (or something else) before starting the next
session's work.

1. **More monsters** — Bozak/Sivak/Aurak Draconians, Thanoi (walrus-men,
   flavor-only at Ice Wall so far -- see Milestone 36), and other
   Monstrous Manual entries are still untouched; the higher-tier
   draconians are spellcasters/shapeshifters, real mechanics this project
   doesn't model yet. See `docs/COMBAT_NOTES.md`'s "Extending this later."
2. **More of DLA's "Magical Items of Krynn" chapter** — Rods/Staves/Wands,
   Crystals and Gems, and Miscellaneous Magic entries beyond the Webnet/
   Brooch of Imog are real, sourced, and unused. See
   `docs/CHARACTER_NOTES.md`'s "Extending this later."
3. **More off-stage canon characters** — Derek Crownguard and Lord
   Gunthar (retrospective dialogue folded into an existing Hero's own
   `TOPIC` at the High Clerist's Tower, same treatment Kitiara got at
   Milestone 50, since their on-page scenes are at the unmodeled Sancrist
   Isle) and Ariakas/Lord Soth (same retrospective treatment -- no
   tracked Hero witnesses their scenes directly). See
   `docs/TIMELINE_NOTES.md`'s "Named vs. off-stage canon characters" and
   "Alhana Starbreeze" sections.
