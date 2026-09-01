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

60. Derek Crownguard and Lord Gunthar -- the Kitiara treatment (retrospective
    `TOPIC` dialogue folded into existing Heroes' schedules, never a
    talkable `CHARACTER` of their own) applied to the next pair on the
    off-stage roster, at the user's explicit pick over Ariakas/Lord Soth.
    Both names were already present in the game since Milestone 35 (Sturm's
    `ice_wall`/`high_clerist_tower` dialogue, the Tower's Garrison Knight,
    the Tarsis Runner), so this closed two real, sourced gaps the existing
    content left untold rather than introducing the names from scratch --
    found by re-reading `.research/dwn_full.txt` directly rather than
    trusting the existing docs summary. Laurana's new `TOPIC "The Writ of
    Vindication"` (`high_clerist_tower 76 80`) explains how Sturm's
    provisional Sancrist pledge became real: Gunthar, mid political war
    with Derek's faction, sent her ahead with the dragonlances and a formal
    Writ of Vindication after she testified to the Knights' Council herself
    -- the missing explanation for a `PRESENCE` line ("freshly vigiled and
    formally sworn") that's been in the game since Milestone 35 with no
    account of how it happened. Sturm's new `TOPIC "Derek's Last Charge"`
    (same window) tells Derek's end: driven half-mad by Sturm's
    vindication, he leads an unauthorized dawn sortie against the besieging
    dragonarmy the night of Sturm's own Knighting and dies raving that he'd
    won and would be Grand Master -- Sturm's own verdict grim and
    complicated ("He's dying -- bravely -- like a true knight") rather than
    a simple villain's end, consistent with the source's own funeral scene
    where Laurana's eulogy blames the whole divided Order, not Derek alone.
    Pure data -- two `TOPIC` lines added to two already-existing `CHARACTER`
    blocks in `data/timeline.txt`, no new `CHARACTER`/`LOCATION`/grammar,
    no `.cpp`/`.h` changes. Sancrist Isle itself stays unmodeled, same
    restraint as Milestone 36 (ocean-locked, no on-page reason to build it
    as a walkable stop). Ariakas and Lord Soth remain the one still fully
    untouched pair on the roster. Verified via a clean rebuild (zero new
    warnings) and the piped smoke test; no throwaway self-test needed (pure
    data, no new grammar). Not yet interactively verified in a real
    playthrough (day 76-80 at the Tower). See `docs/TIMELINE_NOTES.md`'s
    "Derek Crownguard and Lord Gunthar" section.
61. Ariakas and Lord Soth -- the sourcing pass Milestone 60 deferred, at the
    user's request, which overturned its own premise: a direct re-check of
    `.research/dosd_full.txt`'s Crown of Power sequence (the already-existing
    `neraka 105 107` window) found they aren't off-stage at all. Tanis
    personally kills Ariakas on-page, guided by an unidentified whispering
    voice, and Lord Soth appears in person in the same scene -- reaching for
    the fallen Crown, then ordered by Kitiara to personally escort Tanis
    through a hostile crowd at swordpoint. The existing `TOPIC "The Crown of
    Power"` (Tanis, Milestone 50) already dramatized part of this exact
    scene but omitted the kill and Soth's appearance entirely, and its
    ending overstated the resolution as a clean joint walk-out; what
    actually breaks the deal apart is Laurana herself, catatonic with shock
    until she suddenly seizes Kitiara's own sword, holds Tanis at
    swordpoint, declares her own independence, and throws them both off the
    platform into the ensuing succession free-for-all. This milestone
    corrects that `TOPIC` in place (the kill, Soth's escort, and the real
    ending) and adds one new `TOPIC` each for Tanis (`"What Kitiara Asked
    For"`, Kitiara's undisclosed earlier betrayal -- presenting Laurana to
    Takhisis as a war-trophy and offering her soul to Lord Soth) and Laurana
    (`"Free By My Own Hand"`, her own breakout moment, plus a matching
    in-place edit to her existing `TOPIC "Before the Dark Queen"` naming the
    soul-threat from her own point of view). Both names appear directly
    throughout -- the blanket unnamed-canon-character precedent was already
    retired at Milestone 49, and this is a stronger case for naming than
    Kitiara's own introduction was. No new `CHARACTER`/`PRESENCE`/`LOCATION`
    -- pure `TOPIC` edits/additions to two already-existing `CHARACTER`
    blocks, no `.cpp`/`.h` changes; confirmed safe against the user's save
    first (`TOPIC` text isn't persisted in `SaveGame.cpp`). Verified via a
    clean rebuild (zero new warnings) and the piped smoke test; no
    throwaway self-test needed (pure data, no new grammar). Not yet
    interactively verified in a real playthrough (day 105-107 at Neraka).
    See `docs/TIMELINE_NOTES.md`'s "Ariakas and Lord Soth" section.

The off-stage canon-character roster is now fully closed out -- Kitiara,
Fizban, Laurana, Derek Crownguard, Lord Gunthar, Ariakas, and Lord Soth
have all had their sourcing passes completed (see
`docs/TIMELINE_NOTES.md`'s "Named vs. off-stage canon characters"
section).

62. Cleric and Mage spells beyond 1st level -- at the user's explicit
    request, the biggest single engine milestone to date, growing
    `character::Spellcasting` from Milestone 40's one-known-spell-per-
    caster into a real multi-level spellbook. Sourced from
    `References/DQoK.pdf` (the manual for *Dark Queen of Krynn*, an
    official TSR/SSI Dragonlance computer game, not a rulebook -- read via
    rendered page images, `pdftoppm`, since its 4-column layout badly
    scrambles the OCR text layer) and cross-referenced against the actual
    PHB's own alphabetical spell index and Tables 21/24 (also re-rendered
    in full this pass, not just the 1st-level column Milestone 40
    transcribed): 88 real spells found (29 Cleric across 7 levels, 59
    Magic-User across 9), 85 confirmed exactly matching the PHB, three
    corrected (Cleric's "Resist Cold" folded into the real single 2nd-level
    "Resist Fire/Resist Cold"; Magic-User's "Iron Skin" and "Fire Touch"
    dropped as DQoK-original inventions with no real PHB spell behind
    them -- see `docs/CHARACTER_NOTES.md`'s "Spellcasting" section for the
    full census and citations). Of those 88, 49 are actually castable (14
    Cleric, 35 Wizard) -- only a spell whose PHB effect maps onto state
    this engine already tracks was implemented, reusing four existing
    patterns (Magic Missile's damage branch, Cure Light Wounds's heal
    branch, the Webnet/Brooch of Imog "block the monster's attacks"
    mechanism generalized to a count, and new this-fight-only THAC0/AC/
    damage buff-debuff parameters threaded through
    `combat::resolvePlayerAttack`/`resolveMonsterAttack`) rather than a
    bespoke mechanic per spell; the other 39 are sourced and documented but
    not selectable, each needing a subsystem this project doesn't have
    (poison/disease/blindness/curse status, monster saving throws --
    still none exist -- damage-type resistance, stealth, multi-attack
    rounds, ally summoning). Both classes now automatically "know" every
    implemented spell their level unlocks (real 2e Cleric behavior,
    extended to Mage as a documented simplification of the Wizard's
    spellbook/spell-research rules this project doesn't model). `Character`
    gained `memorizedSpellIds` (today's remaining prepared slots) and
    `preferredSpellIds` (the standing loadout); Rest/Bed Rest now default to
    re-memorizing the same loadout, only re-prompting a `drawPickerFrame`
    picker when the player asks to change it -- a real, sourced UX
    precedent, not invented (DQoK's own manual: "Selecting REST without
    choosing new spells has the spellcasters rememorize the spells they
    have cast since last resting"). In combat, `m`/`M` casts directly with
    one spell left, or opens a picker with more than one. `SaveGame.cpp`'s
    `SPELLSTODAY` line is replaced by `SPELLDAY`/`PREFERRED`/`MEMORIZED`,
    with the old keyword still accepted read-only on load (verified
    directly against the user's real save). DQoK's own per-spell Red/White
    Robe restriction was deliberately not modeled -- not sourced from the
    PHB or Dragonlance Adventures, and this project's Robe assignment is
    still flavor-only with nothing to attach a restriction to; flagged as a
    real Dragonlance-flavor design call, not an oversight. Verified via a
    throwaway self-test (spell-slot tables against both fully-transcribed
    PHB tables including the Wisdom-gated 6th/7th Cleric columns, the
    Kender/blocked-subrace zero-slots rule, `castSpell`'s damage/heal/
    block/instant-defeat/combined-buff-debuff shapes, and `SaveGame` round-
    tripping including legacy-`SPELLSTODAY` backward compatibility), a
    clean `/W4` rebuild (zero new warnings), the piped smoke test, and a
    direct load of the user's real save (a Fighter, so spellcasting itself
    is untouched by their character, but the save's legacy `SPELLSTODAY`
    line needed to keep loading regardless). **Interactive verification
    completed 2026-08-25** via a throwaway Cleric character run from an
    isolated copy of the exe (the user's real Fighter save was never
    touched): the first-rest spell-loadout picker offered a real choice
    among multiple spells (not just one), the in-combat `m`/`M` cast
    picker appeared and the user chose Bless (confirming the this-fight
    buff/debuff path, not just the damage/heal branches), and the second
    rest correctly asked "Keep the same spells memorized? (Y/n)" instead
    of re-running the picker. An instant-defeat spell (e.g. a Mage's
    Sleep) still hasn't been keyboard-verified -- the Cleric run had no
    occasion to cast one. See `docs/CHARACTER_NOTES.md`'s "Spellcasting"
    section for the full spell census and every citation.

    Two same-session follow-up refinements, both user-requested after the
    above shipped: the Rest flavor line is now class-specific ("You
    rememorize your prayers." for Cleric, "You memorize your incantations."
    for Mage, with the original combined line kept ready for a future
    dual/multi-class character -- `Character::charClass` is a single value
    today, so that branch can't actually trigger yet); and the character
    sheet ('c') gained an `s` option, only offered to a caster, opening a
    new `MapRenderer::drawSpellbookFrame` -- the full spell roster for the
    character's class, grouped by level with real per-day slot counts and
    each memorized-and-uncast spell marked, looping back to the sheet
    rather than dismissing straight to gameplay. Verified via a clean
    `/W4` rebuild (zero new warnings) and the piped smoke test after each.

63. Key-binding cleanup -- at the user's explicit request ("clean up the
    key bindings... player movement should just be WASD keys"). Movement
    had accumulated three overlapping schemes since Milestone 2/51: arrow
    keys, the vi `hjkl` cardinal convention, and `wasd`, plus `yubn`
    roguelike-convention diagonals. Asked the user (`AskUserQuestion`)
    whether to keep `yubn` diagonals, drop diagonals entirely, or replace
    them with a WASD-adjacent scheme (`qezc`); the user picked dropping
    diagonals entirely -- movement is now `wasd`-only, 4-directional, no
    diagonal movement at all. `render::Key` lost `NorthEast`/`NorthWest`/
    `SouthEast`/`SouthWest`; `Console::readKey` (both the Windows two-byte
    path and the non-Windows `getline` fallback) lost the arrow-key scan-
    code mapping, the `hjkl` aliases, and the `yubn` cases, leaving only
    `wasd` bound to the 4 cardinal directions (the Windows path still has
    to consume the second byte of any extended key it sees, or the classic
    `conio.h` off-by-one bug resurfaces -- see `docs/GOTCHAS.md`);
    `GameLoop::run`'s input switch lost its 4 diagonal-movement cases
    (`tryMoveOverworld`/`tryMoveZone` themselves are untouched -- they're
    generic `(dx, dy)` functions, just never called with a diagonal delta
    now). `MapRenderer`'s two live-frame control-line strings and the `?`
    help screen updated to describe `wasd` only. `North`/`South` remain
    bound to `w`/`s` and are unaffected in their second role as menu up/
    down (shops, inventory, the log pager, character-sheet lists, etc.) --
    only the overworld/zone movement switch changed. Verified via a clean
    `/W4` rebuild (zero new warnings, no self-test needed -- this is pure
    input-mapping deletion, nothing computational to assert against) and
    the piped character-creation smoke test. `README.md`'s Move bullet and
    `docs/GOTCHAS.md`'s key-binding notes updated to match.

    Same-session follow-up, also user-requested: Look moved from `;` to
    `l` (freed up by dropping the `hjkl` cardinal aliases above -- `l` was
    East under that scheme, now unbound). Updated in both of
    `Console::readKey`'s switches, `MapRenderer`'s two live control-line
    strings and the `?` help screen, `README.md`'s controls list, and the
    `Look`-related comments in `GameLoop.h`/`GameLoop.cpp`. Verified the
    same way: clean `/W4` rebuild, piped smoke test.

    A second same-session follow-up, also user-requested: arrow keys work
    again as a silent alias for `wasd`, but deliberately undocumented on
    screen -- the help screen, the two live status-line strings, and
    `README.md`'s controls list all still say `wasd` only. Windows-only
    (the non-Windows `getline` fallback has no way to receive an arrow key
    at all); `Console::readKey`'s extended-key branch maps scan codes 72/
    80/75/77 back to `North`/`South`/`West`/`East` instead of discarding
    them, every other extended key still falling through to `Unknown`.
    `docs/GOTCHAS.md`'s two key-binding notes updated to describe this as
    an intentional gap between the on-screen bindings and what
    `Console::readKey` actually accepts, not an oversight. Verified the
    same way: clean `/W4` rebuild, piped smoke test.

64. Four more monsters -- Bozak, Sivak, and Aurak Draconians, and Thanoi --
    the "More monsters" NEXT UP item the user picked to build next, closing
    it out entirely rather than cherry-picking. Sourced from *Dragonlance
    Adventures* pp.73-75 (Draconians) and p.78 (Thanoi), visually confirmed
    via rendered page images since OCR text badly scrambles this book's
    multi-column stat-block layout. Each monster's real signature ability
    (Bozak's 4th-level spellcasting, Sivak's shapeshifting, Aurak's mind
    control/dimension door/breath weapon, Thanoi's cold immunity) got the
    same flavor-only, unmodeled treatment this roster already gives Baaz's
    magic resistance and Kapak's paralysis-poison -- no new subsystem
    needed, confirming the backlog note's own caveat that the higher-tier
    draconians would need real mechanics this project doesn't have yet.
    None of the four print THAC0 (same gap DLA already has for Baaz/Kapak),
    so all four are derived via this project's own established
    HD-to-THAC0 empirical pattern -- previously validated only up to HD
    4+1, extended here to HD 6 (Sivak, THAC0 15) and HD 8 (Aurak, THAC0
    13), flagged explicitly as an extrapolation past its prior ceiling.
    Thanoi -- the walrus-man already referenced as flavor-only dialogue at
    Ice Wall since Milestone 36 -- is the first monster in the roster to
    carry `TERRAIN_BIAS` toward glacier, closing a gap Milestone 57
    explicitly left open ("nothing in the roster is Arctic-flavored"); no
    `EXCLUDE_TERRAIN` was added, since (unlike Gnoll's real Monstrous
    Manual Climate/Terrain field) *Dragonlance Adventures* prints no such
    field to hang a hard exclusion on. Pure data addition to
    `data/monsters.txt` -- confirmed directly against the source that
    `MonsterLoader.cpp`'s grammar already covered every field needed and
    that `GameLoop.cpp`'s only monster-id special case (Baaz's
    turn-to-stone message) wasn't needed here, so zero `.cpp`/`.h` changes,
    same shape as Milestone 34's four-monster batch. Verified via a
    throwaway self-test (the real 17-monster `data/monsters.txt` parses
    clean, all four new blocks' fields match, 4000 sampled draws on glacier
    terrain confirm Thanoi's `TERRAIN_BIAS` is real weighting rather than
    an accidental exclusive lock, and a malformed-line case still fails
    fast), a clean `/W4` rebuild (zero new warnings, no source changes),
    and the piped character-creation smoke test (with the user's real save
    moved aside and restored afterward, per the established procedure). No
    interactive playtest needed -- pure content, same as Milestone 34's
    own precedent -- though seeing a Thanoi turn up while walking Icewall
    Glacier would be a nice, easy live confirmation of the new bias
    wiring. See `docs/COMBAT_NOTES.md`'s monster-roster sourcing list,
    "Terrain-specific monster pools," and "Extending this later" (which
    now flags real Bozak/Sivak/Aurak mechanics as a possible future
    *engine* milestone, not a commitment).

65. Staff of Striking/Curing -- the item Milestone 56 had explicitly
    flagged by name as needing a "charges" subsystem, closing out the
    NEXT UP item Milestone 64 pointed at. Re-read the full "Magical Items
    of Krynn" chapter directly from rendered page images (book pp.91-99,
    not Milestone 56's secondhand summary) before committing scope --
    confirmed Scrolls/Rods-minus-one/Crystals/Misc Magic still need
    unbuilt subsystems and "Special Magical Items of Krynn" (pp.95-99) is
    entirely unique artifacts bound to named canon owners, correctly out
    of scope by existing precedent, but also surfaced a real gap in
    Milestone 56's earlier pass: **Frostreaver** (p.94, Weapons), a heavy
    battle axe of Icewall Glacier ice tied to the already-shipped Ice
    Wall location and Thanoi monster, not a unique artifact and so not
    covered by the exclusion list. Asked the user to choose between it and
    the Staff (`AskUserQuestion`, same practice as Milestones 54/56/58);
    they picked the Staff, deferring Frostreaver to a future pass (see
    NEXT UP). Two more real gaps needed the user's call before planning
    could lock down: DLA's own text never gives a heal amount for the
    staff's curing function, and the project's 2e DMG has no "Staff of
    Curing" entry to borrow from (confirmed absent by a direct text
    search, not an OCR gap, since it's a 1st-edition-only item DLA
    references without restating) -- user chose 1d8, reusing the
    already-PHB-sourced Cure Light Wounds dice over inventing an unrelated
    number. And: quest reward, Cleric-only, over a shop item -- a
    permanent +3 weapon plus rechargeable healing outclasses anything
    currently sold, so it's gated like Solamnic Armor rather than sold
    like Webnet/Brooch. A design finding surfaced during planning (not a
    question, since it followed directly from those two answers): the
    book's 50-charge pool becomes vestigial once its "no more than once
    per day on a given individual" cap is the actual binding constraint --
    this engine tracks exactly one player character, so the only possible
    "individual" is the player, and 5/day recharge always outpaces at-most-
    2/day consumption once the book's separate double-damage-striking mode
    is cut (needs real-time cooldown tracking this engine has never had,
    same gap already flagged for the Golden Circlet/Flute of Wind
    Dancing). Modeled instead as a flat once-per-day self-heal,
    `Character::lastStaffCureDay`, the identical shape as
    `activateBrooch`/`broochAvailableToday`'s existing day-gate --
    genuinely less code than the charge pool would have needed, not a
    compromise. The striking side needed zero new mechanism at all: an
    ordinary `ItemKind::Weapon` (magicBonus +3, 1d6 base = the book's
    "4-9 points of damage") reuses every piece of the existing
    `MagicWeapon` machinery -- equip/unequip, resale-blocking (falls
    through the existing code path to unsellable automatically, no
    special-casing needed), and the general inventory screen's equip-on-
    Enter. New quest `staff_of_striking_curing` (`REQUIRE cleric`, `SLAY
    skeleton 2`, `REWARD_STAFF_OF_STRIKING_CURING`), offered by a new POI
    at Xak Tsaroth -- "A Ruin-Scavenger" (`S`), the zone's first talkable
    NPC, a deliberate, documented departure from that zone's "no talkable
    NPC, the ruins are abandoned" precedent (someone passing through,
    not a resident, grounded in the same relic-hunting flavor DLA's own
    Bupu's Emerald sources to these ruins). `SaveGame.cpp` touches: one
    new optional `STAFFCUREDAY` line, same backward-compatible shape as
    `RESTDAY`/`BROOCHDAY`; no inventory-format change, since the weapon
    stats ride the existing `MAGICWEAPON` line unchanged. Verified via a
    throwaway self-test (ownership/day-gate/heal-cap behavior;
    `QuestLoader` against the real, now-ten-quest `data/quests.txt`
    including the new reward keyword's fail-fast case; a `SaveGame`
    round-trip covering `STAFFCUREDAY` present and absent), a clean `/W4`
    rebuild (zero new warnings), and a direct piped run confirming the
    user's real save loads cleanly under the new format -- this run also
    exercised every data loader including the new quest/zone content,
    since it reaches the character-creation EOF-fail point cleanly with
    the real save present. Interactive verification (talking to the
    Scavenger as a Cleric, clearing the skeletons, equipping the staff,
    using its combat cure action) still needs the user's own keyboard, the
    same `_getch()` limitation flagged for every quest milestone so far.
    See `docs/CHARACTER_NOTES.md`'s "Magic items" and `docs/QUEST_NOTES.md`'s
    "Shipped quests".

66. Aftermath dialogue -- the timeline schedule could already show who's
    present at a location today, but nothing acknowledged a player arriving
    *after* the Heroes had already come and gone. New zone grammar,
    `TALK_AFTER <char> <dialogue...>` (`world::PointOfInterest::
    dialogueAfter`, parsed by `ZoneLoader` exactly like `TALK_AGAIN`, with
    the same "must already have a POI with a TALK line" validation `SAY_IF`/
    `TOPIC`/`BOAT` already require), backed by a new pure query,
    `timeline::Timeline::latestDayEnd(locationId)` (the max `dayEnd` across
    every character's `PresenceWindow` there, or -1 if none). A zone POI
    carrying `TALK_AFTER` shows it instead of the ordinary `TALK`/
    `TALK_AGAIN` line the first time it's talked to once
    `dayNow > latestDayEnd(effectiveId) >= 0` -- tracked under its own
    synthesized `"<id>:after"` `GameState::metCharacters` entry (same
    met-id-naming precedent `TIMELINE_ANCHOR` already established) so it
    fires correctly even for a player who met the NPC before the Heroes'
    window ever opened, which the ordinary alreadyMet/`TALK_AGAIN` check
    alone would otherwise mask. No save-format change -- reuses the existing
    `MET` line as-is. One proof-of-concept POI, `data/zones/solace_inn.txt`'s
    Otik, referencing the shared `PRESENCE solace 0 1` reunion and the day
    2-3 Haven/Darken Wood split that follows it, deliberately hedged between
    the two rather than picking one, matching that window's own established
    two-versions-of-one-leg ambiguity (`docs/TIMELINE_NOTES.md`). Widening to
    other zones is deliberately left for later, same "one proof-of-concept
    first" precedent the quest engine set (Milestone 51, widened in
    Milestone 52). Verified via a throwaway self-test (`latestDayEnd` against
    synthetic multi-character/multi-window schedules; `ZoneLoader` parsing
    `TALK_AFTER` correctly plus its two fail-fast cases), a clean `/W4`
    rebuild (zero new warnings), and the piped smoke test (confirms the
    modified `solace_inn.txt` still parses). Interactive verification
    (talking to Otik before day 2, after day 2 to see the aftermath line fire
    once, then a third time to confirm the fallback to `TALK_AGAIN`) still
    needs the user's own keyboard. See `docs/ZONE_NOTES.md`'s "Aftermath
    dialogue" section and `docs/TIMELINE_NOTES.md`'s own section on
    `latestDayEnd`.
67. Movement granularity, hours -> minutes -- user feedback while playing:
    ordinary movement was consuming the Heroes' schedule far faster than
    intended. Only two things ever advanced `GameState::hoursElapsed` (the
    clock `timeline::Timeline` checks every `PRESENCE` window against):
    overworld movement and Rest. `world::TerrainInfo::hoursToCross` was a
    flat, whole-hour cost per tile (1-6 hours) -- even the cheapest terrain
    consumed a full hour per single keypress, and the earliest window
    (Solace, day 0-1, 48 hours) could be exhausted by ordinary local
    wandering alone. Renamed to `minutesToCross` and rescaled x15 (15-90
    minutes, same relative tuning between terrain types, just four times
    finer-grained), fed into a new `GameState::minutesElapsed` remainder
    (0-59, `GameLoop::tryMoveOverworld`) that rolls into the existing
    `hoursElapsed` on overflow -- `hoursElapsed` itself, and everything that
    reads it for day/hour math, is completely unchanged. `data/timeline.txt`'s
    day windows and Rest's flat 8-hour cost are both deliberately untouched
    -- the windows are individually sourced against each novel's own
    elapsed-time cues (rescaling them would invalidate that research), and a
    night's rest is still a night's rest regardless of movement granularity.
    New optional `MINUTES <n>` save line, same backward-compatible shape as
    `RESTDAY`/`BROOCHDAY`/`STAFFCUREDAY` -- a save from before this milestone
    simply has no `MINUTES` line, and `minutesElapsed`'s `0` default is
    already correct for it. Verified via a throwaway self-test (rollover
    arithmetic at the 60-minute boundary; `SaveGame` round-tripping a
    nonzero `minutesElapsed`; a hand-edited save missing the `MINUTES` line
    still loading cleanly), a clean `/W4` rebuild (zero new warnings), and
    the piped smoke test. See `docs/MAP_NOTES.md`'s "Movement granularity"
    section.
68. Anticipation dialogue -- the direct follow-up the user asked for after
    Milestone 67: since ordinary movement now typically lands a player
    *before* a location's `PRESENCE` window opens rather than mid-window
    (concretely, Haven -- a 40-hour walk against a window that doesn't open
    until hour 48), arriving early was silent, `Timeline::presentAt` simply
    returning nothing. New `timeline::Timeline::earliestDayStart(locationId)`
    (the mirror-image query to Milestone 66's `latestDayEnd` -- earliest
    `dayStart` instead of latest `dayEnd`) backs a new `TALK_BEFORE <char>
    <dialogue...>` zone grammar (`world::PointOfInterest::dialogueBefore`,
    parsed by `ZoneLoader` exactly like `TALK_AFTER`, same "must already have
    a TALK line" validation). Deliberately **not** the same "exactly once"
    treatment `TALK_AFTER` uses, though: anticipation dialogue is an ongoing
    truth ("they still aren't here"), not a one-time event, so it's shown on
    *every* visit while `dayNow < earliestDayStart`, with no
    `GameState::metCharacters` tracking id needed at all -- the condition
    stops firing on its own once the Heroes actually arrive. Same
    zone-native-POI-only scope as `TALK_AFTER`, and the same reasoning for
    why (`TIMELINE_ANCHOR` already goes silent-then-populated for free via
    `presentAt`). One proof-of-concept POI, `data/zones/haven.txt`'s Seeker
    Guard, reacting to the shared `PRESENCE haven 2 3` window -- not
    Otik/Solace, since Solace's own window starts at day 0 and so has no
    "before" period to demonstrate. Verified via a throwaway self-test
    (`earliestDayStart` against synthetic multi-character/multi-window
    schedules, including the "-1, no schedule here" case; `ZoneLoader`
    parsing `TALK_BEFORE` correctly plus its two fail-fast cases), a clean
    `/W4` rebuild (zero new warnings), and the piped smoke test (confirms
    the modified `haven.txt` still parses). Interactive verification
    (talking to the Guard before day 2 to see the anticipation line repeat
    across visits, then on/after day 2 to confirm the fallback to ordinary
    `TALK`/`TALK_AGAIN`) still needs the user's own keyboard. See
    `docs/ZONE_NOTES.md`'s "Anticipation dialogue" section and
    `docs/TIMELINE_NOTES.md`'s own section on `earliestDayStart`.
69. Character creation redesign: screen-per-step, colorized, Method V
    dice -- a user request to mimic two reference screenshots
    (`References/abilityscore.png`, a "STEP 1: ABILITY SCORES" wizard
    screen; `References/Bobs_Game.png`, the same "Bob's game" already
    cited in Milestone 43 for its color palette). Two changes bundled
    together: ability scores, race, class, and alignment each became
    their own cleared screen (`STEP n: TITLE`, bright yellow) with the
    running scores recapped at the top of every later step and, new for
    race, a "Race Adjustments" recap showing each changed ability's
    before/after/delta; and the whole wizard picked up the project's
    existing ANSI palette (`\x1b[93m` yellow headers, `\x1b[96m` cyan
    labels, `\x1b[97m` white name banner, plus one new code, `\x1b[92m`
    bright green, for assigned/confirmed values), reusing exactly what
    `MapRenderer.cpp` already established rather than inventing a new
    palette. The reference screenshot's own flavor line ("Choose wisely,
    Echoborn" — tied to that other game's race name) was dropped per the
    user's explicit instruction.

    **The dice method changed too, deliberately, not incidentally.** The
    reference screen's actual interaction ("Assigning: Strength", pick
    from a pool of rolled values) turned out to be a specific named PHB
    method once checked against the real rulebook (rendered as a page
    image, since this page's two-column layout mis-orders under
    `pdftotext -layout`): Method V, "roll 4d6 six times, drop the lowest
    die each time, assign the six results to abilities however you
    want" (PHB p.19, verbatim). This directly reopened a previously
    documented decision -- `docs/CHARACTER_NOTES.md` recorded that the
    project owner had specifically chosen Method I (3d6 straight down
    the fixed line, free whole-set reroll) *over* Method II (4d6 drop
    lowest, arrange to taste) in the past. Surfaced directly rather than
    assumed either way; the user confirmed switching to Method V. New
    `character::roll4d6DropLowest()` (`character/Dice.h`/`.cpp`); the
    pre-existing free-whole-set-reroll house rule carries over unchanged,
    now applied to the six Method V rolls before assignment. See
    `docs/CHARACTER_NOTES.md`'s "Ability score generation" for the full
    sourcing and the superseded-decision note.

    **No live arrow-key cursor.** `docs/ARCHITECTURE.md`'s "Character
    creation" section explains why `CharacterCreator::run()` deliberately
    stays on plain `std::cin`/`std::cout` rather than
    `render::Console::readKey()`: it's the one part of the game a
    piped/redirected script can drive end-to-end (`docs/GOTCHAS.md`). The
    reference screenshot's live-highlighted green `>` needs real
    single-keypress input to work, so the "Assigning: Strength" screen
    instead shows the remaining rolled-value pool as a numbered list and
    the player types the number -- the same `promptChoice` interaction
    the race/class/alignment menus already used, just applied one more
    place. Screen-clearing uses the raw `\x1b[2J\x1b[H` VT100 sequence
    emitted inline (matching how `MapRenderer.cpp` already does it)
    rather than calling `render::Console::clearScreen()`, specifically so
    `character/` keeps its zero-dependency-on-`render/` status from
    `docs/ARCHITECTURE.md`'s module map -- no new `#include`, nothing to
    update there. Also added `character::abilityName(Ability)`
    (`character/Ability.h`/`.cpp`), the same "one name() function per
    enum with a display name" pattern `alignmentName`/`saveCategoryName`
    already established.

    Verified two ways beyond the usual pair: a throwaway self-test
    confirmed `roll4d6DropLowest()` stays in `[3, 18]` with a measured
    200,000-trial average of 12.243 (PHB Method V's known ~12.24, not
    `roll(3,6)`'s flat 10.5 in disguise); and, because this module fully
    tolerates piped `std::cin` unlike the rest of the game, a complete
    scripted run (name, keep-rolls, six assignment picks, an Elf +
    Silvanesti-heritage race pick to exercise the subrace/adjustments
    path, Fighter, Lawful Good) was piped through the real executable
    end-to-end and inspected -- confirming every screen, color code, and
    the race-adjustments math (Silvanesti Elf: Dex 9→10 (+1), Con 13→12
    (-1)) rendered correctly, and that the resulting character
    successfully reached the real game loop afterward. `save.txt` was
    moved aside first and restored after, per project convention. Also a
    clean `/W4` rebuild, zero new warnings. Actually *looking* at the
    colors in a real terminal is left to the user.

70. Widened aftermath dialogue (`TALK_AFTER`) beyond Otik -- the direct
    follow-up NEXT UP item #5 named after Milestone 66 shipped the
    mechanism as a single proof of concept. Four more POIs, picked exactly
    from that item's own named candidates: `data/zones/haven.txt`'s `G`
    (Seeker Guard, reacting to the shared `PRESENCE haven 2 3` window),
    `data/zones/xak_tsaroth.txt`'s `S` (Ruin-Scavenger, `PRESENCE
    xak_tsaroth 4 6`), `data/zones/qualinesti.txt`'s `E` (Elven Sentinel,
    `PRESENCE qualinesti 7 9`), and `data/zones/kalaman.txt`'s `G` (City
    Watchman, covering both `PRESENCE kalaman 90 92` and `100 100` in one
    line, since `latestDayEnd` only fires once every window at that
    location has closed). Every line reframes a detail already established
    in that location's own `PRESENCE` flavor text or the POI's own existing
    voice rather than inventing new lore -- the Seeker Guard's line
    callbacks to Tasslehoff's `PRESENCE` line badgering that exact guard
    with theology questions (and the guard's own pre-existing "headache"
    joke); the Ruin-Scavenger's line callbacks to Tasslehoff diving down a
    stairwell to poke through the ruins, a fellow scavenger in effect; the
    Elven Sentinel's line callbacks to Tasslehoff being trailed by two
    elven sentries (her being one of them); and the City Watchman's line
    callbacks to his own established "light fingers"/purses joke and the
    zone's already-existing locked Cartographer's Stall POI, which
    Tasslehoff's own `PRESENCE` line has him picking. Same "reframe an
    existing NPC's established voice, don't invent a new one" approach the
    quest-widening pass (Milestone 52) already used successfully. Pure data
    content -- zero `.cpp`/`.h` changes, since the `TALK_AFTER` grammar,
    parser, and runtime behavior were already generic and zone-agnostic.
    Verified via a clean `/W4` rebuild (zero new warnings) and the piped
    smoke test (confirms all four modified zone files still parse); no
    throwaway self-test needed, same call Milestone 47 made for a similarly
    pure-content pass. Interactive verification (reaching each zone after
    its `latestDayEnd`, confirming the aftermath line fires once and falls
    back to `TALK_AGAIN` after) still needs the user's own keyboard, same
    limitation every prior `TALK_AFTER`/`TALK_BEFORE` milestone has flagged.
    See `docs/ZONE_NOTES.md`'s "Aftermath dialogue" section.

71. "Ask about anything" -- free-text conversation subjects, at the user's
    explicit request to go deep on NPC dialogue: instead of only picking
    from a curated `TOPIC` menu, the player can now type any subject at
    all. New `SUBJECT <keywords> <text>`/`SUBJECT_UNKNOWN <text>` grammar
    (char-prefixed for zone files, same as `SAY_IF`/`TOPIC`) in both
    `data/timeline.txt` and `data/zones/*.txt`, a new `Speech::SubjectEntry`
    list on the existing `Speech` struct, and two new free functions
    (`game::tokenizeAskInput`/`game::matchSubject`, unit-tested via the
    throwaway self-test pattern) doing simple lowercase/punctuation-
    stripped whole-word keyword matching -- no NLP, no fuzzy matching,
    same "checked in authored order, first match wins" convention as
    `SAY_IF`. `GameLoop::talkTo`'s existing topic picker (already labeled
    "Ask <name> about...") gained one more row, "Ask about something
    else...", shown whenever any `SUBJECT` content exists, opening a new
    free-text prompt (`MapRenderer::drawAskInputFrame` +
    `Console::readLine`). `Console::readLine` deliberately reads via the
    same raw `_getch()` primitive `readKey()` already uses rather than
    `std::cin`/`getline` -- `CharacterCreator`'s own `std::cin` usage only
    ever runs *before* `GameLoop`'s raw-keypress loop starts, so mixing the
    two inside one live session is untested territory this project has
    never needed before, and `readLine` sidesteps it entirely rather than
    gambling on it (see `docs/GOTCHAS.md`). Its cancel key is Esc, not `q`,
    a deliberate deviation from this game's usual Quit convention since `q`
    is an ordinary character a player might type in a real question.
    Content shipped as a single proof of concept, same "prove it out
    narrow, widen later" restraint `SAY_IF`/`TOPIC` (Milestone 19),
    `TALK_AFTER` (Milestone 66), and `TALK_BEFORE` (Milestone 68) each
    followed: Raistlin's `PRESENCE solace 0 1` window gained three real
    `SUBJECT` entries -- Kitiara (freshly written, sourced from
    `.research/dat_full.txt`'s actual Inn-of-the-Last-Home letter scene:
    "Who knows with Kitiara? ... she has sworn allegiance to another. She
    is, after all, a mercenary."), Caramon (leveraging the twin dynamic
    already established in this same window), and magic/the Test (a
    keyword-reachable variant of the existing `TOPIC "The Towers of High
    Sorcery"`, not a change to that `TOPIC` itself) -- plus a
    `SUBJECT_UNKNOWN` in his own dismissive voice, which doubles as the
    natural in-character answer to a forward-referencing question the
    timeline hasn't reached yet (e.g. a title from later in his own
    in-universe arc) without any special-casing. No other Hero, window, or
    zone NPC has `SUBJECT` content yet. Verified via a throwaway self-test
    (tokenization/matching: case-insensitivity, punctuation stripping,
    whole-word-only matching, first-match-wins ordering), a clean `/W4`
    rebuild (zero new warnings), and the piped smoke test (confirms
    `data/timeline.txt`'s new grammar still parses). **Interactive
    verification of the free-text prompt itself (typing "Kitiara," typing
    something unrelated, Backspace/Enter/Esc while typing) still needs the
    user's own keyboard** -- piped stdin can't drive `_getch()`, and this
    feature adds a second raw-input mode on top of that, same limitation
    every prior picker/quest-UI milestone has flagged. See
    `docs/TIMELINE_NOTES.md`'s and `docs/ZONE_NOTES.md`'s "Ask about
    anything" sections.

72. Character-level subject pools and day-gated knowledge -- Milestone 71's
    `SUBJECT` was window-only, which doesn't scale: Raistlin alone has eight
    talkable windows, and hand-copying twenty-five subjects into each would
    drift out of sync within two content passes. `SUBJECT`/`SUBJECT_UNKNOWN`
    are now legal *before* a `CHARACTER` block's first `PRESENCE` line too,
    where they attach to the character as a whole (same positional
    convention `NAME` already uses) rather than to one window. A new
    `SUBJECT_WHEN <d0> <d1> <keywords> <text>` line (character-level only,
    an error after the first `PRESENCE`) adds a day-range gate on top of
    that -- plain character-level `SUBJECT` is exactly `SUBJECT_WHEN 0 -1`.
    At talk time on the current in-game day, the candidate list is that
    window's own `SUBJECT` entries first, then the character's day-filtered
    pool -- `game::matchSubject` runs over the concatenation unchanged
    (first match wins, same as `SAY_IF`), so a window `SUBJECT` sharing a
    keyword with a pool entry silently overrides it, and two `SUBJECT_WHEN`
    rows sharing a keyword with adjacent ranges give an evolving answer for
    free. Two new pure `timeline::Timeline` queries (`subjectsFor`/
    `subjectUnknownFor`, same "tiny, return by value" shape as
    `latestDayEnd`/`earliestDayStart`) do the resolution; `GameLoop`'s
    `speechFromWindow` adapter (both the overworld and `TIMELINE_ANCHOR`
    talk paths) is the only caller -- `talkTo`, `TalkCandidate`,
    `pickAndTalk`, `tokenizeAskInput`, and `matchSubject` itself are
    unchanged, the same "the executor was always source-agnostic" precedent
    Milestones 26 and 71 both already established. `TimelineLoader` also
    gained a keyword-collision check: after loading each `CHARACTER` block,
    it warns (stderr, `file:line`, never a load failure) about any keyword
    reachable from two entries whose day ranges overlap, since a deliberate
    window-over-pool override is indistinguishable at load time from an
    accidental duplicate -- authoring rule: order specific keywords before
    general ones. Content-wise, Raistlin's three Solace-only subjects
    (`caramon`/`brother`, `magic`/`test`/`towers`/`sorcery`/`tower`, and
    `SUBJECT_UNKNOWN`) were promoted to character-level unchanged; Kitiara's
    Inn-letter-scene entry stayed window-scoped at Solace, with a new,
    deliberately non-committal character-level Kitiara entry covering his
    other seven windows (left ungated, not split into a `SUBJECT_WHEN`
    pair, because every one of those windows closes by day 30 -- well
    before Kitiara's Dragon Highlord reveal at the `high_clerist_tower 81
    81` siege -- so no in-game moment exists where the player could ask him
    about her after the fact). Fourteen new always-true subjects were added
    on top of that: his hourglass eyes, golden skin, the Test/Wayreth/
    Conclave, the Staff of Magius, Par-Salian, the three orders/robes/his
    own neutrality, the three moons, his health/cough/blood, his mother and
    father, and one opinion entry each for Tanis, Sturm, Flint, Tasslehoff,
    Goldmoon, and Riverwind (Caramon's was already covered by the migrated
    entry) -- grounded in the Solace reunion scene already used throughout
    this file (`.research/dat_full.txt`), the orders/moons material in
    `.research/dla_full.txt`, and two freshly-extracted passages for the
    parents, neither of whom is ever named in this project's source
    library: `.research/wott_full.txt` lines 12952-12993 (his mother --
    "magic in her blood," weak-willed, died young) and
    `.research/testott_full.txt` line 4784 (his father -- "a poor
    woodcutter," a "perpetual look of worry and care"). A research pass
    then covered every remaining candidate against this game's
    already-modeled timeline (not assumed from canon memory), for the
    user's review before any gate was written -- three names (Khisanth,
    Verminaard, and the Disks of Mishakal) were confirmed groundable, all
    at day 4 (`xak_tsaroth`'s own `PRESENCE` start): `.research/dat_full.txt`
    lines 6990-7035 (Mishakal's temple vision, naming the Disks and
    Khisanth directly) and line 6075 ("confer with Lord Verminaard about
    the staff"), following the same "this file's own established facts
    win over book chronology" reasoning the spec's own Fistandantilus case
    laid out -- Raistlin isn't the on-page witness for either passage, but
    per Milestone 24's standing "whole company experiences a stop
    together" abstraction, that doesn't disqualify it the way the book
    explicitly sending Goldmoon away at Pax Tharkas would. Each got a
    `SUBJECT_WHEN 0 3`/`SUBJECT_WHEN 4 -1` pair (an in-voice "I don't know
    that name yet" reaction, then the real answer), plus a matching
    `gods`/`mishakal`/`faith` "true gods" pair at the same day-4 boundary.
    Takhisis moved to the always-true set instead of being gated -- her
    proper name never appears in any novel in this project's library (only
    in the DLA rulebook), and the novels' own "Queen of Darkness"/"Dark
    Queen" title is already in play from the book's opening prologue, so
    there was no scene-boundary to gate against. Dragon orbs, Astinus, and
    Laurana were dropped outright (this file's own Xak Tsaroth content
    already frames Raistlin's vault find as the spellbook, not an orb, and
    neither Astinus nor Laurana appears before Raistlin's last talkable
    window closes) -- and a further well-grounded group (a dedicated
    `fistandantilus` name subject, `draconians`, `bupu`, `cyan`/
    `bloodbane`, `lorac`, `alhana`/`starbreeze`) was deliberately left for
    a later pass at the user's own call, not a sourcing gap; see NEXT UP
    for the citations already on file for each. See
    `docs/TIMELINE_NOTES.md`'s "Ask about anything" for the full reasoning
    and citations.
    Section 4.6's tokenizer question was settled via the throwaway
    self-test: `game::tokenizeAskInput` keeps a literal hyphen inside a
    word rather than splitting on it, so "half-sister" tokenizes to one
    word and "half sister" (a space) still splits into two -- see
    `docs/GOTCHAS.md`. Verified via a throwaway self-test (day-range
    filtering at both edges and outside, `-1` open-ended, window-beats-pool
    ordering, the two-`SUBJECT_WHEN` evolving-answer case, `subjectUnknownFor`
    falling window -> character -> empty, the hyphen behavior, and the
    loader rejecting a post-`PRESENCE` `SUBJECT_WHEN`/backwards range/
    negative `dayStart`), a clean `/W4` rebuild (zero new warnings), and the
    piped smoke test (confirms the new grammar parses and the deliberate
    Kitiara collision warning fires as expected). **Interactive verification
    still needs the user's own keyboard** -- typing a gated-adjacent subject
    like "Kitiara" at Solace vs. at Silvanesti and confirming the Solace
    override still wins over the pool answer, and confirming the curated
    `TOPIC` menu and "Ask about something else..." row both still behave.
    See `docs/TIMELINE_NOTES.md`'s "Character-level subject pools and
    day-gated knowledge".

73. Colored dialogue/picker/combat screens, and re-picking after a talk --
    two small requests bundled together: extend the "Bob's game" ANSI
    palette (`docs/ARCHITECTURE.md`'s Milestones 43/69) beyond the
    overworld/zone status panel and character creation to the screens a
    player actually sees while talking to someone or fighting
    (`drawDialogueFrame`/`drawPickerFrame`/`drawCombatFrame`), and fix
    `GameLoop::pickAndTalk` so finishing a conversation with multiple
    people present returns to the "Talk to whom?" picker instead of
    dropping all the way back to the explore screen. A new `BoxLine`
    (text + optional whole-line ANSI color) and a parallel colored
    `wrapLongLines`/`writeBoxed` overload extend `writeBoxed`'s existing
    box-hugging pipeline without touching the plain-text overload every
    other "organic" screen still uses (character sheet, spellbook, shop,
    inventory, journal, help, the ask-input prompt -- deliberately left
    plain, narrower scope than "every screen"). Dialogue's speaker name
    (bright yellow, reusing "named thing in the world," the same meaning
    `colorLine` already gives `Standing On:`/location glyphs) sits on its
    own colored line rather than a colored "Name: text" prefix -- a whole-
    line-only constraint inherited from `colorLine` itself (pads to width,
    *then* wraps in the ANSI pair, so a substring can't be tinted without
    miscounting escape bytes as visible ones). The picker's selected row
    (bright white, matching the player's own `@` glyph) is colored inside
    the one shared `drawPickerFrame`, so every screen that reuses it --
    Talk to whom?, Ask about... topic menus, quest Accept/Decline, and
    Look's own picker -- picked it up for free. Combat colors the
    player's stat line bright white (same convention) and the monster's
    bright red (`\x1b[91m`, a genuinely new code -- nothing existing meant
    "hostile"); the scrolling combat log itself stays plain, matching the
    precedent that free-form log prose is never colored, only labels/
    named things. The `pickAndTalk` fix is one line: dropping the `return`
    after `talkTo(candidates[selected])` in the picker's `Enter` branch so
    the loop redraws the same picker instead of exiting; `q`/Quit at the
    picker itself is unchanged. Verified via the throwaway self-test
    pattern (a `ColorSelfTest.cpp` calling all three changed draw
    functions, output inspected with escape codes visible via `cat -v`,
    confirming every color-set code is immediately followed by its reset
    before the border and that bordered rows still align column-for-
    column across colored and uncolored lines -- see `docs/GOTCHAS.md`),
    a clean `/W4` rebuild (zero new warnings), and the piped smoke test.
    Real in-terminal color rendering, and the `pickAndTalk` fix's actual
    keypress behavior, still need the user's own keyboard -- same
    `_getch()` limitation as every prior interactive-UI milestone. See
    `docs/ARCHITECTURE.md`'s "Colored dialogue/picker/combat screens, and
    re-picking after a talk".

74. Showing the math behind attacks -- at the user's request, the combat
    log now shows the real PHB roll math behind every weapon swing, not
    just the hit/miss result and final damage. `combat::AttackOutcome`
    (`Combat.h/.cpp`) gained the roll breakdown both
    `resolvePlayerAttack`/`resolveMonsterAttack` already computed
    internally but previously discarded -- `naturalRoll`, `toHitBonus`,
    `attackerThac0`/`defenderArmorClass` (already folded in any this-fight
    spell buff/penalty), the resulting `targetNumber`, and, on a hit, the
    damage dice spec/roll/bonus -- with the underlying hit/miss and damage
    logic itself byte-for-byte unchanged, just exposed instead of thrown
    away. New file-local `game::describeToHit`/`describeDamage`
    (`GameLoop.cpp`) render that breakdown as a bracketed suffix on the
    existing log line, e.g. `You hit the Goblin for 6. [d20 14 +2 = 16 vs
    THAC0 18 - AC 6 (need 12)] [1d8 5 +1 = 6]`, with a distinct natural-20/
    natural-1 phrasing for the PHB's always-hit/always-miss override.
    Scoped to just the two real `resolvePlayerAttack`/`resolveMonsterAttack`
    call sites (`playerAttacks`/`monsterAttacks`) -- spell damage already
    states its amount plainly and has no "roll vs AC" attack math to show,
    so it's untouched. Hit entries now typically wrap to two physical lines
    instead of one, so `drawCombatFrame`'s `kMaxLogLines` was trimmed 12->8
    in the same change to keep the box roughly its old height. Verified via
    a throwaway self-test (`CombatMathSelfTest.cpp`, 10,000 rolls split
    across both attack directions, asserting every new field's formula
    including the natural-20/natural-1 override, deleted after passing), a
    clean `/W4` rebuild (zero new warnings), and the piped smoke test. Real
    in-terminal rendering of the new log lines still needs the user's own
    keyboard, same `_getch()` limitation as every prior combat-UI
    milestone. See `docs/COMBAT_NOTES.md`'s "Showing the math" section.

75. Raistlin's deferred second "ask about anything" group -- the NEXT UP
    item 6(b) Milestone 72 deliberately deferred, even though the citations
    were already found and sitting in this file. Six new character-level
    subjects (`draconians`, a dedicated `fistandantilus` name-keyword
    subject, `bupu`, `alhana`/`starbreeze`, `cyan`/`bloodbane`, `lorac`),
    each a `SUBJECT_WHEN 0 <d>` / `SUBJECT_WHEN <d> -1` pair -- a
    pre-knowledge "doesn't know it yet" snap, then the real answer once the
    player's reached the window that establishes it -- same shape as
    Milestone 72's own Khisanth/Verminaard/Disks/gods group. Gated at
    `darken_wood`'s day 2 (draconians), `xak_tsaroth`'s day 4
    (fistandantilus, bupu), `tarsis`'s day 20 (alhana/starbreeze), and
    `silvanesti`'s day 25 (cyan/bloodbane, lorac) -- this file's own
    already-modeled timeline deciding the gate, not book chronology, same
    precedent Milestone 72 established. Every citation was re-verified
    directly against the `.research/*.txt` extractions before writing, not
    trusted from the old summary blind: the Forestmaster scene naming
    draconians and the "Order of Draco," Bupu's introduction and goodbye
    scene (where she gives Raistlin Fistandantilus's own spellbook -- the
    direct payoff of the existing `TOPIC "The Spellbook in the Vault"`),
    Alhana Starbreeze's on-page arrest and naming at Tarsis, and Raistlin's
    own on-page account of Cyan Bloodbane and Lorac at the Tower of the
    Stars. Pure data content -- zero `.cpp`/`.h` changes, no new grammar
    (`SUBJECT_WHEN` already existed), so no throwaway self-test needed, same
    precedent as Milestones 47-50. Verified via a clean rebuild (zero new
    warnings; not strictly required for a data-only change but run anyway
    for consistency) and the piped smoke test, including checking stderr
    for `TimelineLoader`'s keyword-collision warning -- none of the six new
    keyword sets collide with any existing Raistlin subject, confirmed both
    by inspection and at runtime (the one warning that does fire, the
    pre-existing `kitiara` Solace-window override, predates this change and
    is already documented as an intentional override). Real in-terminal
    verification of the new gated answers still needs the user's own
    keyboard, same `_getch()` limitation as every prior dialogue milestone.
    See `docs/TIMELINE_NOTES.md`'s "Ask about anything" section.

76. Widened "ask about anything" to the other seven Heroes -- the NEXT UP
    item 6 Milestone 75 deferred, built all seven at once rather than
    proving the pattern on one first (asked directly, user's explicit
    choice). Tanis, Caramon, Flint, Goldmoon, Riverwind, Sturm, and
    Tasslehoff each gained a character-level `SUBJECT` pool in
    `data/timeline.txt` -- 1-2 self-identity subjects generalized from
    that character's own existing `PRESENCE`/`SAY`/`TOPIC` content (the
    same "promote a window `TOPIC` into the character pool" move
    Milestone 72 used for Raistlin), one opinion subject about each of
    the other seven Heroes, and a `SUBJECT_UNKNOWN` in their own voice --
    roughly 85 new `SUBJECT`/`SUBJECT_WHEN` lines, zero `.cpp`/`.h`/
    grammar changes. No fresh PDF extraction needed: every fact and
    relationship beat was already sourced and shipped in this project's
    own existing dialogue for these seven; this pass only generalizes it.
    Day-gated only the specific (asker, subject) pairs where an ungated
    answer would otherwise contradict a later window: Sturm's own
    knighthood self-subject (day 76, aspirant vs. sworn) and every
    surviving Hero's opinion-of-Sturm (day 81, his death),
    opinion-of-Raistlin (day 83, his ambiguous Palanthas collapse -- never
    confirmed dead, matching the existing Milestone 44 framing), and
    opinion-of-Flint (day 103, his death, only for Tanis/Caramon/
    Tasslehoff, whose schedules actually extend past it) -- see
    `docs/TIMELINE_NOTES.md`'s "Ask about anything" section for the full
    per-boundary breakdown. Verified via a clean `/W4` rebuild (zero new
    warnings, pure data) and the piped smoke test, confirming stderr shows
    no new `TimelineLoader` keyword-collision warnings (the collision
    check runs per-`CHARACTER` block, so the seven new pools can't
    collide with each other or Raistlin's; the one warning that still
    fires, the pre-existing Kitiara Solace-window override, predates this
    milestone). No throwaway self-test needed -- pure data, existing
    grammar, same precedent as Milestones 47-50/75. Real in-terminal
    free-text asking across all seven still needs the user's own keyboard,
    same `_getch()` limitation as every prior dialogue milestone.

77. Widened aftermath dialogue (`TALK_AFTER`) to five more zones -- NEXT UP
    item 5, picked exactly from its own named candidates: `data/zones/
    darken_wood.txt`'s `U` (the unicorn/Forestmaster, `PRESENCE darken_wood
    2 3`, all eight Heroes), `data/zones/high_clerist_tower.txt`'s `K`
    (Garrison Knight, covering both `76 80` and `81 81`), `data/zones/
    ice_wall.txt`'s `K` (Young Knight, `38 42`), `data/zones/silvanesti.txt`'s
    `W` (Silvanesti Warder, `25 30`), and `data/zones/palanthas.txt`'s `K`
    (Knight of the Watch, covering both `83 83` and `83 89`). Same "reframe
    an existing NPC's established voice using details already in that
    location's own `PRESENCE` text, refer to Heroes by epithet not name"
    approach Milestone 70 used successfully -- the Forestmaster's line counts
    off all eight Heroes' own `darken_wood 2 3` flavor text through her
    established judging-by-what-a-life-gives-vs-takes voice; the Tower's
    Garrison Knight's line covers Sturm's Knighting and death, Flint's hand on
    the ambush lever, and Laurana's eulogy in one witness account; Ice Wall's
    Young Knight's line covers Laurana's stillness against the Ice Reaver's
    fear magic alongside Sturm/Flint/Tasslehoff; the Silvanesti Warder's line
    covers the five-Hero subgroup's river crossing without naming Alhana
    Starbreeze or Tika, preserving Milestone 59's precedent of keeping this
    POI generic and separate from her; and Palanthas's Knight of the Watch's
    line ties Raistlin's ambiguous collapse at the library together with
    Laurana's rise to Golden General, a detail his own pre-existing `TOPIC
    K "A City Under Watch"` already referenced. Pure data content -- zero
    `.cpp`/`.h` changes, since the `TALK_AFTER` grammar, parser, and
    `Timeline::latestDayEnd`-keyed runtime were already generic and
    zone-agnostic. Verified via a clean `/W4` rebuild (zero new warnings) and
    the piped smoke test (confirms all five edited zone files still parse;
    the one warning in stderr, the pre-existing Kitiara Solace-window
    override, predates this milestone); no throwaway self-test needed, same
    call Milestone 70 made for an identical pure-content pass. Interactive
    verification (reaching each zone after its `latestDayEnd`, confirming
    each line fires once and falls back to `TALK_AGAIN` after) still needs
    the user's own keyboard, same limitation every prior `TALK_AFTER`
    milestone has flagged. See `docs/ZONE_NOTES.md`'s "Aftermath dialogue"
    section.
78. Frostreaver -- NEXT UP item 1, deferred once already (in favor of the
    Staff of Striking/Curing) since the Dragonlance-magical-items milestone
    first read DLA's "Magical Items of Krynn" chapter closely. Sourced from
    DLA p.94 (Weapons) and PHB Table 44 (Weapons, p.94), both visually
    confirmed via rendered page images this session (`pdftoppm`, previously
    flagged unavailable in this environment, was re-checked and works
    fine): *"the equivalent of a heavy battle axe +4... can only be wielded
    by a character with a Strength of 13 or greater"* -- the PHB has no
    separate "heavy battle axe" line, so its one axe entry (1d8) is the
    base weapon the "+4" applies on top of. A quest reward
    (`data/quests.txt`'s `frostreaver_salvage`, `REQUIRE str_13` -- a new
    `game::conditionMatches` token and this project's first quest
    requirement keyed on a raw ability score), offered by Ice Wall's
    *existing* Young Knight POI rather than a new NPC (he already carries
    established Thanoi-flavor dialogue; the axe is framed as salvage from a
    dead Ice Folk raider, avoiding a talking Ice Folk character never
    actually placed at Ice Wall Castle in the novel's own scene there,
    same restraint keeping Alhana/Derek/Gunthar off-stage), `SLAY thanoi
    2` (already `TERRAIN_BIAS`-tied to glacier, so the fight genuinely
    happens on the terrain the axe's bonus needs). This milestone's one
    real engine wrinkle: DLA's own weakness for the item (melts useless
    above freezing within a day) has no analog anywhere in this project (no
    "item destroyed by its environment" mechanic exists), simplified
    instead to "only carries its +4 bonus while standing on glacier
    terrain," exactly the deviation the earlier NEXT UP note had already
    proposed. Modeled as a **this-fight-only local bonus inside
    `game::GameLoop::runCombat`**, the identical mechanism already used for
    spell buffs (`playerThac0Bonus`/`playerDamageBonus`) -- the granted
    item itself carries `weaponMagicBonus = 0` (its honest off-glacier
    baseline), so no change was needed to `combat::Combat.h`'s
    `resolvePlayerAttack` signature or any other caller; `runCombat`'s only
    call site (`tryMoveOverworld`) means the fight's own tile is always
    `state_.x`/`state_.y`, recomputed with the same `world::terrainFor`
    call movement already makes. Verified via a throwaway self-test
    (`QuestLoader` parsing the real, now-eleven-quest `data/quests.txt`,
    confirming the new quest's requirement/objective/reward shape and
    `REWARD_FROSTREAVER`'s fail-fast trailing-argument case), a clean
    `/W4` rebuild (zero new warnings), and the piped smoke test (confirms
    `main.cpp`'s cross-validation accepts the new `QUEST K
    frostreaver_salvage` zone binding, and that the user's real
    executable-relative save still loads unchanged -- this milestone made
    no `SaveGame.cpp` changes, re-confirmed by a byte-identical checksum
    before/after). Interactive verification (accepting the quest as a
    Strength-13+ character, killing 2 Thanoi, turning in, equipping the
    Frostreaver, and confirming the +4 applies on a glacier tile but not
    off it) **completed 2026-08-25** via a maxed-stat throwaway Human
    Fighter (all scores 18, hand-patched into the save after a real
    character-creation run -- see below), played by the user from Solace to
    Tarsis to Ice Wall Castle over a real ~20-day in-game journey. Along
    the way this also exercised a quest-engine path never previously
    hit in real play: `monsterKills` is a lifetime tally, so the 2 Thanoi
    kills that happened before the quest was ever accepted still counted,
    and accepting it immediately flipped straight to `ReadyToTurnIn`
    (`GameLoop::checkQuestReadiness`'s "already-satisfied on accept" case) --
    confirmed working exactly as designed. The log line ("Your Frostreaver's
    edge bites keener than steel, sharpened by the glacier's own cold.")
    was confirmed present when fighting on glacier and absent when fighting
    off it, verifying the terrain-gated +4. See `docs/CHARACTER_NOTES.md`'s
    "Magic items", `docs/QUEST_NOTES.md`'s "Shipped quests",
    `docs/ZONE_NOTES.md`'s "Quests: POIs that offer them", and
    `docs/COMBAT_NOTES.md`'s "Player actions".

79. Zone-NPC `SUBJECT` content -- NEXT UP item 6, the last major gap in
    "ask about anything" (Milestone 71's grammar): every generic
    zone-native NPC had zero `SUBJECT` content, only the eight Heroes of
    the Lance did (Milestones 71/72/75/76). The user picked "all zone
    NPCs at once" over a narrower proof-of-concept pass, the same call
    Milestone 76 made for the Heroes. All 21 talkable zone-native NPCs
    across 15 zones (Otik, Tika, the Seeker Guard, the Ruin-Scavenger,
    the Elven Sentinel, the City Watchman, the Forestmaster, all four
    High Clerist's Tower knights, the Silvanesti Warder, the Knight of
    the Watch, Astinus, the Ice Wall Young Knight, the Fortress Guard,
    the Deserting Guard, the Old Sailor, the Knight's Runner, and the
    Plainswoman Rider) gained exactly 2 `SUBJECT` entries and one
    `SUBJECT_UNKNOWN`, generalized from that NPC's own already-
    established `TALK`/`TOPIC`/`SAY_IF`/POI-description text rather than
    fresh invention -- these are original, non-canon NPCs, so no PDF
    research pass was needed. Two POIs with a `TALK` line were
    deliberately excluded: Solace's Notice Board and Pax Tharkas's Ore
    Cart are objects, not people, and free-text "ask about anything"
    doesn't fit either one. Godshome has no talkable NPC at all
    (deliberately sparse, Milestone 45) and stayed out of scope for the
    same reason it's out of scope for `TALK_AFTER`. One real content
    constraint discovered while planning: zone-file `SUBJECT` has no
    day-range gate the way `data/timeline.txt`'s character-level
    `SUBJECT_WHEN` does, so none of this content draws on any NPC's own
    `TALK_AFTER` block -- that material is deliberately gated to fire
    only once a canon-character window has closed, and reusing it in an
    ungated `SUBJECT` would let a player learn retrospective Hero content
    before it's happened in-game; every new subject instead stays scoped
    to what each NPC's un-gated `TALK`/`TOPIC` material already treats as
    always true. Pure data content across 15 `data/zones/*.txt` files --
    zero `.cpp`/`.h` changes, same grammar Milestone 71 shipped. Verified
    via a clean rebuild (zero new warnings; no source files even needed
    recompiling, a pure-data change) and the piped smoke test (confirms
    all 15 edited zone files still parse cleanly, reaching character
    creation with no new `ZoneLoader` warnings in stderr). Real
    in-terminal free-text asking across all 21 NPCs still needs the
    user's own keyboard, same `_getch()` limitation every prior
    `SUBJECT`/picker milestone has flagged. See `docs/ZONE_NOTES.md`'s
    "Ask about anything" section.
80. Widened aftermath dialogue (`TALK_AFTER`) to the last three real zones --
    NEXT UP item 4, closing out the backlog Milestone 66 started and
    Milestones 70/77 widened. `data/zones/pax_tharkas.txt`'s `G` (Fortress
    Guard, `PRESENCE pax_tharkas 10 12`), `data/zones/tarsis.txt`'s `S` (Old
    Sailor, `PRESENCE tarsis 20 22`), and `data/zones/neraka.txt`'s `G`
    (Deserting Guard, `PRESENCE neraka 105 107`) each gained a `TALK_AFTER`
    line, same "reframe an existing NPC's established voice using details
    already in that location's own `PRESENCE` text, epithets not names"
    technique Milestones 66/70/77 all used. The Fortress Guard's line covers
    the fortress's Sla-Mori/chain-room climax (all 8 Heroes, Laurana, and
    Fizban) in his own understated, uninvolved voice; the Old Sailor's line
    ties into his own pre-existing wizard/outsider hostility (`SUBJECT S
    wizards,mages,magic`), giving Raistlin's on-page library hunt the most
    attention since it's the detail his established voice would actually
    fixate on; the Deserting Guard's line covers only the characters
    actually present at Neraka per Milestones 46/49 (Tanis, Caramon,
    Tasslehoff, Laurana, Fizban -- not all eight), filtered through his own
    shaken, too-busy-running-to-ask voice already established in his
    `TALK`/`TOPIC`. Tarsis's other named candidate, the Knight's Runner, was
    deliberately left alone to keep the established "one `TALK_AFTER` per
    zone" pattern (10/10 prior examples) intact rather than doing two at one
    zone. Pure data content -- zero `.cpp`/`.h` changes, same generic,
    zone-agnostic `TALK_AFTER` grammar/parser/`Timeline::latestDayEnd`
    runtime as every prior milestone in this series; no fresh PDF research
    needed, since all three lines draw on `PRESENCE` flavor text already in
    `data/timeline.txt`. Verified via a clean `/W4` rebuild (zero new
    warnings; no source recompilation at all, pure data) and the piped smoke
    test (confirms all three edited zone files still parse cleanly, real
    save moved aside and restored byte-identical afterward); no throwaway
    self-test needed, same call every prior `TALK_AFTER` content pass made.
    Interactive verification (reaching each zone after its `latestDayEnd`,
    confirming each line fires once and falls back to `TALK_AGAIN` after)
    still needs the user's own keyboard, same limitation every prior
    `TALK_AFTER` milestone has flagged. Every real `TALK_AFTER` candidate is
    now done -- Godshome has no talkable zone-native NPC at all and stays
    out of scope permanently. See `docs/ZONE_NOTES.md`'s "Aftermath
    dialogue" section.

81. Widened the "Bob's game" color palette to the remaining plain organic
    screens -- NEXT UP item 4, explicitly flagged "only worth doing if the
    user actually wants it," picked by the user this session via
    `AskUserQuestion`. Milestone 73 colored dialogue/picker/combat and
    deliberately left character sheet, spellbook, shop, inventory, journal,
    help, and the ask-input prompt plain; this milestone converts six of
    those seven (`drawCharacterSheet`/`drawSpellbookFrame`/`drawShopFrame`/
    `drawInventoryFrame`/`drawJournalFrame`/`drawHelpFrame`) from the plain
    `std::vector<std::string>` `writeBoxed` overload to the `BoxLine`
    overload Milestone 73 introduced, using strictly the palette that
    already existed -- no new color, no new meaning invented. A new named
    constant, `kSectionLabelColor` (`"\x1b[96m"`, the same bright cyan
    `buildStatusPanel` already used inline for `MODE:`), formalizes "fixed
    section/category label" as a third reusable meaning and lands on six
    section headers across these screens (`Saving Throws:`, each
    spellbook `Level N (...):`, the shop's `-- Buying --`/`-- Selling --`,
    the inventory's `Carried items:`, the journal's `Completed:`, and
    Help's three category headers). The shop/inventory selected cursor row
    reuses `drawPickerFrame`'s existing bright-white convention directly,
    since both are the same cursor-list shape; journal quest titles reuse
    the bright-yellow "named thing" convention, the clearest analog to an
    NPC/location name outside dialogue. Everything else on these screens
    (ability scores, HP/AC/THAC0, weapon/armor, spell lists, item labels,
    footers, free-form messages) stays plain, matching Milestone 73's own
    "only label sections/named-things/the selection, never free-form
    prose" restraint. `drawAskInputFrame` was deliberately left unchanged
    -- neither of its two lines is purely a section label, a named thing,
    or a selected row, and `BoxLine` still only colors a whole line
    (Milestone 73's unchanged constraint), so tinting just the NPC's name
    inline would mean restructuring the sentence rather than widening the
    existing palette. Verified via a throwaway self-test
    (`ColorSelfTest2.cpp`, same pattern as Milestone 73's own
    `ColorSelfTest.cpp` -- all six changed functions called with sample
    data, output inspected via `cat -v` confirming every color-set code is
    immediately followed by its reset and that bordered rows stay aligned
    column-for-column between colored and plain lines, deleted afterward
    along with its temporary CMake target), a clean `/W4` rebuild (zero new
    warnings), and the piped smoke test (real save moved aside and
    restored byte-identical afterward). Real in-terminal color rendering
    still needs the user's own eyes, same limitation as every prior color
    milestone. See `docs/ARCHITECTURE.md`'s "Widening the palette to the
    remaining organic screens".

82. Widened zone-native NPC `SUBJECT` coverage -- a real gap Milestone 79
    left open, caught during interactive play: the user asked the Palanthas
    Knight of the Watch about the Tower siege and the Golden General --
    material his own `TOPIC "A City Under Watch"` already covers -- and got
    the generic `SUBJECT_UNKNOWN` brush-off, since his `SUBJECT` list only
    covered the harbor and the Shoikan Grove. Investigating confirmed the
    "Ask about something else..." row is correctly gated (never shown for
    an NPC with zero `SUBJECT` entries -- verified directly against
    `GameLoop::talkTo` and every zone file); the real issue was content
    depth, not gating. Asked the user which fix they wanted (widen coverage
    vs. suppress the option below some content floor) and how far to take
    it: widen, and review all 21 Milestone-79 NPCs rather than just
    Palanthas. 11 of the 21 got 1-2 new `SUBJECT` entries each (17 lines
    total) -- darken_wood's Unicorn, haven's Seeker Guard, ice_wall's Young
    Knight, high_clerist_tower's Garrison Knight, neraka's Deserting Guard,
    palanthas's Knight of the Watch and Astinus, kalaman's City Watchman,
    pax_tharkas's Fortress Guard, silvanesti's Warder, and both solace_inn
    NPCs (Otik and Tika) -- every new line paraphrased (not copied
    verbatim) from that same POI's own already-shipped `TOPIC` text, or for
    the Seeker Guard, `TALK_BEFORE`; deliberately never from `TALK_AFTER`,
    which stays untapped on purpose since it's gated to fire only once a
    canon-character window closes and reusing it in an ungated `SUBJECT`
    would leak spoilers (see `docs/ZONE_NOTES.md`'s "Ask about anything").
    The other 10 NPCs were deliberately left alone: High Clerist's Tower's
    Sword/Circle/Rose Knights have no `TOPIC` authored for any of them at
    all -- their terseness ("keeps to himself for now", "hasn't decided
    anything he's willing to say out loud") is a characterization choice,
    not a content gap, and widening them would work against their own
    established voice; Plains of Dust's Rider, Qualinesti's Sentinel, Xak
    Tsaroth's Scavenger, both Tarsis NPCs, and Solace's blacksmith had no
    unaddressed `TOPIC` material to draw from. Pax Tharkas's Ore Cart and
    Solace's Notice Board (objects, not people) correctly still have no
    `SUBJECT`/ask option at all, matching the user's own original request
    exactly. Found and fixed two latent parsing bugs while re-reading every
    `SUBJECT` line in scope: `neraka.txt`'s `temple,dark queen` and
    `xak_tsaroth.txt`'s `ruins,city,xak tsaroth` each had a space inside
    the keyword-list token, which `ZoneLoader`'s `iss >> keywordList`
    (whitespace-delimited) silently truncates -- the word after the space
    was leaking into the *displayed dialogue text* rather than becoming a
    matchable keyword. Both fixed by comma-joining instead
    (`temple,dark,queen` / `ruins,city,xak,tsaroth`). Pure data content,
    zero `.cpp`/`.h` changes -- same grammar, same loader, same runtime as
    Milestone 79. Verified via a throwaway self-test
    (`ZoneLoaderSelfTest.cpp`, a minimal `world::Zone`/`ZoneLoader`/
    `ZoneTile`-only CMake target -- deliberately not linking `GameLoop.cpp`
    just to reach `matchSubject`/`tokenizeAskInput`, which would have
    pulled in character/combat/quest/timeline/render as a dependency for
    two free functions; a local reimplementation of the same
    tokenize-then-exact-token-match logic was used instead) covering all 35
    new/bugfixed keyword lookups across every edited POI plus a
    no-keyword-collision check per POI, all passing before the file and its
    temporary CMake target block were deleted; a clean `/W4` rebuild (zero
    new warnings -- pure data, nothing to recompile); and the piped smoke
    test (confirms all 11 edited zone files still parse cleanly end-to-end
    through the real startup path, real save moved aside and restored
    byte-identical afterward). Interactive verification (actually asking
    these NPCs the new keywords in a real playthrough) still needs the
    user's own keyboard, same limitation every prior `SUBJECT`/`TOPIC`
    content milestone has flagged. See `docs/ZONE_NOTES.md`'s "Ask about
    anything".

83. Town-proximity monster gating and a Thanoi hard terrain lock -- a real
    engine gap the user hit directly in play: high-HD Ogres and Draconians
    turning up right outside Solace, a brand-new character's starting
    town, because `combat::MonsterCatalog::randomMonster`'s Milestone-57
    terrain weighting has no notion of "near civilization" at all -- Ogre
    and all five Draconians carried neither `EXCLUDE_TERRAIN` nor
    `TERRAIN_BIAS`, so they were uniformly eligible on every passable tile
    across the whole 480x320 map. Two small additions to the existing
    data-driven mechanism, not a new subsystem: `MIN_TOWN_DISTANCE <n>`
    (`combat::Monster::minTownDistance`, a hard exclusion below `n` tiles
    straight-line from the nearest `world::Location::isTown`) and
    `ONLY_TERRAIN <codes>` (`onlyTerrain`, the inverse of the existing
    `EXCLUDE_TERRAIN` -- a hard lock to *only* the listed terrain).
    `MonsterCatalog::randomMonster` gained a second parameter,
    `distanceToNearestTown`, computed inline in
    `GameLoop::tryMoveOverworld` right before the encounter roll (the
    destination tile's `state_.x`/`state_.y` are already current at that
    point) against the same `world::Location::isTown` set
    `nearestTown()`'s post-knockout respawn already uses, but as its own
    small inline loop rather than sharing that function (different return
    type, different purpose, one extra call site -- not worth the
    indirection). Applied via `data/monsters.txt` content changes, tuned
    to a rough danger-scaled curve confirmed with the user (Kapak grouped
    with the high-powered tier despite being HD3, since its real
    paralysis-poison bite is disproportionately punishing for a low-level
    character): Ogre and Kapak Draconian at `MIN_TOWN_DISTANCE 20`, Bozak
    at `25`, Sivak at `35`, Aurak at `45`. Thanoi's existing `TERRAIN_BIAS
    :` (Milestone 64) was tightened to `ONLY_TERRAIN :`, a hard glacier
    lock -- unlike everything else in this pass, explicitly *not* claimed
    as book-sourced (Dragonlance Adventures prints no Climate/Terrain
    field for the Thanoi at all), just an honest, user-requested gameplay
    restriction. Left deliberately untouched: Goblin, Kobold, Hobgoblin,
    Timber Wolf, Giant Spider, Baaz Draconian, Bugbear, Gnoll, Ghoul,
    Skeleton, Zombie -- "lower hit die monsters around Solace and other
    cities" (the user's own framing) falls out naturally once the HD4+
    threats and Kapak are excluded near town, without needing a second,
    positive-bias mechanism. Known, accepted limitation: this only keeps
    danger away from *civilian* towns, not toward actual war-front
    locations (High Clerist's Tower, Neraka, Pax Tharkas) -- a
    location/faction-aware placement system is real future-engine
    territory, out of scope for a data-only tuning pass; a fortress that
    happens to sit close to a `TOWN` (the Tower is ~16 tiles from
    Palanthas) gets the same civilian safety bubble as anywhere else near
    a town. Verified via a throwaway self-test (`MonsterSelfTest.cpp`, a
    minimal `Monster`/`MonsterCatalog`/`MonsterLoader`-only CMake target;
    16 assertions against the real `data/monsters.txt` covering the new
    near-town exclusion, far-from-town eligibility, the glacier hard lock,
    and confirming existing `EXCLUDE_TERRAIN`/`TERRAIN_BIAS` behavior --
    Gnoll's salt-flat exclusion, forest bias -- is unchanged; all passing
    before the file and its temporary CMake target were deleted), a clean
    `/W4` rebuild (zero new warnings), and the piped smoke test (real save
    moved aside and restored byte-identical afterward). Interactive
    verification (actually walking near vs. far from Solace and observing
    which monsters turn up) still needs the user's own keyboard, same
    limitation every prior combat/content milestone has flagged. See
    `docs/COMBAT_NOTES.md`'s "Town-proximity monster pools".

84. Zeroed shallow water's encounter chance -- a follow-up bug the user hit
    immediately after Milestone 83, this time on the water: sailing (via
    `GameState::hasBoat`, Milestone 36) was triggering "a lot of monster
    battles in ocean squares." True ocean (`~`) and the Blood Sea (`!`)
    were already 0% in `world::Terrain.cpp`'s `kTable`, explicitly because
    "no sea monsters exist in the monster roster yet" -- but shallow water
    (`r`) still carried a leftover 5%, set at Milestone 27 before boats
    existed. `docs/MAP_NOTES.md` already documents that `r` in the
    generated grid is essentially always coastal water, not real river
    fords (thin river lines never survived the discovery-phase
    downsampling), so a boat route along any coastline crosses a long run
    of `r` tiles, each independently rolling that 5% -- exactly the "land
    monster ambushes you mid-voyage" problem the ocean/Blood Sea 0% was
    already written to prevent, just missed for this one code. Fixed by
    zeroing `r`'s `encounterChancePercent` in `Terrain.cpp`'s `kTable`
    (was 5), bringing all water terrain in line. A single invented-tuning
    constant, not a new mechanism -- no self-test needed (nothing to
    assert beyond what the compiler already checks for a table-literal
    edit); verified via a clean `/W4` rebuild (zero new warnings) and the
    piped smoke test (real save moved aside, hash-confirmed identical
    afterward). See `docs/COMBAT_NOTES.md`'s "Encounters: a per-terrain
    chance while traveling".
85. Higher-fidelity map re-derivation -- the user found a much
    higher-resolution, legibly-labeled Ansalon map
    (`References/dragonlancemap2.png`, replacing a 10125x6750 JPEG whose
    labels were illegible at any practical resolution since Milestone 2)
    and asked to redo the world map from it. `tools/generate_overworld.py`
    was re-pointed at the new image, `NUM_COLORS` raised 16 -> 32 with
    each bucket verified by index-mask inspection rather than judging RGB
    triples by eye, and all 15 locations' `POS` re-derived -- 11 from a
    directly-read label/icon (impossible on the old JPEG for most of
    them), Plains of Dust from its region label, and Darken Wood/Ice Wall
    Castle re-anchored by the same relative-geography method the
    originals used (neither has a direct label on this map either).
    Glacier is recoverable in the classification for the first time,
    letting Milestone 36's 46-tile hand-painted glacier patch be retired
    (Ice Wall Castle's new `POS` lands on real generated glacier); the old
    JPEG's mountain-shadow-into-Blood-Sea misclassification (see the High
    Clerist's Tower section of `docs/MAP_NOTES.md`) also doesn't recur on
    the new map. All 14 `ROAD_PAIRS` re-verified end-to-end reachable via
    a throwaway 4-directional BFS; verified via a clean `/W4` rebuild and
    the piped smoke test (real save backed up, hash-confirmed identical
    afterward). A second user-provided image, an ASCII-art conversion of
    the same map, was investigated and found to be a cosmetic photo-filter
    (character choice follows pixel luminance, not terrain semantics) --
    not usable as data; noted as possible inspiration for a future
    overworld-rendering style, not acted on this milestone. See
    `docs/MAP_NOTES.md`'s "Higher-fidelity map re-derivation" for the full
    writeup, including the before/after `POS` table for every location.
86. Added Thorbardin and Sancrist Isle -- while re-reading labels for
    Milestone 85, two real, book-significant places turned up that
    weren't modeled yet; the user asked to add both. Thorbardin (dwarven
    kingdom under the Kharolis mountains) had no prior decision against
    it -- *Dragons of Winter Night* opens there, the Hammer of Kharas
    ceremony in Thane Hornfel's Great Hall, with Tanis and Sturm on-page
    and Raistlin conjuring an illusory dragonlance immediately after.
    Sancrist Isle had an explicit "deliberately not modeled" call on file
    from Milestone 36 (its trial content already folded into the High
    Clerist's Tower as retrospective dialogue) -- the user was shown that
    exact reasoning and chose to override it anyway. Both got real `POS`
    placements off the new map, `data/zones/` interiors, and
    `PRESENCE`/`SAY`/`TOPIC` content: Thorbardin for all 8 Heroes
    (`13 19`, the gap between Pax Tharkas and Tarsis), Sancrist Isle for
    Sturm/Flint/Tasslehoff/Laurana (`55 60`, between Ice Wall and the
    Tower -- Laurana added beyond the original plan once research
    confirmed she's the witness Sturm actually names, and that her
    already-shipped Tower `TOPIC` was asserting a scene this milestone
    could now actually show). `("pax_tharkas", "thorbardin")` added to
    `ROAD_PAIRS`; Sancrist Isle stays boat-only like Ice Wall. All 16
    roads and both new locations' tiles reverified with a throwaway BFS;
    verified via a clean `/W4` rebuild and the piped smoke test (real
    save backed up, hash-confirmed identical afterward). See
    `docs/MAP_NOTES.md`'s "Thorbardin and Sancrist Isle" and
    `docs/TIMELINE_NOTES.md`'s "Thorbardin"/"Sancrist Isle" sections for
    full sourcing and citations.
87. Fixed roads that crossed open water -- the user spotted, on a
    rendered view of the map, that `ROAD_PAIRS`'s straight-line roads
    don't account for terrain, and a few cut across real bodies of
    water. A throwaway line-walker replayed every pair's path against
    the classified grid, cross-checked against pixel-marked crops of the
    reference map; two of the four flagged pairs turned out to be
    bogus/redundant straight lines across real bays (New Bay, crossed by
    `xak_tsaroth`-`plains_of_dust` for 27 tiles and by `solace`-
    `silvanesti` for 25) and were removed from `ROAD_PAIRS` outright --
    the former was already redundant via `pax_tharkas`, the latter
    matches Silvanesti's own "sealed off" DESC and Ice Wall Castle's
    existing "no road reaches it" precedent. The other two (a handful of
    single-pixel classification-noise tiles on the `darken_wood`-
    `qualinesti` and `solace`-`high_clerist_tower` roads) were patched
    via a new `MANUAL_TERRAIN_OVERRIDES` dict in
    `tools/generate_overworld.py`, since they sat inside terrain already
    classified as fordable shallow water elsewhere. Reviewed
    `References/portcities.txt` per the user's request; concluded no new
    port-city location is needed for this fix specifically (see
    `docs/MAP_NOTES.md`), folding the open "which of these deserve a
    real modeled location" question into NEXT UP below. Re-verified with
    the same line-walk check (0 water crossings left) and a throwaway
    BFS (every location, including the boat-flavor ones, still
    reachable); clean `/W4` rebuild and piped smoke test, real save
    hash-confirmed identical afterward. See `docs/MAP_NOTES.md`'s
    "Fixing roads that crossed open water" for the full pair-by-pair
    writeup.

88. Replaced the global sea-travel flag with a scripted one-time voyage --
    the user, playing their live character (already at Ice Wall Castle,
    `BOAT 1` in their save), noticed that once `GameState::hasBoat` is
    granted it lets the PC cross *any* ocean tile anywhere on the
    continent forever, from any coastline, regardless of where they stood
    -- effectively free-roam sailing. Two problems: it's granted at
    Tarsis, which this project's own data already establishes as
    landlocked ("a city stranded far from any sea"), and the sourced
    material (`dwn_full.txt` lines 5690-5734, 5920-5927) describes one
    specific ship's route -- Tarsis to Ice Wall Castle, continuing past
    Southern Ergoth toward Sancrist Isle -- never general open-world
    sailing. A throwaway BFS against the live `data/overworld.grid`
    (passable terrain only, no boat) confirmed removing `hasBoat` couldn't
    strand the player: Ice Wall Castle and Sancrist Isle are already
    reachable on foot via the coastal shallow-water (`r`) path, matching
    Milestone 87's own finding.

    Replaced `GameState::hasBoat`/`world::TerrainInfo::crossableByBoat`
    with a scripted, talk-triggered location jump: `BOAT <char>` gained a
    payload, `BOAT <char> <destination-location-id> <hours>`. Since this
    carries an id needing cross-file validation, it follows the existing
    `PORTAL`/`QUEST` precedent (a separate `Zone`-level map, a new
    `world::BoatVoyage` struct, validated in `ZoneCatalog::loadForWorld`
    once a `World` exists) rather than `SHOP`/`BED`'s plain-bool shape.
    `TalkCandidate` gained `boatDestinationId`/`boatHours` in place of
    `grantsBoat`; `talkTo`, the first time such a candidate is ever talked
    to, moves the player straight to the destination's `POS`, pops back to
    `Mode::Overworld`, advances `hoursElapsed`, logs one travel line, and
    returns immediately (skipping that POI's topic menu -- the player has
    left the scene). `data/zones/tarsis.txt`'s Knight's Runner now grants
    `BOAT R ice_wall 48` (a ~2-day voyage, invented for pacing like every
    `minutesToCross` value already is). `game::SaveGame` still recognizes
    the old `BOAT <0/1>` line on load and discards it, so a save written
    before this milestone -- including the user's own live character --
    still loads cleanly.

    Scoped to only the Tarsis -> Ice Wall Castle leg, at the user's
    explicit choice: Sancrist Isle relied on the same global `hasBoat` and
    has no scripted voyage of its own yet, so it becomes reachable only by
    the long coastal foot-walk confirmed above -- a disclosed, deliberate
    gap (see `docs/MAP_NOTES.md`'s "Sancrist Isle reachability gap"),
    tracked as a NEXT UP follow-up rather than fixed here. Verified via a
    throwaway self-test (`BoatVoyageSelfTest.cpp`, a minimal `World`/
    `WorldLoader`/`Zone`/`ZoneLoader`/`ZoneCatalog`/`ZoneTile`-only CMake
    target: confirmed the real `tarsis.txt` parses to `ice_wall`/48 hours,
    every malformed `BOAT` variant fails fast with the right message, the
    existing "must have a TALK line" rule still holds, and
    `ZoneCatalog::loadForWorld` -- not `ZoneLoader` alone -- rejects a bad
    destination id; all passing before the file and its temporary target
    were deleted), a clean `/W4` rebuild (zero new warnings), and the
    piped smoke test. The user's real save (moved aside and restored
    per the established procedure) was confirmed to still load cleanly
    with its old `BOAT 1` line, byte-identical before/after since no
    keypress was ever processed. Interactive confirmation of the new
    jump itself (talking to the Runner and landing at Ice Wall Castle)
    still needs the user's own keyboard -- the character who'd trigger it
    has already sailed that leg under the old mechanic, so a fresh
    character is needed to exercise it live. See
    `docs/ARCHITECTURE.md`'s "Sea travel", `docs/ZONE_NOTES.md`'s "Boats:
    POIs that grant a scripted sea voyage", and `docs/MAP_NOTES.md`'s
    "Sancrist Isle reachability gap" for the full writeup.

89. Multi-slot save/load -- the user asked for a real save/load system with
    at least 3 save files, replacing the single hardcoded `save.txt` that
    had been in place since Milestone 1. `game::SaveGame`/`game::GameLoop`
    needed zero changes -- both were already parameterized by an arbitrary
    `path`, so the whole feature lives in `main.cpp` plus a `CMakeLists.txt`
    define rename (`ANSALON_SAVE_FILE` -> `ANSALON_SAVE_FILE_BASE`, a path
    with no number/extension so `<N>.txt` can be appended).

    3 fixed slots (`save1.txt`/`save2.txt`/`save3.txt`), not a configurable
    count -- "at least 3" was the ask, and a generic N-slot system wasn't
    needed for it. A new `describeSlot` helper in `main.cpp` tries to load
    and cross-validate each slot up front (the same "does the saved `ZONE`
    still exist" check `main.cpp` already ran for the single save), and
    -- unlike the old single-save behavior -- **a bad slot no longer aborts
    the whole program**: it's just shown as "(unreadable save: ...)" in
    that slot's menu row, with the other slots still fully usable. The
    slot-picker menu itself reuses the same plain `std::cin`/`std::cout`
    interaction mode `CharacterCreator`/the old y/n prompt already used,
    with its own small local `promptLine`/`promptSlotChoice`/`promptYesNo`
    (mirroring `CharacterCreator.cpp`'s file-local reprompt-on-garbage,
    throw-on-EOF idiom, kept as its own copy per this project's "each file
    owns its own tiny copy" precedent rather than shared across translation
    units). Picking an occupied slot and declining to continue asks a
    second, explicit confirmation before allowing a fresh character to
    overwrite it.

    The user's real, in-progress `save.txt` is migrated to Slot 1
    automatically the first time this build runs and finds no `save1.txt`
    yet (`std::filesystem::rename`, non-fatal on failure -- the old file is
    just left in place and Slot 1 shows empty). This is what carries their
    live Ice Wall Castle character forward with no manual step. Verified
    via piped scripted input against an isolated scratch copy of the built
    exe + `data/` (never the user's real `build\Debug\`, even though the
    migration path is designed to be safe): confirmed the empty-slot menu
    reaches `CharacterCreator`'s first prompt, a valid occupied slot's
    summary renders and "decline continue" -> "decline overwrite" correctly
    loops back to the menu instead of crashing, a deliberately corrupted
    slot (`MODE BOGUS`) shows as unreadable without taking down the other
    slots, and a dropped-in legacy `save.txt` gets migrated to `save1.txt`
    with the notice printed and then loads/plays correctly end to end
    (confirmed rendering the real overworld frame at the migrated
    character's saved position). Also a clean `/W4` rebuild (zero new
    warnings) and a direct diff confirming the user's actual
    `build\Debug\save.txt` was never touched by any of the above. See
    `docs/ARCHITECTURE.md`'s "Save/load" and `docs/GOTCHAS.md`'s "Save/load"
    section for the updated mechanics.

90. Fixed a real softlock at Ice Wall Castle -- reported directly by the
    user: getting knocked out while fighting the Thanoi the
    `frostreaver_salvage` quest requires (hard-locked to glacier terrain
    around Ice Wall since Milestone 83) sent the player to `nearestTown()`'s
    pick, Tarsis, ~56 tiles away by straight-line distance -- but the only
    way back is Tarsis's Knight's Runner `BOAT` voyage, which Milestone 88
    deliberately made fire only the first time that POI is ever talked to
    (replacing a permanent `hasBoat` flag specifically because it let the
    player "just sail around" the continent). So once knocked back to
    Tarsis, Ice Wall Castle became permanently unreachable.

    Confirmed with the user that the one-time `BOAT` restriction itself
    should stay untouched -- the fix instead targets `nearestTown()`'s
    blind spot: it only ever considered `world::Location::isTown` entries,
    with no notion that a non-town location might still be the only safe
    place to wake up. Added a new `SEA_LOCKED` keyword to
    `data/locations.txt` (mirroring `TOWN`'s own loader shape exactly --
    `world::Location::seaLocked`, set by `WorldLoader` on an optional,
    argument-less line), tagged `ice_wall` with it, and widened the
    function's filter to `isTown || seaLocked`. Renamed it
    `nearestRefuge()` to match its broadened meaning (the separate,
    unrelated nearest-town loop in the encounter-spawn code, used only for
    keeping dangerous monsters away from civilian towns, was already its
    own inline computation and stays untouched). Since Ice Wall Castle is
    always the closest landmark to any tile a Thanoi fight can happen on, a
    knockout there now wakes the player back up at Ice Wall Castle itself
    instead of marooning them in Tarsis.

    Verified via a throwaway self-test (`SeaLockedSelfTest.cpp`, a minimal
    `World`/`WorldLoader`-only CMake target: confirmed the real
    `data/locations.txt` parses `ice_wall.seaLocked` as `true` and that it's
    the only location with the flag set; passed, then the file and its
    temporary target were deleted), a clean `/W4` rebuild (zero new
    warnings), and the piped smoke test against an isolated scratch copy
    (the real `build\Debug\save1.txt`/`save2.txt` -- which carry the user's
    actual live character, currently at Ice Wall Castle -- were never
    touched: the smoke test ran against an isolated scratch copy of the
    exe + `data/` only, and the real files' timestamps were confirmed
    unchanged before/after). See
    `docs/MAP_NOTES.md`'s `data/locations.txt` grammar section and
    `docs/COMBAT_NOTES.md`'s "Death: knocked out, not killed" for the
    updated mechanics.

91. Sancrist Isle sea voyage -- the NEXT UP item the user picked to close a
    gap Milestone 88 deliberately left open: converting `GameState::hasBoat`
    into a scripted, point-to-point voyage moved only the Tarsis -> Ice Wall
    Castle leg, leaving Sancrist Isle (which relied on that same removed
    global flag) reachable only by a long coastal foot-walk. A fresh read of
    `.research/dwn_full.txt` (not the existing docs summary alone) found two
    concrete details for the second leg: line 5704, the historical party
    escaped Ice Wall Castle's collapse "with the help of the Ice
    Barbarians" -- the real hook for the new NPC's identity; and lines
    5919-5920, the captain's own line that they'd "make Sancrist in two
    days" if the wind held, an actual sourced duration this time, unlike the
    first leg's invented-for-pacing 48 hours (two days converts to the same
    48-hour figure by coincidence, not by copying it). New POI
    `B "An Ice Barbarian Guide"` in `data/zones/ice_wall.txt`, granting
    `BOAT B sancrist_isle 48` -- deliberately not the zone's existing Young
    Knight (`K`), who's already that zone's `frostreaver_salvage`
    quest-giver and shouldn't also whisk the player away on first talk.
    Placed one tile past the zone's `ENTRY` doorway rather than on it, so
    the existing "You stand at the way back out" arrival message (shown
    only when no POI occupies that tile) stays intact. Pure data content --
    no `.cpp`/`.h` changes, since `BOAT`'s destination-id validation
    (`ZoneCatalog::loadForWorld`) is already fully generic and
    `sancrist_isle` was already a real `LOCATION` (Milestone 86). Verified
    via a throwaway self-test (14 assertions: the new POI parses at its
    intended tile, its `TALK`/`TALK_AGAIN`/`SUBJECT`/`SUBJECT_UNKNOWN`
    lines are all present, its `BoatVoyage` resolves to `sancrist_isle` at
    48 hours, the zone's `ENTRY` tile itself carries no POI, and the
    existing Young Knight is untouched), a clean `/W4` rebuild (zero new
    warnings), and the piped smoke test against an isolated scratch copy
    (the real `build/Debug/save1.txt`/`save2.txt` were never touched).
    Interactive confirmation of the actual jump (talking to the Guide,
    landing at Sancrist Isle, the log line, the clock advance) still needs
    the user's own keyboard -- same limitation every prior `BOAT` milestone
    has flagged. See `docs/ARCHITECTURE.md`'s "Sea travel",
    `docs/ZONE_NOTES.md`'s "Boats" and "Ice Wall Castle" sections,
    `docs/TIMELINE_NOTES.md`'s "Ice Wall"/"Sancrist Isle" sections, and
    `docs/MAP_NOTES.md`'s "Sancrist Isle reachability gap" for the full
    writeup.

92. Boat voyage decline option + Sancrist Isle -> Palanthas leg -- the
    interactive playtest that closed Milestone 91's two open verification
    items (the Milestone 88 Tarsis -> Ice Wall jump and the Milestone 91
    Ice Wall -> Sancrist jump both confirmed working, along with NEXT UP's
    long-open instant-defeat-spell item) also surfaced two real gaps:
    talking to a `BOAT`-granting NPC executed the voyage unconditionally,
    with no way to decline, and Sancrist Isle -- reachable since Milestone
    91 -- had no talkable NPC and no route out at all, a genuine dead end.
    The decline gap turned out to be a pre-existing bug, not just a missing
    nicety: `data/zones/tarsis.txt`'s Knight's Runner was already written
    expecting a choice ("waiting on your answer," a `TALK_AGAIN` line
    about "if you've changed your mind"), and its `SUBJECT` content
    (`derek,knights` / `dragons,ship`) was unreachable dead content because
    the unconditional jump never let execution reach it. Fixed with a new
    `GameState::voyagesTaken` set (persisted as a new `VOYAGED` save line,
    same shape/position as `VISITED`/`MET`), separate from `metCharacters`
    because that set is inserted into on every talk regardless of outcome
    and so can't also gate "have I taken this voyage" without a decline
    permanently forfeiting it. `GameLoop::talkTo` now shows a Board/"Not
    yet" `drawPickerFrame` picker (mirroring `offerOrTurnInQuest`'s
    Accept/Decline picker) gated on `voyagesTaken`; declining falls through
    to the ordinary topics/`SUBJECT` picker instead of ending the
    conversation. Also fixed in the same code block: the travel log line
    hardcoded "carries you **south**," which was already wrong for both
    shipped legs (Tarsis -> Ice Wall is actually southwest, Ice Wall ->
    Sancrist is actually northwest) and would have been wrong again for
    the new leg -- now computed via the existing `compassDirection` helper
    already used by Look. The Sancrist Isle gap closes with a third `BOAT`
    grant, `data/zones/sancrist_isle.txt`'s new `E "An Embarkation
    Officer"`, sourced from *Dragons of Winter Night*'s account of Sturm's
    army mustering at Sancrist to sail for Palanthas
    (`.research/dwn_full.txt` lines 10631-10731 -- the same "made
    third-in-command of the army sailing for Palanthas" scene
    `docs/TIMELINE_NOTES.md`'s own Sancrist Isle section already cited),
    tied to that specific detail rather than an invented generic guard,
    keeping Milestone 86's original "no talkable NPC" restraint intact in
    spirit. No sourced day-count exists for the crossing itself (unlike
    Milestone 91's sourced "two days"), so its `BOAT E palanthas 96` is
    invented-for-pacing, longer than the two 48-hour legs since it's a
    materially longer crossing -- and lands somewhere already useful:
    Palanthas already has a walkable road to `high_clerist_tower`
    (Milestone 44), where the same four Heroes' next `PRESENCE` window
    already sits. Placed one tile east of `ENTRY` (21,14), matching
    Milestone 91's "not on the entry tile itself" rule. Verified via a
    throwaway self-test (23 assertions: the new POI's `TALK`/`TALK_AGAIN`/
    `SUBJECT`/`SUBJECT_UNKNOWN`/`BOAT` lines all present and correct, the
    existing Runner/Guide voyages untouched, a `VOYAGED` `SaveGame`
    round-trip, backward compatibility with a save that has no `VOYAGED`
    line at all, and a malformed-count fail-fast case), a clean `/W4`
    rebuild (zero new warnings), the piped smoke test, and a direct check
    that the user's real `save1.txt`/`save2.txt` (both backed up before
    this session's playtest) still load correctly and describe themselves
    right in the save-slot menu under the new `VOYAGED` keyword --
    timestamps confirmed unchanged throughout. **Known accepted edge
    case**, documented rather than solved: a character who already boarded
    a voyage under the pre-Milestone-92 code has an empty `voyagesTaken`
    for it, so deliberately walking back to that POI would re-offer it;
    harmless (no corruption, just a re-run of an already-real jump) and
    requires walking back across the whole map to trigger. Interactive
    confirmation of the decline flow and the new leg itself still needs
    the user's own keyboard. See `docs/ARCHITECTURE.md`'s "Sea travel",
    `docs/ZONE_NOTES.md`'s "Boats" and "Sancrist Isle" sections,
    `docs/TIMELINE_NOTES.md`'s "Sancrist Isle" section, and
    `docs/MAP_NOTES.md`'s "Sancrist Isle reachability gap" for the full
    writeup.

93. Crossing -- the user spotted `("solace", "high_clerist_tower")`'s road
    crossing open water north of Solace and asked for it to be deleted,
    plus a port city placed there if the sourcing held up. That road
    (Milestone 35) predated this project's `dragonlancemap2.png` source and
    was never actually checked against it; Milestone 87's water-crossing
    audit kept it anyway, on the theory it was the only overland link
    between the Abanasinia/Kharolis cluster and the Solamnia cluster, and
    patched its 3 true-water noise pixels rather than questioning the road
    itself. Cropping and grid-overlaying the real map around the strait
    (same method as every placement since Milestone 85) found no drawn
    road crosses the Strait of Schallsea anywhere -- the map's own roads
    hug each shore and meet a real, clearly labeled ferry town in the
    middle of the strait, **"Crossing."** `("solace",
    "high_clerist_tower")` was removed from `ROAD_PAIRS` outright (the
    map draws no fordable version of it to keep, unlike Milestone 87's two
    patched cases), and a throwaway BFS confirmed removing it strands
    nothing -- the strait's shallow water was already deliberately
    foot-passable without a boat (Milestone 87), so `high_clerist_tower`
    and its whole downstream chain (Kalaman, Palanthas, Godshome, Neraka)
    stay reachable exactly as before, just without a paved-road-over-open-
    sea visual. New `LOCATION crossing` (`POS 200 185`, `REGION
    Abanasinia`, sourced from the map's own label and `TSR 2143 Player's
    Guide`'s framing of Abanasinia as the land south of the strait), a
    small new zone (`data/zones/crossing.txt`, one generic Ferry Keeper
    NPC, no `PRESENCE`/`TIMELINE_ANCHOR` since none of the three sourced
    novels ever mention the place, only the map does) -- same restrained
    "generic NPC, no canon-character content" treatment already given to
    Thorbardin. Resolved with the user before building: Crossing stays a
    plain, road-free, foot-reachable waypoint, not a scripted `BOAT` ferry
    -- the strait doesn't need one for reachability, and adding one would
    pull in return-leg/`voyagesTaken` machinery this crossing has no use
    for. Pure data content -- zero `.cpp`/`.h` changes. Verified via a
    throwaway BFS script (all remaining `ROAD_PAIRS` connections intact,
    Crossing's own tile and every other location still foot-reachable from
    Solace, zero road tiles left crossing the strait), a clean `/W4`
    rebuild (zero new warnings), and the piped smoke test -- run with the
    user's real `save1.txt`/`save2.txt`/`save3.txt` moved aside
    beforehand and restored after, timestamps and content confirmed
    unchanged. See `docs/MAP_NOTES.md`'s "Crossing" section and
    `docs/ZONE_NOTES.md`.

94. Explicit save-slot deletion -- the user found the launch menu had no
    way to delete a save to make room for a new character. Investigation
    found this was half-true: declining "Continue this character?" on an
    occupied slot already offered "Start a new character... overwrite,"
    but that only *implicitly* overwrites the file on the next autosave,
    not an immediate delete (a deliberate Milestone 89 choice, per
    `docs/ARCHITECTURE.md`). Confirmed with the user (`AskUserQuestion`)
    that they wanted a real, explicit, immediate delete, separate from
    character creation. New `game::SaveGame::remove` (a thin, non-throwing
    `std::filesystem::remove` wrapper mirroring the existing `exists()`
    static) plus a `main.cpp`-only change: the save-slot menu now accepts
    `d1`/`d2`/`d3` (`SlotChoice`, extending `promptSlotChoice`) to delete a
    slot immediately after a `y/n` confirmation, reusing `describeSlot` to
    refresh that slot's menu entry afterward. The existing decline/
    overwrite-on-next-save flow is untouched and still the right path for
    "don't care about the old save, just let me play." Verified via a
    clean `/W4` rebuild (zero new warnings) and a piped interactive test
    against an isolated `build/Debug` copy (exe + `data/` + a throwaway
    copy of a real save) -- since the whole save-slot menu, not just
    `CharacterCreator`, runs on plain `std::cin`/`std::cout` and is
    pipeable: confirmed deleting an occupied slot removes the file and
    redraws it as `(empty)`, deleting an already-empty slot shows a
    message with no confirmation prompt, and declining a delete leaves the
    slot untouched -- plus the standard piped smoke test, with the user's
    real `save1.txt`/`save2.txt`/`save3.txt` moved aside first and
    restored after (timestamps confirmed unchanged). See
    `docs/ARCHITECTURE.md`'s "Save/load" section.

95. Southern Ergoth -- closes a real, previously-unexplained gap in Sturm,
    Flint, Tasslehoff, and Laurana's schedule (`ice_wall 38 42` jumped
    straight to `sancrist_isle 55 60`), found while researching NEXT UP's
    item 4 asking whether Nordmaar, Ergoth, or various named ports
    deserved a real `LOCATION`. Nordmaar, Schallsea Island (the New Sea
    one, distinct from the Strait of Schallsea Milestone 93 modeled),
    Caergoth, and New Ports all turned out to be pure map geography, never
    on-page in any of the three Chronicles novels -- correctly left
    unmodeled. Sanction is real and vivid but only ever reported/flashback
    dialogue, never visited on-page by a tracked Hero -- also stays out.
    Southern Ergoth was different: **this project's own existing docs
    contained an error.** `docs/TIMELINE_NOTES.md`'s Ice Wall section
    (Milestone 36) called it "deliberately not modeled... the party never
    lands," citing `.research/dwn_full.txt` lines 5920-5927 -- accurate as
    cited, but the citation stopped mid-scene. Reading ~200 lines further
    shows the white dragon Sleet attacks that exact ship and drives it
    onto Southern Ergoth's rocks for real: Sturm, Flint, Tasslehoff, and
    Laurana (with off-stage Derek Crownguard, Gilthanas, Elistan) are
    captured by Silvanesti refugee elves, nearly fight them in a
    standoff Laurana talks down by revealing the dragon orb, and are
    escorted toward the Wilder Elves' camp, where "Silvan" is unmasked by
    Fizban (already a tracked `CHARACTER` since Milestone 48) as
    **Silvara**, a silver dragon living in disguise -- confirming the
    Whitestone Council is coming "around Famine Time," the same Council
    `sancrist_isle 55 60` already covers. New `LOCATION southern_ergoth`
    (`POS 91 193`, `REGION Southern Ergoth` -- directly labeled on
    `dragonlancemap2.png`, along with "Elderwild Wood" and the
    "Silvamori"/"Qualimori" refugee camps the source text's own three-
    elven-kindreds description matches closely; see `docs/MAP_NOTES.md`),
    `SEA_LOCKED` with no `ROAD_PAIRS` entry, same as Ice Wall/Sancrist
    Isle. New sparse 40x16 zone (a wrecked shore, a talkable Silvanesti
    Sentry, and the Wilder Elves' Camp `TIMELINE_ANCHOR`), matching
    Godshome's "deliberately sparse" precedent rather than a sprawling
    city. `PRESENCE southern_ergoth 43 50` windows added for Sturm, Flint,
    Tasslehoff, and Laurana (between their existing `ice_wall`/
    `sancrist_isle` windows), a new window on Fizban's existing
    `CHARACTER` block (his first appearance chronologically, between
    `pax_tharkas 10 12` and `godshome 103 103`), and a new full
    `CHARACTER silvara` block -- decided with the user before building:
    Silvara gets Alhana Starbreeze's "real, talkable, tracked" treatment
    (Milestone 59), not Kitiara's flavor-only one (Milestone 50), since
    she's central to this scene with a real ongoing arc rather than a
    background antagonist referenced only in retrospect. The sea-travel
    chain gained a link rather than a new destination bolted on
    separately: `data/zones/ice_wall.txt`'s existing "Ice Barbarian Guide"
    (`B`) was repointed from `BOAT B sancrist_isle 48` to `BOAT B
    southern_ergoth 48` -- same POI char, so `GameState::voyagesTaken`'s
    `"ice_wall:B"` key stays correct for any save that already recorded
    the voyage as taken (checked directly against the user's own real
    saves, two of which already had it) -- and a new POI, "A Silvanesti
    Sentry" (`S`), grants the continuation, `BOAT S sancrist_isle 60`.
    Verified via a throwaway self-test (25 assertions: `LOCATION`/zone
    loading, the repointed and new `BOAT` grants, `Timeline::presentAt`
    day-boundary correctness across 42/43/50/51, all six expected
    characters present, Ice Wall/Sancrist Isle's own windows untouched), a
    clean `/W4` rebuild (zero new warnings, no `.cpp`/`.h` changes -- pure
    data content), and the piped smoke test with the user's real
    `save1.txt`/`save2.txt`/`save3.txt` confirmed byte-identical
    afterward. Interactive verification (walking the new leg, talking to
    the new POIs) still needs the user's own keyboard, same limitation
    every prior `BOAT`-touching milestone has flagged. See
    `docs/MAP_NOTES.md`'s "Southern Ergoth" section, `docs/TIMELINE_NOTES.md`'s
    "Southern Ergoth" section (including the correction to this project's
    own prior claim), and `docs/ZONE_NOTES.md`'s "Southern Ergoth" and
    "Boats" sections.

96. Port Balifor and Flotsam -- closes the second real gap the Milestone 95
    research pass turned up, and a deliberate reversal of a "don't invent to
    fill a gap" call this project made three separate times (Milestones 39,
    49, 50): Tanis, Raistlin, Caramon, Goldmoon, and Riverwind's tracked
    schedule used to jump straight from `silvanesti 25 30` to
    `palanthas 83 83`/`kalaman 100 100`, skipping roughly 70 in-game days
    that *Dragons of Winter Night*/*Dragons of Spring Dawning* spend on real,
    sourced content: a month sheltering at Port Balifor (Raistlin's "Red
    Wizard" illusion act funding passage, Goldmoon's healing reputation
    quietly starting to spread), then Flotsam, where Tanis -- disguised in a
    dragonarmy officer's uniform -- is recognized and taken captive by a
    Dragon Highlord who turns out to be Kitiara, while the other four wait
    out his unexplained absences before a storm-night escape opens *Dragons
    of Spring Dawning*. Two new `LOCATION`s (`port_balifor POS 356 179`,
    `flotsam POS 371 152`, both directly legible on the reference map for
    the first time since Kalaman -- see `docs/MAP_NOTES.md`), two new zones
    (a small, three-POI Port Balifor built around William Sweetwater's Pig &
    Whistle; a four-POI Flotsam whose Saltbreeze Inn is deliberately
    flavor-only, no NPC standing in for Kitiara -- her Milestone 50
    off-picker precedent stands -- see `docs/ZONE_NOTES.md`), and matching
    `PRESENCE port_balifor 35 64`/`flotsam 65 82` windows for all five
    Heroes in `data/timeline.txt`, each with a real `SAY`/`SAY_AGAIN`/`TOPIC`
    -- Tanis's own stays deliberately evasive about what's actually
    happening to him, feeding into rather than rewriting his existing
    `TOPIC "A Debt He Won't Name"` at Kalaman. `("neraka", "flotsam")` and
    `("flotsam", "port_balifor")` were added to `ROAD_PAIRS` (Neraka is the
    nearest already-modeled location, not Kalaman, despite Kalaman being
    the location the source text itself names as Flotsam's neighbor -- see
    `docs/MAP_NOTES.md`); regenerating found 5 true-water tiles across the
    two new roads, all patched via `MANUAL_TERRAIN_OVERRIDES` as short fords
    or single-pixel noise, re-verified at 0 after regenerating. Confirmed,
    rather than assumed, that Ice Wall's old 46-tile hand-painted glacier
    patch needs no reapplication here -- that caveat was already retired at
    Milestone 85. What's explicitly left out: the Silvanesti-to-Port-Balifor
    journey and the Blood Sea maelstrom/shipwreck itself (neither is shown
    on-page), Kitiara as a talkable `CHARACTER` (Milestone 50's precedent),
    and Laurana's own later Flotsam/Dargaard Keep captivity (a real,
    still-open gap, not resolved by this milestone -- see
    `docs/TIMELINE_NOTES.md`'s Laurana section). Verified via a throwaway
    self-test (49 assertions: `LOCATION`/zone loading, both zones' POIs and
    `TIMELINE_ANCHOR`s, `Timeline::presentAt` day-boundary correctness for
    all five Heroes across both new windows, and confirming the existing
    `silvanesti`/`kalaman`/`palanthas` windows are untouched), a clean `/W4`
    rebuild (zero new warnings, no `.cpp`/`.h` changes -- pure data content),
    and the piped smoke test with the user's real `save1.txt`/`save2.txt`/
    `save3.txt` confirmed byte-identical afterward. Interactive verification
    (walking the new roads, talking to William Sweetwater and the Flotsam
    dockhand, confirming the new `PRESENCE` dialogue in sequence) still
    needs the user's own keyboard. See `docs/MAP_NOTES.md`, `docs/
    ZONE_NOTES.md`, and `docs/TIMELINE_NOTES.md`'s "Port Balifor and
    Flotsam" sections.

97. Interactive confirmation of Milestone 92's boat flow, plus Embarkation
    Officer topic depth -- closes NEXT UP's long-open item 1, the one piece
    of Milestone 92 that had only ever been structurally verified (self-test,
    rebuild, piped smoke test) rather than actually played. Walked the full
    chain -- Tarsis's Knight's Runner, Ice Wall's Ice Barbarian Guide,
    Southern Ergoth's Silvanesti Sentry, and Sancrist Isle's Embarkation
    Officer -- on the user's real save slot 2 ("Mason"), confirming at each
    stop that declining falls through to that POI's topics/`SUBJECT`
    content instead of ending the conversation, and that accepting still
    boards correctly; at the final Sancrist Isle -> Palanthas leg
    specifically, confirmed the travel log correctly reads "northeast" (not
    the old hardcoded "south" Milestone 92 fixed) and the clock advances the
    full 96 hours, landing at Palanthas. Along the way, the user flagged
    `data/zones/sancrist_isle.txt`'s `E "An Embarkation Officer"` as feeling
    thin in practice: his own dialogue names seven proper nouns (Derek,
    Alfred, Brightblade, Sturm, Palanthas, Tower, army) but only 2 authored
    `SUBJECT` topics existed to ask about (3 distinct replies counting the
    `SUBJECT_UNKNOWN` fallback) -- functioning exactly as designed (an
    identical 2-topic pattern to Tarsis's Knight Runner, from the same
    milestone) but reading as unusually compressed once actually played,
    especially collapsing three distinct named knights into one shared
    reply. Split `derek,alfred,brightblade,sturm` into three separate
    topics -- `derek,crownguard,rose`, `alfred,markenin,sword`,
    `brightblade,sturm,crown` -- each re-sourced from the same
    `.research/dwn_full.txt` trial/muster passage (lines ~10600-10740)
    Milestone 92 already cited for this POI: Derek Crownguard as High
    Commander for the Order of the Rose who stormed out over the Council's
    verdict; Alfred MarKenin commanding for the Order of the Sword, angrier
    at the trial than he let on to Sturm's face; Sturm Brightblade himself
    as the odd man out, made third-in-command over half the Council's
    objections. Not new research -- a closer read of material already on
    file for this exact POI. `palanthas,tower,army` was left untouched.
    Verified via a clean rebuild (zero new warnings, no `.cpp`/`.h`
    changes -- pure data content), the piped smoke test against an isolated
    scratch copy (confirms the zone file's fail-fast loader accepts the new
    lines without touching the user's real saves), and, for the first time
    on this exact content, real interactive confirmation via the user's own
    keyboard -- the three new topics each returning distinct replies. Real
    `save1.txt`/`save2.txt`/`save3.txt` were backed up before the session
    began. See `docs/ZONE_NOTES.md`'s "Sancrist Isle" section and
    `docs/MILESTONES.md` entry 92.

98. Dargaard Keep -- closes the real, still-open gap Milestone 96 left
    behind: Laurana is captured between her `kalaman 90 92` and
    `neraka 105 107` windows, but nothing staged it, only referenced it
    obliquely afterward. A fresh, direct re-read of `.research/
    dosd_full.txt` (not the existing doc summary, which turned out to
    conflate this scene with Flotsam -- corrected in the same session)
    confirmed a real, three-Hero, on-page scene: a forged letter lures
    Laurana, Flint, and Tasslehoff out of Kalaman the night after the
    festival with a false claim that Tanis is dying at Dargaard Keep;
    Bakaris, the dragonarmy officer freed as part of the trade, turns on
    them in a forest clearing short of the keep itself; Tasslehoff kills
    him, and an ancient, spectral Knight of Solamnia -- confirmed by
    cross-reference to be Lord Soth, but never named to Flint or Tas
    on-page -- paralyzes them both and carries Laurana off toward Neraka,
    already modeled. New `LOCATION dargaard_keep` (`POS 258 74`, `REGION
    Estwilde`, placed by cropping the reference map around the same
    region Milestone 39 used for Kalaman, then confirmed against the live
    `data/overworld.grid`'s own small mountain cluster southwest of
    Kalaman's real, current tile rather than trusted from the image crop
    alone -- a real discrepancy between the two was found and documented,
    see `docs/MAP_NOTES.md`) and a new, sparse, walled-free zone
    (`data/zones/dargaard_keep.txt`: a forest clearing, a cave mouth, and
    the keep itself visible only as a distant silhouette -- no interior,
    since no tracked Hero is ever shown conscious inside the keep anywhere
    in the source text). `("kalaman", "dargaard_keep")` added to
    `ROAD_PAIRS`; regenerating produced exactly 6 diff tiles (the new
    road plus its destination), no true-water crossings, no glacier-patch
    reapplication needed (confirmed still retired since Milestone 85).
    `PRESENCE dargaard_keep 93 93` added for Flint and Tasslehoff only,
    each with a real `SAY`/`SAY_AGAIN`/`TOPIC` distinct from Flint's
    already-shipped `kalaman 100 100` retrospective regret -- Bakaris is
    named directly (completing a naming this project had already
    half-committed to via Laurana's own existing dialogue), the spectral
    knight stays unnamed to them (they never learn who it was on-page, so
    naming him in their mouths would invent knowledge the source doesn't
    give them, even though the blanket "keep major characters unnamed"
    policy itself was retired at Milestone 49). **No new `PRESENCE`
    window for Laurana** -- her absence is now confirmed rather than
    deferred: Dargaard Keep's own interior has no tracked-Hero-witnessed
    content at all, and a no-`SAY` window would break this project's own
    convention that device is reserved for a *permanent* schedule ending
    (Sturm, Raistlin, Flint), which hers is not. Verified via a throwaway
    self-test (34 assertions: `LOCATION`/zone loading, day-92/93/94
    boundary correctness for Flint and Tasslehoff, and confirming
    Laurana's own schedule -- including `neraka 105 107` -- is untouched),
    a clean `/W4` rebuild (zero new warnings, no `.cpp`/`.h` changes --
    pure data content), and the piped smoke test. No live save currently
    exists in `build/Debug` to risk, so no save-preservation step was
    needed this session; the one stale `save.txt` at the repo root (not
    on the executable's own resolved data path) was confirmed
    byte-identical before and after regardless. Interactive verification
    (walking the new road, finding Flint and Tasslehoff at the new
    location) still needs the user's own keyboard, same limitation every
    prior milestone has flagged. See `docs/MAP_NOTES.md`, `docs/
    ZONE_NOTES.md`, and `docs/TIMELINE_NOTES.md`'s "Dargaard Keep"
    sections, plus the correction folded into `docs/TIMELINE_NOTES.md`'s
    existing Laurana section.
99. Real mechanics for Bozak/Sivak/Aurak Draconians -- NEXT UP item 2, open
    since Milestone 64 first added the three as flavor-only roster entries.
    The full book ability set spans six distinct subsystems (spellcasting,
    shapeshifting, mind control, dimension door, a breath weapon, plus each
    creature's own magic resistance/save bonuses); several of those have no
    clean translation into this project's positionless, single-player-vs-
    single-monster combat loop (no monster-instance persistence to hang
    disguise-based shapeshifting on, no targeting/positioning for mind
    control or dimension door), so the user scoped this to the parts that
    ground out in real, sourced numbers, via `AskUserQuestion`. Sourcing was
    re-verified directly against rendered page images of *Dragonlance
    Adventures* pp.73-75 (not reused from the existing doc summary) -- no
    errata found this time, everything already documented held up. Three
    new `combat::Monster` fields and matching `MonsterLoader` keywords,
    each a dedicated flag/percent pair following the existing `poisonOnHit`
    precedent rather than a general monster-ability subsystem (there is
    exactly one monster of each kind): **Bozak** (`CASTS_MAGIC_MISSILE 40`)
    casts Magic Missile 40% of rounds instead of its weapon attack, reusing
    the player's own real PHB p.176 math (`character::castSpell`'s
    `magic_missile` case) fixed at "4th-level caster" -- no attack roll, no
    save; **Aurak** (`BREATH_WEAPON 30`) breathes its noxious cloud 30% of
    rounds instead, rolling the previously-dormant-in-combat
    `character::SaveCategory::BreathWeapon` for half of the book's 20
    damage, or full damage plus a blinded -4 this-fight to-hit penalty (the
    book names blinded but gives no number, so that value is invented,
    flagged as such); **Sivak** (`BURSTS_INTO_FLAME`) now deals a real 2d4
    retaliatory hit on death instead of a flavor-only victory message like
    Baaz's stone/Kapak's acid/Bozak's own bone-explosion -- the book's
    "killed by something larger than itself" trigger condition is dropped
    (no SIZE stat exists to check it against), so it always fires. Both
    monster-side percent chances are invented pacing, called out as such --
    the book gives no real-time frequency for any of this, and Aurak's
    actual "three times per day" has no way to track across stateless
    encounters with no monster-instance persistence, so it's compressed to
    "available this whole fight." The Sivak burst can knock the player out
    *after* they already landed the killing blow (they still keep the
    XP/steel), a new edge case handled by factoring `GameLoop::runCombat`'s
    existing knockout ending into a shared `knockedOutBy(cause)` lambda
    called from both its original site and the new post-victory check --
    the only structural change in this milestone, and a real second call
    site rather than speculative abstraction. Left deliberately unmodeled,
    same restraint as ever: all three creatures' magic resistance and save
    bonuses; Sivak's shapeshifting itself (no per-instance monster identity
    or NPC-disguise gameplay to hang it on); Aurak's dimension door, mind
    control, change self/polymorph self, at-will invisibility, full spell
    list, and three-stage death sequence. Verified via a throwaway
    self-test (synthetic single-monster data files confirming each new
    keyword parses onto the right field, fail-fast cases for
    `CASTS_MAGIC_MISSILE`/`BREATH_WEAPON` missing their percent argument,
    and the real `data/monsters.txt` still loading its full 17-monster
    roster), a clean `/W4` rebuild (zero new warnings), and the piped smoke
    test. **Interactive verification** was completed in a follow-up
    session via an isolated throwaway copy of the exe and `data/`
    (real save backed up defensively first, though never actually at
    risk): `data/monsters.txt` was temporarily narrowed to one draconian
    at a time (and, since Sivak didn't turn up by chance even with a
    1-in-3 pool, `world::Terrain.cpp`'s per-terrain encounter percentages
    were temporarily bumped to 100 for one more throwaway rebuild, then
    reverted) to force each encounter quickly rather than waiting on the
    real, rare random-encounter odds. All three fired correctly in a real
    playthrough: Bozak's Magic Missile line, Aurak's breath weapon
    save/no-save lines, and the Sivak burst-then-knockout edge case
    (XP/steel still awarded even when the post-kill burst finishes the
    player off). See `docs/COMBAT_NOTES.md`'s "Draconian roster" and
    "Death: knocked out, not killed" sections.

100. Three more monsters -- Owlbear, Wight, Troll -- bringing the roster to
    20, picked from `docs/COMBAT_NOTES.md`'s "Extending this later" backlog
    at the user's request after Milestone 99 closed out the Draconian-
    abilities NEXT UP item and left the backlog otherwise exhausted. All
    three are generic `Monster Manual (2nd ed).pdf` entries, same precedent
    as the existing Bugbear/Ogre/Gnoll/Ghoul/Skeleton/Zombie (this project
    models Krynn specifically, which has no orcs, but draws non-Krynn-
    specific creatures from the Monster Manual same as ever), visually
    confirmed against rendered page images (`pdftoppm`, this session had it
    available, unlike Milestone 34's text-only cross-check). **Owlbear**
    (p.284, HD5+2): a forest apex predator, its three-hit claw/claw/beak
    simplified to a single representative beak hit (2d6), same "one
    representative die" treatment as the Ghoul's/Sivak's own multi-attack
    simplifications; its real grapple-and-squeeze "hug" special attack
    stays unmodeled (no ongoing-effect state exists for anyone yet).
    **Wight** (p.360, HD4+3): the roster's first non-Draconian undead above
    Ghoul/Skeleton/Zombie tier; its real level-drain touch and "hit only by
    silver or +1-or-better magical weapons" defense both stay unmodeled,
    same restraint as every other special attack/defense in this roster --
    the flat XP 1,400 the book gives it (not a per-hp formula) reflects how
    disproportionately valuable real level drain is, even though this
    project's own Wight is mechanically just a plain 1d4 hit. **Troll**
    (p.349, base "Troll" column only -- not the six other variants sharing
    that page): the roster's new apex tier, HD6+6 with a simplified single
    representative bite (1d8+4); its real regeneration (3 hp/round, stopped
    only by fire/acid) stays unmodeled, since no per-round monster HP
    recovery exists in `runCombat` -- also lands on a flat XP 1,400, same as
    Wight. All three follow the existing `MIN_TOWN_DISTANCE` danger-curve
    convention from Milestone 83 (Owlbear 20, matching Ogre's tier; Wight
    25, matching Bozak's; Troll 35, matching Sivak's HD6 tier) rather than
    inventing a new one. Pure data addition to `data/monsters.txt`, no
    `.cpp`/`.h` changes -- `combat::MonsterLoader` already parses every
    keyword these three need. Verified via a clean `/W4` rebuild (zero new
    warnings) and the piped save-slot-menu smoke test (confirms
    `MonsterCatalog` parses the file cleanly end-to-end, including all
    three new blocks); no throwaway self-test needed, same reasoning as
    every prior pure-content monster milestone. See `docs/COMBAT_NOTES.md`'s
    "Accuracy: what's sourced, what's invented" and "Extending this later"
    sections.

101. Per-location shop wares -- closes a scope cut documented since
    Milestone 28 ("there's no per-location wares"), at the user's
    explicit request that every town carry a distinct shop with real
    class-relevant weapons/armor. `character::ShopItem` gained a
    `ShopItemKind` discriminant and `purchaseItem` now dispatches
    directly on it instead of the old fragile `armorCount + 1`-style
    offset math; a new `character::ShopCatalog` (`General`/`Armory`/
    `MarketGoods`/`Salvage`/`Bazaar`/`HarborTrade`) filters which item
    kinds `availableShopItems` returns, via a small `catalogDef` lookup
    table in `Equipment.cpp`. Six shops now exist across five towns, each
    catalog picked from that POI's own already-written flavor text rather
    than an arbitrary assignment: Solace's General Store (`general`,
    unchanged baseline) and a new second shop, Flint's Smithy (`armory`
    -- all armor/weapon upgrades, no potions/arcane items); Haven's
    Market Stalls (`market` -- Leather + shield + potion only); Tarsis's
    Old Sailor (`salvage` -- potion + magic weapon only, extending the
    existing "scavenged relic" framing); two new shops at towns that
    previously had none, Kalaman's Market Square (`bazaar`) and
    Palanthas's Harbor (`harbor`). `world::PointOfInterest` gained a
    plain `shopCatalog` string (validated by `ZoneLoader` against the six
    known names, but never referencing `character::ShopCatalog` itself --
    `world::` stays decoupled from `character::`, per
    `docs/ARCHITECTURE.md` -- `game::GameLoop::handleShop` does the
    string->enum translation). Flint's Smithy is also this project's
    first quest-gated shop: a new zone-grammar line, `SHOP_LOCKED <char>
    <quest-id>` (modeled directly on `QUEST`, cross-validated against
    `quest::QuestCatalog` in `main.cpp` the same way), keeps it closed
    until `ore_for_the_forge` (Milestone-era `DELIVER` quest) is turned
    in -- a real payoff for already-shipped content instead of an
    invented mechanic. Deliberately left out: Crossing, Port Balifor, and
    Flotsam, three other `TOWN`-flagged locations with no plausible
    friendly-merchant POI (a ferry waypoint, a draconian-guarded harbor,
    and a smugglers' haven, respectively) -- same restraint-over-
    completeness discipline as ever, flagged to the user during planning
    rather than silently cut. Webnet/Brooch of Imog are now
    General-Store-exclusive, a real behavior change from Milestone 56
    (previously sold everywhere) called out explicitly in docs. Verified
    via a throwaway self-test (all 6 catalogs' item filtering, the new
    `kind`-based `purchaseItem` dispatch across several catalogs,
    `ZoneLoader` parsing `SHOP <char> <catalog>`/`SHOP_LOCKED` plus their
    fail-fast cases), a clean `/W4` rebuild, and the piped smoke test
    (confirms all 5 edited zone files and `main.cpp`'s new `SHOP_LOCKED`
    cross-validation loop parse the real data end-to-end). **Interactive
    verification still needed** (same limitation as every prior
    quest/shop-UI milestone): confirming Flint's Smithy is locked before
    `ore_for_the_forge` and opens after turn-in, and that each of the 6
    shops shows its intended catalog in `drawShopFrame`. See
    `docs/ZONE_NOTES.md`'s "Shops"/"SHOP_LOCKED", `docs/CHARACTER_NOTES.md`'s
    "Six shops, six catalogs", and `docs/QUEST_NOTES.md`'s
    `ore_for_the_forge` entry.

102. More shop items, quest items, and weapons/armor for every class -- a
    user-requested content pass, picked once the NEXT UP backlog below was
    otherwise exhausted. Research first (rendered PHB/DLA page images, same
    discipline as every prior equipment milestone) found DLA's "Magical
    Items of Krynn" chapter (pp.91-99) fully mined -- everything left is
    either a unique artifact permanently owned by a named canon character
    or needs an unbuilt subsystem, now stated explicitly in
    `docs/QUEST_NOTES.md` so a future session doesn't re-open that chapter
    expecting to find something. The real gap turned out to be the PHB's
    own Weapons/Armor tables, only partly drawn from originally. Two new
    armor tiers (`character::ArmorId`, appended after `SolamnicArmor`,
    append-only-safe): Studded Leather (AC7, 20stl, a budget mid-tier) and
    Plate Mail (AC3, 600stl, the next real tier above Splint Mail, priced
    for banked quest/kill rewards rather than starting steel) -- each shop's
    `ShopCatalogDef` picks up the new tiers per its already-established
    character (General/Armory/Harbor get both; Market/Bazaar get Studded
    Leather only; Salvage stays armorless). Mage and Tinker's own long-
    documented "no mundane weapon upgrade" gap is closed:
    `character::weaponUpgradeFor` used to return `nullptr` for both --
    Mage now gets a Quarterstaff (1d6, priced nominally at 2stl since the
    PHB lists no real cost for a cut length of wood) and Tinker a Light
    Crossbow (1d4+1, reusing the Light Quarrel's damage since this engine
    doesn't track ammunition separately, a mechanical weapon fitting the
    class's gadgeteer identity). Fighter/Cleric/Thief keep their single
    existing upgrade unchanged -- a deliberate scoping choice, since giving
    every class a *second* tier would need `WeaponUpgrade` restructured
    into a per-class list rather than just new data. A second `DELIVER`
    quest, `seed_for_thorbardin`, proves the mechanism isn't a one-off
    after `ore_for_the_forge`: a zone-file scan for quest-less `TALK` NPCs
    found exactly one strong, unforced item hook left -- Thorbardin's
    Refugee Quarter, whose existing dialogue ("come spring we're meant to
    try the mountainside for crops") already wrote the quest for itself.
    Granted at a new POI in `data/zones/haven.txt` (`F`, "A Farmer's Cart",
    reframing the Seeker Guard's own "farmers wanting rain blessed" line),
    turned in at Thorbardin's existing Refugee Quarter POI with no new
    dialogue needed there. `SaveGame.cpp`'s two `ArmorId` bound checks moved
    5->7, append-only-safe (confirmed directly against the user's real
    save, which still loads unchanged). Verified via a throwaway self-test
    (44 assertions covering the new armor tiers' AC/cost/resale, both
    classes' new `WeaponUpgrade`, per-catalog armor-tier filtering,
    `QuestLoader` parsing the new quest, `ZoneLoader` parsing both edited
    zone files, and a `SaveGame` round-trip through the widened `ArmorId`
    bound), a clean `/W4` rebuild (zero new warnings), the piped smoke
    test, and a direct check that the user's real save loads byte-for-byte
    unchanged. Interactive verification (buying the new armor tiers at the
    right shops, equipping the Mage/Tinker upgrades, completing
    `seed_for_thorbardin`) still needs the user's own keyboard, the same
    `_getch()` limitation flagged for every prior milestone. See
    `docs/CHARACTER_NOTES.md`'s "Equipment"/"Six shops, six catalogs" and
    `docs/QUEST_NOTES.md`'s "A second DELIVER quest".

103. Three distinct Tests of High Sorcery -- a user-requested follow-up to
    the existing level-3 Robe-assignment flavor moment (Milestone-era, see
    "Wizards of High Sorcery" in `docs/CHARACTER_NOTES.md`), which until now
    gave every Mage the exact same generic line regardless of which Robe
    they were assigned. Research first (rendered page images of DLA
    pp.33-37, since this section's text is column-garbled in `pdftotext` and
    unreadable without rendering) found the book deliberately gives no
    single canonical Test to transcribe -- each initiate's is individually
    designed around their own weaknesses, and failure means death -- only
    design guidelines a DM builds a Test from: at least three trials
    unsolvable by magic alone, a combat against a known ally, a solo combat
    against a stronger-than-usual opponent, casting every spell the
    initiate knows. Three new passages (`character::applyPendingLevelUps`,
    `Leveling.cpp`, a new `switch` on the already-computed `RobeColor`) each
    freshly dramatize one of those named elements rather than inventing
    unrelated flavor: White reframes "unsolvable by magic" as refusing to
    spend a trusted illusion for personal power; Red dramatizes the Robe's
    own defining "balance" identity (p.36, "the widest range of spells
    available") as every trial resolving into a mercy-vs-cruelty choice and
    refusing both; Black reframes the "combat against an ally" guideline as
    choosing yourself over a friend, with the Conclave marking that choice,
    not the spell, as the pass condition. All three still reuse the
    existing `robeColorName`/`robeMoonName` helpers (`WizardOrder.cpp`) for
    the "you emerge a ___, sworn to ___" clause rather than hardcoding
    Robe/moon names. Pure flavor-text change -- no new fields, no
    save-format changes, no header changes. Verified via a clean `/W4`
    rebuild (zero new warnings) and the piped smoke test (confirms nothing
    broke on load; the user's real save, a level 4 Human Fighter, loaded
    untouched); no throwaway self-test needed, since the new logic is a
    straight switch over an already-tested enum with no new state.
    **Interactive verification still needed** -- a Mage actually reaching
    level 3 under each of the three alignment groups to see all three new
    passages -- same `_getch()` limitation as every prior UI-reachable
    milestone. See `docs/CHARACTER_NOTES.md`'s "Wizards of High Sorcery".

104. The "ask about anything" screen shows its own keywords instead of
    making the player guess blind. `render::MapRenderer::drawAskInputFrame`
    gained a `hints` parameter -- a "You could ask about: ..." line built in
    `game::GameLoop::talkTo` from every `Speech::SubjectEntry` in scope
    (`keywords.front()`, capitalized for display; the data stays lowercase
    since that's what `matchSubject` compares against). A fully clickable
    keyword menu (each `SUBJECT` as its own `drawPickerFrame` row, like
    `TOPIC`) was considered first and rejected: `drawPickerFrame`/
    `writeBoxed` have no scrolling, and Raistlin's character-level subject
    pool alone runs 20+ entries -- a menu that long would just render an
    unusably tall, unscrollable box. The hint line sidesteps that: it's one
    line, wrapped for free by `writeBoxed`'s existing prose-wrap pipeline,
    same as any other long line on an "organic" screen. `matchSubject`/
    `tokenizeAskInput`/`SUBJECT_UNKNOWN`/`Console::readLine` and every
    `data/*.txt` file are completely unchanged -- this is a presentation-only
    addition confirmed with the user (`AskUserQuestion`) before building,
    given the picker-overflow risk the full-menu alternative carried.
    Verified via a throwaway `AskHintSelfTest.cpp` (piped to a file, `cat -v`
    inspected -- confirmed the hint line renders, wraps correctly inside the
    box, and that the no-`SUBJECT` case looks byte-for-byte identical to
    before), a clean `/W4` rebuild (zero new warnings), and the piped smoke
    test (`echo "1" | ansalon_rpg.exe` -- the empty-`echo ""` form this
    project's docs previously called out no longer reaches character
    creation on its own now that Milestone 89 added the save-slot menu in
    front of it; a slot number must be piped first). **Interactive
    verification still needed** -- actually talking to a large-subject-pool
    character (Raistlin) and confirming the hint line reads well in a real
    conversation, same `_getch()` limitation as every prior UI-reachable
    milestone. See `docs/TIMELINE_NOTES.md`'s "Ask about anything" section.

105. Three more quests -- a user-requested "more quests and things to do"
    content pass, using the same "unforced hook" method every prior
    quest-content milestone used: every `TALK`-having POI across all 23
    zone files was checked for a missing `QUEST` line, then cross-referenced
    against that POI's own already-written `TALK`/`TOPIC`/`SUBJECT` flavor
    text rather than inventing new lore. Most candidates (Crossing,
    Palanthas's Knight/Astinus, Sancrist's Embarkation Officer, Flotsam,
    Port Balifor, Tarsis's Sailor/Runner, Pax Tharkas's Fortress Guard)
    turned out too thin to ground without real invention; three held up.
    `what_the_stones_remember` (Darken Wood's Unicorn/Forestmaster, `SLAY
    owlbear 1`) reframes her established "judge what a life gives versus
    takes" characterization and pays off the zone's Old Ruins POI's own
    unexplained "kept back" ground. `word_to_the_wilder_kin` (Southern
    Ergoth's Silvanesti Sentry, `TALK southern_ergoth:K`) turns the
    Sentry's own admitted Kaganesti/Silvanesti distance into an errand --
    and since the obvious destination POI, the Wilder Elves' Camp, is this
    zone's `TIMELINE_ANCHOR` (which this project keeps pure scenery), a new
    POI (`K`, "A Kaganesti Lookout") was split off it instead, the same
    `data/zones/solace_inn.txt` `O`/`Y`-off-`K`-the-Bar treatment already
    established; this is also the first POI in the project combining
    `QUEST` and `BOAT` on one tile, which needed no code changes since
    `GameLoop::talkTo` already runs both offers as independent sequential
    checks. `new_faces_on_the_road` (Haven's Seeker Guard, `SLAY gnoll 3`)
    pays off his own already-written "the new faces aren't pilgrims" line
    and gives Haven the "clear the roads" quest every other major town
    already had, using a previously quest-unused monster (Gnoll) instead of
    a fourth Wolf/Goblin/Hobgoblin reskin. All three are pure data content
    -- no new `REQUIRE`, no new reward flag, no `.cpp`/`.h` changes, no
    save-format changes. Verified via a throwaway self-test (`QuestLoader`
    against the real, now-15-quest `data/quests.txt`; `ZoneLoader` parsing
    all three edited zone files, confirming each new `QUEST <char>
    <quest-id>` binding and the new Kaganesti Lookout POI's grammar), a
    clean `/W4` rebuild (zero new warnings), and the piped smoke test
    (confirms `main.cpp`'s cross-validation accepts all three new zone
    bindings). **Interactive verification still needed** -- accepting/
    completing all three quests, and confirming the Sentry's quest-then-
    boat sequence reads naturally -- same `_getch()` limitation as every
    prior quest milestone. See `docs/QUEST_NOTES.md`'s "Shipped quests" and
    `docs/ZONE_NOTES.md`'s "Southern Ergoth" section.

106. `reason_worth_giving` -- a new quest at the Plains of Dust, found by
    digging past the Milestone 105 sweep's own blind spot at the user's
    request. That sweep's checked-and-rejected list never actually named
    Qualinesti's Elven Sentinel, Neraka's Deserting Guard, or Plains of
    Dust's Rider; re-checked directly this session, the first two genuinely
    don't hold up (pure gatekeeping, pure desertion -- neither shaped like
    an errand) but the Rider does. Her own already-shipped `TALK`/
    `TALK_AGAIN` lines (`data/zones/plains_of_dust.txt`) carry an explicit,
    unresolved "prove yourself, and you'll be welcome" arc this quest
    finally pays off. Plains of Dust is this project's one deliberately
    *invented* zone (an original Plainsfolk tribe stands in for the real
    Que-Shu, which DL3 confirms is already destroyed), so unlike every
    other zone's quest content, this needed no novel citation -- the whole
    zone already runs on invented-but-flagged tone. The shape: the tribe's
    burial mounds have been disturbed, and putting the dead back to rest is
    the "reason" her dialogue already gestures at, using Ghoul
    (`data/monsters.txt`) -- the one Monster-Manual-sourced monster no
    quest had used yet, whose own `DESC` ("rises from a shallow grave") is
    a direct fit for disturbed-mound flavor -- rather than a fourth Wolf/
    Goblin/Hobgoblin reskin. `QUEST R reason_worth_giving` binds to the
    Rider's existing `TALK` line (no new `TALK` needed); a new flavor-only
    POI, `M "The Old Mounds"`, gives the quest a physical anchor, the same
    role Darken Wood's pre-existing Old Ruins played for
    `what_the_stones_remember`; a new `SUBJECT R mounds,graves,dead,barrows`
    entry ties the Rider's "ask about anything" pool to the new thread. No
    `REQUIRE`, `SLAY ghoul 2`, 40 steel / 90 XP. Pure data content -- no
    `.cpp`/`.h` changes, no new reward flag, no save-format changes.
    Verified via a throwaway self-test (`QuestLoader` against the real,
    now-16-quest `data/quests.txt` confirming the new quest's shape;
    `ZoneLoader` parsing the edited `plains_of_dust.txt`, confirming the new
    POI, the new `QUEST R` binding, and the new `SUBJECT` all parse
    cleanly), a clean `/W4` rebuild (zero new warnings -- no source files
    changed), and the piped smoke test (confirms `main.cpp`'s
    cross-validation accepts the new zone binding; no real save existed at
    `build\Debug\` to protect this session). **Interactive verification
    still needed** -- accepting the quest, killing 2 Ghouls, turning in, and
    confirming the new `SUBJECT`/POI read well in a real conversation --
    same `_getch()` limitation as every prior quest milestone. See
    `docs/QUEST_NOTES.md`'s "Shipped quests" and `docs/ZONE_NOTES.md`'s
    Plains of Dust section.

107. Three more monsters -- Black Bear, Worg, Ice Bear -- bringing the
    roster to 23, picked from `docs/COMBAT_NOTES.md`'s "Extending this
    later" backlog at the user's request for a new content pass, once a
    fresh check confirmed the quest well is dry (`docs/QUEST_NOTES.md`)
    and DLA's magic items are fully mined (`docs/CHARACTER_NOTES.md`) --
    the same "pick from the bestiary backlog once everything else is
    exhausted" path Milestone 100 took. **Black Bear** and **Worg**
    (*Monster Manual (2nd ed).pdf* pp.17/362, the "Bear"/"Wolf" comparison
    tables) are ordinary, non-Krynn-specific entries, same precedent as
    Bugbear/Ogre/Gnoll/Owlbear/Wight/Troll; both were visually confirmed
    against rendered page images. The Worg's real "often serve as mounts
    of goblins" line ties it directly to the Goblin/Hobgoblin already in
    this roster. **Ice Bear** (*Dragonlance Adventures*, TSR 2021, p.76,
    the "Creatures of Krynn" chapter) is this project's first roster
    addition from DLA's own broader bestiary chapter beyond the Draconians
    and Thanoi -- its prose directly references the Thanoi using ice bears
    to track prey and sharing the kill, so it inherits the same
    `ONLY_TERRAIN` glacier restriction and lands right alongside its
    already-shipped kin at Ice Wall. Its THAC0 isn't printed (same
    recurring DLA gap as every other Krynn-specific monster) and was
    derived via this project's established HD-to-THAC0 pattern; its XP is
    a real per-hp formula simplified to a flat value near the average
    roll, same treatment as the five Draconians. All three follow the
    existing "one representative die" simplification for multi-attack
    monsters (Ghoul/Owlbear/Troll's own precedent) and get no steel reward,
    matching the Wolf's own "wild animal, no worn treasure" precedent --
    all three print `TREASURE: Nil` in their real stat blocks, so this is
    sourced, not just tone-matched. Pure data addition to
    `data/monsters.txt`, no `.cpp`/`.h` changes -- `combat::MonsterLoader`
    already parses every keyword these three need (confirmed no monster id
    is hardcoded anywhere in `src/`). Verified via a clean `/W4` rebuild
    (zero new warnings) and the piped smoke test (confirms `MonsterCatalog`
    parses the file cleanly end-to-end, including all three new blocks);
    no throwaway self-test needed, same reasoning as every prior
    pure-content monster milestone. See `docs/COMBAT_NOTES.md`'s "Accuracy:
    what's sourced, what's invented" and "Extending this later" sections.

108. Fighter multi-attacks per round -- the first real engine change in
    several milestones (107 and before were pure data), closing a gap
    both `docs/CHARACTER_NOTES.md` and `docs/COMBAT_NOTES.md` had flagged
    as "not modeled" since the combat system's own introduction: the
    round loop always resolved exactly one attack per side, deferred
    because level 7 was "far off" at the time. Picked from the backlog
    at the user's request once the quest well and DLA magic items were
    both re-confirmed dry this session. PHB Table 15 ("Warrior Melee
    Attacks per Round," p.36, visually confirmed via a rendered page
    image, not OCR alone -- the raw text extraction badly garbles this
    table) is scoped to "warriors" (Fighter/Paladin/Ranger); this project
    only has Fighter in `ClassGroup::Warrior`, so Paladin/Ranger are moot.
    New `character::meleeAttacksThisRound(ClassId, level, roundNumber)`
    (`Leveling.h`/`.cpp`): 1-6 = 1/round, 7-12 = 3/2 rounds, 13+ =
    2/round, every non-Warrior class always 1. The "3/2 rounds" rate
    isn't printed with a specific odd/even breakdown, so this project
    took the standard interpretation (1 attack on odd rounds of the
    fight, 2 on even) as an invented-but-flagged convention, same
    treatment as the sell-back half-price rule (Milestone 28).
    `GameLoop::runCombat`'s `playerAttacks` lambda now loops that many
    times per round (breaking early if the monster already fell, mirroring
    the loop's existing early-exit pattern), fed by a new local
    `roundNumber` counter incremented once per `for(;;)` iteration; only
    ordinary weapon attacks get the multiplier -- casting/potions/webnet/
    Brooch/Staff of Curing stay one action per round, matching the real
    rule's scope to melee attacks specifically. No save-format changes, no
    new `Character` field. Verified via a throwaway self-test (13
    assertions across the level 6/7/12/13 boundaries, the 7-12 bracket's
    odd/even split, and every non-Warrior class), a clean `/W4` rebuild
    (zero new warnings), and the piped smoke test. **Interactive
    verification still needed** -- a Fighter actually reaching level 7 and
    13 in a real fight to see the extra swings and the odd/even pattern
    live -- same `_getch()` limitation as every other combat-facing
    milestone. See `docs/CHARACTER_NOTES.md`'s "Leveling / experience" and
    `docs/COMBAT_NOTES.md`'s "Accuracy" and "Attacks per round" sections.

109. Ability score ranges and class level limits -- closes a gap flagged as
    "not modeled, deliberately" since the Elf/Dwarf subrace milestone and
    the Kender milestone before it, picked from the backlog at the user's
    request once the quest well, DLA magic items, and (per Milestone 108)
    Fighter multi-attacks were all confirmed shipped. Sourced from
    rendered page images (not OCR, which badly garbles all of these
    tables): PHB Table 7 (p.27, ability-score min/max for base Gnome/
    Half-Elf/Halfling), DMG Table 7 ("Racial Class and Level Limits,"
    p.15 -- genuinely absent from the PHB, which explicitly defers this to
    "ask your DM"), and Dragonlance Adventures' own per-subrace/Kender
    ability-range and class-limit tables (pp.53,59-61,66-67). New
    `character::AbilityRange`/`meetsAbilityRange`/
    `meetsSubraceAbilityRange` and `classLevelCap` (`Race.h`/`.cpp`,
    replacing the old single-purpose `canBeMage` bool with a general
    per-class table `effectiveCanBeMage` now wraps). Both are
    **hard-enforced** in `CharacterCreator`: the race/subrace prompts
    reject a pick whose base (pre-adjustment) rolled scores fall outside
    its range and reprompt (Human always qualifies, so the player can
    never be dead-ended), and the class prompt rejects a class the
    sourcebooks mark "N/E" for that race/subrace, generalizing the old
    Mage-only block to all four core classes -- fixing two real,
    previously-live mismatches the cross-check turned up (Halfling could
    be a Mage; Silvanesti Elf could be a Thief). `Leveling.cpp`'s
    `applyPendingLevelUps` stops converting XP into levels once a
    demihuman hits their class's cap; XP itself keeps accruing, no new
    save field needed. Mage/Cleric map onto Dragonlance's licensed/
    sanctioned class rows (Wizard of High Sorcery, Holy Orders of the
    Stars) rather than the Renegade/Heathen ones, a user-confirmed project
    interpretation matching how those classes already play (Test of High
    Sorcery, real Cleric spells from the start). Two DM-optional/DL-edge-
    case "exceed the cap via an exceptional prime requisite" bonus-level
    mechanics (DMG Table 8, Kender's own STR 17/18 footnote) are
    deliberately not modeled, same restraint as Sword/Rose Knight's other
    unmodeled real abilities. No save-format changes. Verified via a
    throwaway self-test (30+ assertions: range boundaries for a
    representative sample of races/subraces, the two fixed mismatches,
    numeric cap spot-checks, and `applyPendingLevelUps` stopping exactly
    at a capped level even with enormous XP), a clean `/W4` rebuild (zero
    new warnings), and the piped smoke test. **Interactive verification
    still needed** -- triggering an actual reject-and-reprompt on both the
    race and class screens with real (random) rolls, and a demihuman
    Fighter/Cleric actually hitting their level cap in a long real
    playthrough -- same `_getch()` limitation as every other character-
    creation/leveling milestone. See `docs/CHARACTER_NOTES.md`'s "Ability
    score ranges and class level limits" section.

110. Removed Halfling as a playable race, at the user's request -- Krynn
    has no separate Halfling people in Dragonlance canon, the same reason
    Half-Orc was never one; Kender already fill that niche and (per
    Dragonlance Adventures p.53) are mechanically built on the PHB's own
    Halfling chassis anyway. `RaceId::Halfling` removed from `Race.h`/
    `.cpp` along with its `kTable`/`classLevelCap` entries; `kAllRaces`
    drops from 7 to 6; `game::conditionMatches`'s `"halfling"` REQUIRE/
    SAY_IF condition removed (confirmed unused by any `data/*.txt` file
    first). This surfaced a real, live instance of the raw-enum-int save
    fragility `docs/GOTCHAS.md` already documented but had never actually
    hit: deleting `Halfling` (ordinal 5) silently renumbered `Kender` from
    6 to 5, and a real save created minutes earlier in this same session
    (`build/Debug/save1.txt`, a level-1 Kender Thief named "Mason" -- the
    user's own test of Milestone 109) failed to load as a result. Fixed by
    pinning `RaceId::Kender = 6` explicitly rather than letting it
    renumber, leaving ordinal 5 permanently unused, plus a new
    `character::kRaceIdCount` (7, not `kAllRaces.size()`'s 6) for
    `SaveGame.cpp`'s RACE bounds check -- the same shape `ClassId`/
    `kClassIdCount` already used for Tinker's ordinal. Confirmed fixed by
    directly reloading the real save through the built executable (not
    just a unit test): the slot menu now shows "Mason, level 1 Kender
    Thief (Day 0)" and loads cleanly again. See `docs/GOTCHAS.md`'s
    `RACE`/`CLASS`/`ALIGNMENT` raw-enum-int fragility note for the
    general trap this confirms, and `docs/CHARACTER_NOTES.md`'s "Kender in
    place of Half-Orc and Halfling" section. No new self-test needed --
    the live-save reload is a stronger end-to-end proof than a synthetic
    one would be; a clean `/W4` rebuild (zero new warnings) and the piped
    smoke test cover the rest. No further interactive verification flagged
    beyond what Milestone 109 already carries.

111. Hoopak for Kender, plus two new armor tiers -- a user-requested
    "more weapons and armor" content pass. Research first (rendered page
    images, since this scan's OCR badly garbles table columns) found the
    PHB and Dragonlance Adventures both give the hoopak zero game stats,
    only flavor mentions -- real numbers came from `References/DQoK.pdf`
    (Dark Queen of Krynn, the official computer-game manual already used
    for this project's Spellcasting census), whose Weapons Table (printed
    p.51) gives a Melee (3-8, i.e. 1d6+2) and Missile (2-5, 1d4+1) profile,
    both footnoted "Only usable by kender characters." Only the higher
    Melee number is modeled (this engine has no ranged/melee distinction
    for any weapon). Unlike every existing `weaponUpgradeFor` entry
    (one per `ClassId`), the Hoopak is **race-gated**
    (`character.race == RaceId::Kender`) via a new `ShopItemKind::
    KenderWeapon`, additive rather than a replacement -- a Kender keeps
    their own class upgrade too. Cost (50stl) is invented, calibrated to
    the Fighter's Two-Handed Sword (same 5.5 average damage). Separately,
    the same research pass found a real armor-tier gap this project had
    skipped entirely: **Hide Armor** (AC 6, 15stl, PHB Table 46/47) sits
    between Studded Leather (AC 7) and Chain Mail (AC 5) -- and is
    genuinely cheaper than Studded Leather despite better AC, a real book
    quirk, not a research error. At the user's follow-up request, **Field
    Plate** (AC 2) was also added, reversing this project's earlier
    documented "too expensive" scope cut, but re-priced to 1200stl rather
    than the real 2,000gp -- a deliberate, flagged deviation from the
    "transcribe the sourced number" rule, done because the user asked for
    it directly, to keep it a reachable late-game item rather than the
    book's raw price. `ArmorId` gains `HideArmor`/`FieldPlate`, appended
    after `PlateMail` (ordinals 7-8) rather than inserted in AC order --
    same append-only-safe precedent Milestone 102 established for Studded
    Leather/Plate Mail -- and `SaveGame.cpp`'s two `ArmorId` bound checks
    move 7->9. Each shop's `ShopCatalogDef` picks up both new armor tiers
    and the Hoopak per its already-established character (General/Armory/
    Harbor get both new armor tiers; Market/Bazaar gain Hide Armor but not
    Field Plate; Salvage stays armorless; the Hoopak rides along wherever
    a class weapon upgrade is already sold -- General/Armory/Bazaar).
    Incidental correctness fix found while re-confirming Table 44 for the
    Hoopak: the Tinker's Light Crossbow was wrongly statted at 1d4+1 --
    that's actually the Heavy Quarrel's damage; the real Light Quarrel
    line (which this engine's ammunition-free crossbow reuses) is 1d4 with
    no bonus. Also fixed `purchaseItem`'s rejection message, hardcoded to
    "Wizards cannot wear armor or a shield." for any `buyable == false`
    item (already slightly wrong for a non-Mage rejected from Webnet/
    Brooch, clearly wrong for a non-Kender rejected from the Hoopak) --
    now a generic "You can't use that." Verified via a throwaway self-test
    (35 assertions: new armor tiers' AC/cost/resale, per-catalog armor
    filtering, the Hoopak's race-gate and resale, the corrected Tinker
    damage, and the corrected rejection message), a clean `/W4` rebuild
    (zero new warnings), the piped smoke test, and a direct check that
    both of the user's real saves (`save.txt`, `build/Debug/save1.txt`,
    the Kender Thief "Mason" from Milestone 110) load byte-for-byte
    unchanged and still show correctly in the slot menu. See
    `docs/CHARACTER_NOTES.md`'s "Equipment" section (armor tier bullet and
    new "Hoopak" subsection) and "Six shops, six catalogs".

    **Follow-up, same session**: at the user's direct request ("Kender
    characters should start with a hoopak"), `CharacterCreator::run` now
    assigns the Hoopak's stats as `Character::weaponName`/
    `weaponDamageSides`/`weaponDamageBonus` instead of `ClassInfo`'s
    normal starting weapon whenever `character.race == RaceId::Kender`,
    regardless of class -- a small, targeted branch (Mage is already
    race-blocked for Kender, so only Fighter/Cleric/Thief are reachable
    here). Doesn't touch the class's own `WeaponUpgrade` path at all -- a
    Kender Fighter can still buy a Two-Handed Sword later, same as before.
    The shop's existing `ownsWeapon` name-match check automatically greys
    out buying a duplicate starting Hoopak with no special-casing needed.
    Verified via a clean `/W4` rebuild (zero new warnings) and the piped
    smoke test; no throwaway self-test added, since the change is a
    three-line conditional assigning already-proven constants (the
    previous self-test already confirmed `kHoopakName`/
    `kHoopakDamageSides`/`kHoopakDamageBonus`), the same "too small to
    need one" judgment call as Milestone 103's own similar
    conditional-branch-in-CharacterCreator change. **Interactive
    verification still needed** (now covering all of Milestone 111) --
    buying Hide Armor/Field Plate at a shop that carries them, and a real
    Kender character confirming their character sheet shows "Weapon:
    Hoopak" at creation.

    **Second follow-up, same session**: at the user's direct request
    ("Kenders cannot be evil alignment"), re-checked the exact DLA p.53
    "Kender Game Statistics" box already sourcing Kender's ability ranges/
    class limits/Mage block, and found it states plainly: "No evil kender
    are known to exist." New `character::meetsAlignmentRestriction(RaceId,
    Alignment)` (`Race.h`/`.cpp`, `Race.h` now includes `Alignment.h` --
    an intra-`character/` dependency already established by
    `Knighthood.h`, which combines the same two headers) returns false
    only for Kender + a Lawful/Neutral/Chaotic Evil pick. Hard-enforced in
    `CharacterCreator::run`'s alignment prompt with the same
    annotate-and-reject-and-reprompt shape the race/class prompts already
    use, not the project's older soft-flag style -- closing the "No
    alignment restrictions" line in `docs/CHARACTER_NOTES.md`'s "Scope"
    section, now struck through with this one real exception noted. This
    is the first race-based alignment restriction this project has ever
    enforced. Verified via a throwaway self-test (54 assertions: all 9
    alignments checked against Kender and against every other race), a
    clean `/W4` rebuild (zero new warnings), the piped smoke test, and a
    direct check that the user's real saves still load unchanged (no
    save-format change at all -- this only gates a character-creation-time
    choice, the same way ability-range/class-cap checks already do).
    **Interactive verification still needed** (added to the same running
    list) -- creating a Kender and confirming the alignment screen
    annotates and blocks all three Evil options, while every other race
    still offers all 9 freely.

112. Three more Monster Manual monsters -- Lizard Man, Giant Toad, Ettin --
    bringing the roster to 26, picked once a fresh check confirmed the
    `NEXT UP` backlog below was essentially exhausted and Milestone 106
    had already declared the quest-hook well dry. `docs/COMBAT_NOTES.md`'s
    own "Extending this later" section still named the bestiary as the one
    genuinely open backlog item. Before returning to the ordinary Monster
    Manual, this session first checked DLA's own "Common Creatures of
    Krynn" chapter (pp.74-78, the same chapter Thanoi/Ice Bear came from,
    Milestone 107) for anything still unused -- Dreamshadow, Dreamwraith,
    Fetch, Minotaur (Bloodsea), Shadowpeople, and Spectral Minion are all
    real entries there, but none hold up under this project's
    wandering-overworld-encounter model: the first two only exist inside a
    *mindspin* spell's illusion, Fetch is reachable only through mirrors,
    Minotaurs are a civilized organized race (same reason Kender/Gnomes
    aren't monsters), Shadowpeople are Sanction-only (and Sanction itself
    is out of scope, see `docs/TIMELINE_NOTES.md`), and Spectral Minions
    are bound to one specific death-site, a location/quest fixture rather
    than a roaming encounter -- confirming the DLA well really is dry
    beyond Ice Bear. All three actual additions are visually confirmed
    against rendered Monster Manual page images (Lizard Man p.227, Giant
    Toad p.345, Ettin p.135, `pdftoppm`). Lizard Man's real three-attack
    claw/claw/bite and Ettin's real two-club attack are each simplified to
    their single most damaging hit, same "one representative die"
    treatment as the Ghoul's and Owlbear's own multi-attack
    simplifications; Giant Toad's real single 2-8 (2d4) bite needed no
    simplification. Lizard Man and Giant Toad both carry `TERRAIN_BIAS *`
    (bog), the project's only swamp-equivalent code and, until now, an
    almost-unused one -- both sourced directly from their own real
    Climate/Terrain fields (swamp; "near water"), not invented. Ettin
    carries `TERRAIN_BIAS ^ A` (hills+mountains, its own real field,
    reusing the exact combo Bugbear already established) and
    `MIN_TOWN_DISTANCE 40` -- HD10 is the highest in the roster, kept
    between Sivak (35) and Aurak (45); see `docs/COMBAT_NOTES.md`'s updated
    `MIN_TOWN_DISTANCE` writeup for the full Ettin-vs-Aurak comparison.
    Deliberate omissions, same restraint as every prior bestiary milestone:
    Lizard Man's advanced-tribe/Lizard King variant, Giant Toad's Fire/Ice/
    Poisonous Toad variants (all on the same source pages), and Ettin's
    real "speaks orc, goblin, giant dialects" flavor line, left out of its
    `DESC` entirely -- the same "no orcs on Krynn" restraint already
    applied to this whole roster. Pure data content -- no `.cpp`/`.h`
    changes (confirmed by reading `src/combat/MonsterLoader.cpp`: every
    keyword these three need already exists), no save-format changes.
    `README.md`'s Status paragraph's inline monster-name list was
    deliberately left untouched -- it's already been stale since Milestone
    100 (Owlbear/Wight/Troll/Black Bear/Worg/Ice Bear were never
    backfilled into it either), so it's treated as the established,
    illustrative example list it's already become rather than taking on an
    unrelated six-monster backfill here. Verified via a clean `/W4`
    rebuild and the piped smoke test only, no throwaway self-test needed
    (same reasoning as every prior pure-monster-roster milestone, 34, 100,
    and 107) -- no "interactive verification needed" flag either, since
    parsing is the only thing to confirm and the piped smoke test already
    covers it. See `docs/COMBAT_NOTES.md`'s roster/"Extending this later"
    sections.

113. Monster encounter groups, Phase 1 of a Gold Box (SSI's Pool of
     Radiance ... Dark Queen of Krynn) -inspired combat-screen pass --
     scoped down, with the user, from a full tactical grid to a
     self-contained first phase: multiple monsters of the same type per
     fight, individually tracked HP, lettered identity (Goblin A/B/C...),
     and a target picker. No position/grid/movement was added -- this
     project's combat stays a strictly-ordered, non-positional 1-vs-many
     exchange; a real tactical grid stays separate, later, deliberately
     unstarted work. Group size is sourced, not invented: every one of the
     26 roster monsters' real Monstrous Manual/*Dragonlance Adventures*
     "No. Appearing" field was re-checked via rendered page images (a
     field this project never needed before, combat having always been
     1-vs-1), then clamped to one small invented playability cap of 4. The
     research found real No. Appearing data supporting groups for nearly
     the whole roster, including several already-dangerous, currently
     ungated monsters (Wight, Troll, Thanoi, plus the already-gated Ogre/
     Kapak/Bozak/Sivak/Ettin) -- flagged back to the user via
     `AskUserQuestion` before writing any data, since grouping those
     without also re-tuning their danger gates would have shipped an
     untested difficulty spike. At the user's explicit direction, this
     pass only applies `GROUP <min> <max>` to the roster's 14 low/mid-HD
     "line troop"/wildlife monsters that already carry no
     `MIN_TOWN_DISTANCE` gate (Goblin, Kobold, Hobgoblin, Timber Wolf,
     Bugbear, Gnoll, Ghoul, Skeleton, Zombie, Baaz Draconian, Worg, Black
     Bear, Lizard Man, Giant Toad); the other twelve (Ogre, Kapak, Bozak,
     Sivak, Aurak, Ettin, Wight, Troll, Thanoi, Owlbear, Ice Bear, Giant
     Spider) stay solo exactly as before, a documented, deliberate
     deferral rather than an oversight. New `combat::Monster::groupMin`/
     `groupMax` (`Monster.h`) and `combat::rollGroupSize` (`Monster.cpp`,
     extracted as its own function specifically so it's unit-testable,
     same reasoning as `character::meleeAttacksThisRound`); `MonsterLoader`
     parses the new `GROUP` line with the same fail-fast idiom as every
     other line. `GameLoop::runCombat` is restructured around a
     `std::vector<MonsterInstance>` instead of a single HP int -- target
     selection is driven by how many instances are *currently* alive, not
     the group size rolled at the start, so a solo fight (still the
     overwhelming majority, unaffected) and a group fight fought down to
     its last survivor both auto-target the same way with no picker,
     protecting the ~40 already-verified single-monster milestones' worth
     of exact log wording from regressing. A Fighter's multi-attacks all
     land on one round's chosen target (wasted, not auto-redirected, if
     that target dies mid-volley); non-damage spell effects that used to
     implicitly target "the monster" (Sleep/Hold/Charm/Confusion/Fear,
     Bestow-Curse-style debuffs, Webnet) now ask which enemy first, one
     more "representative target" simplification consistent with this
     project's existing single-die/single-attack simplifications
     elsewhere; the Brooch of Imog's globe stays a shared ward since it
     protects the player, not a debuff on a monster. Bozak's Magic
     Missile/Aurak's breath weapon/the Giant Spider's poison bite all now
     roll independently per living instance (currently exercised as "loop
     of one" in practice, since none of those three are in the grouped
     tier this pass, but already correct if a future pass groups them).
     Steel/XP/the quest kill-tally are awarded the instant an instance
     dies rather than deferred to the end of the round, so a multi-kill
     round correctly advances a `SLAY` objective by more than one in a
     single fight with zero quest-system changes; the "Press any key"
     pause only fires once, when the last instance falls. Baaz Draconians
     are the one already-shipped monster whose default behavior actually
     changes -- they now always appear in groups of 2-4, never solo, since
     their real No. Appearing never supported a lone Baaz to begin with.
     `MapRenderer::drawCombatFrame` takes a `std::vector<CombatMonsterView>`
     (a small presentation-only adapter struct, same "stays ignorant of
     the domain type" pattern as `JournalEntry`/`DialogueLine`) instead of
     a single `combat::Monster` + HP pair; defeated instances stay visible
     marked `(defeated)` rather than disappearing, and the footer only
     hints `(choose target)` once 2+ are alive. Verified via a throwaway
     self-test (`MonsterLoader`'s `GROUP` parsing against both the real
     26-monster `data/monsters.txt` and malformed fail-fast cases,
     `rollGroupSize`'s bounds across 2000 trials), a clean `/W4` rebuild,
     and the piped smoke test (confirming the new `GROUP` lines parse and
     both of the user's real saves still load unchanged -- no save-format
     changes). **Interactive verification is required more than usual** --
     `_getch()` means none of the actual play loop (the target picker,
     multiple monsters attacking per round, a multi-attack landing all
     swings on one target, a target dying mid-volley, per-instance special
     abilities, a multi-kill fight progressing a quest) can be driven
     headlessly; see `docs/COMBAT_NOTES.md`'s "Monster encounter groups"
     section for the full sourcing table and design writeup, and
     `docs/CURRENT_WORK.md` for the specific scenarios still needing a
     real playthrough.

114. Positional combat grid, Phase 2 of the Gold Box-style combat pass --
     the piece Milestone 113 deferred: a real tactical grid with
     player/monster positions and movement, replacing the strictly
     turn-ordered, positionless exchange combat had used until now. Unlike
     Milestone 113's group sizes, this had a real source to check:
     `References/DQoK.pdf` (Dark Queen of Krynn, an actual SSI Gold Box
     Dragonlance game, already used for the Hoopak's weapon table at
     Milestone 111) has its own "COMBAT" section (manual pp.9-11)
     describing this exact system -- re-read before finalizing the design,
     which corrected two assumptions and added a real mechanic the
     original plan had gotten wrong or left out: ranged weapons are
     disabled *while* adjacent to an enemy (not usable at any range
     unconditionally, the plan's first draft), the combat map is
     terrain-flavored (not a blank grid), and moving away from an
     adjacent enemy provokes a real, sourced opportunity attack. New
     `src/combat/CombatGrid.h`/`.cpp` (`combat::GridPos`, `isAdjacent`,
     `stepToward` -- extracted as their own functions specifically so
     they're unit-testable, same reasoning as `combat::rollGroupSize`),
     an 11x7 grid (`render::MapRenderer::kCombatGridWidth`/
     `kCombatGridHeight`) rendered in plain text (this project's
     "organic" screen family only supports one color per line, not per
     cell) but using the real tile's terrain glyph for the floor. Melee
     attacks now require adjacency (`pickTarget` gained an eligibility
     filter); the Tinker's Light Crossbow
     (`character::kLightCrossbowName`, same plain-string-compare pattern
     as `kFrostreaverName`) is this project's first-ever ranged weapon,
     able to hit anyone on the grid while the player isn't adjacent to
     anyone, refused outright the instant an enemy closes to melee range;
     monsters (none of which have a ranged attack) close distance via
     `combat::stepToward` when not adjacent instead of attacking from
     wherever they stand. `w`/`a`/`s`/`d` (previously ignored inside
     combat) move the player as a full round action, validated up front
     the same "reject before it costs a round" way an unusable spell
     press already is. Deliberately not adopted, honestly flagged rather
     than silently dropped: segmented (1-10) initiative, variable
     movement speed from encumbrance, speed-based/edge-of-map `Flee`, 2
     arrows/3 darts per turn and real range brackets for missile weapons,
     and thief backstab -- all real DQoK.pdf mechanics this project isn't
     taking on this pass. **A Milestone 113 discrepancy was found and
     fixed in the same session, at the user's request**: the manual's
     real rule retargets a Fighter's remaining multi-attack swings to a
     new opponent if the first target dies mid-volley, rather than
     wasting them the way Milestone 113 originally shipped -- `fightEndedByBurst`
     was also renamed to `fightAlreadyEnded` since an opportunity attack
     can now end the fight the same way a death-burst already could. No
     save-format changes. Verified via a throwaway self-test
     (`combat::isAdjacent` across all 8 neighbors plus self/distance-2,
     `combat::stepToward`'s bounds-respecting, collision-avoiding, greedy
     approach), a clean `/W4` rebuild, and the piped smoke test
     (confirming both of the user's real saves still load unchanged).
     **Interactive verification required, same heavier-than-usual flag as
     Milestone 113** -- `_getch()` blocks the actual play loop (the grid
     itself, movement, the ranged-weapon lockout, the opportunity attack,
     monster pathing) from being driven headlessly. See
     `docs/COMBAT_NOTES.md`'s "Positional combat grid" section for the
     full sourcing and design writeup, and `docs/CURRENT_WORK.md` for the
     specific scenarios still needing a real playthrough.

115. In-frame combat actions, Phase 3 of the Gold Box-style combat pass --
     fixing the user's own complaint that "the picking who to attack takes
     you away from the screen." Every sub-choice inside a fight (which
     enemy to attack, which spell to cast) used to pop a full-screen
     `render::MapRenderer::drawPickerFrame`, clearing the terminal and
     replacing the grid/HP roster/log with a bare list at exactly the
     moment the player needed them. `References/DQoK.pdf`'s own manual
     confirms this is a real deviation, not just a taste call -- it
     targets on the battle map itself ("Use the CENTER command to
     determine who will be in the area of effect... if the spell is
     targeted in the center of the screen"; Hold Person: "use the EXIT
     command to target fewer"). Scope was checked against a ranked gap
     analysis of everything left separating this project from a real Gold
     Box game (multi-square movement, segmented initiative, a party of up
     to six, area-of-effect spells, mixed-monster encounters, sweep
     attacks, real Flee) -- the user picked in-frame targeting plus a real
     action menu as this milestone's scope, and a party of up to six as
     the backlog item to record (see `docs/COMBAT_NOTES.md`'s "Extending
     this later"). New `render::MapRenderer::CombatPrompt` drives
     `drawCombatFrame`'s three states: idle (a real command row -- `ATTACK
     (Enter)   MOVE (wasd)   CAST (m)   USE: <item> (i)   FLEE (f)`,
     naming only what's legal right now), target picking (the grid itself
     is the picker -- the cursored instance's cell renders `[X]` instead
     of ` X `, cells widened from 1 to 3 columns to fit the bracket, and
     its HP-roster line gets a `> ` prefix), and an in-frame option list
     (spell/item selection, which has no grid cell to point at). New
     `character::availableCombatItems` (`Equipment.h`/`.cpp`) replaces the
     old fixed-priority `'i'`-key handling (Potion, then Webnet, then
     Brooch, then Staff, stopping at the first match) with the full list
     of everything usable this round -- a real, previously-live gap this
     closes along the way: a character carrying both a Potion and a
     Webnet could never reach the Webnet through `'i'` before this
     milestone. Auto-selects with no chooser when exactly one item
     qualifies, same "no picker for one candidate" rule `pickTarget`
     already followed. Deliberately not adopted, honestly flagged: a
     free-roaming cursor over empty grid squares (DQoK.pdf's real
     behavior, deferred until area-of-effect spells actually need one),
     DELAY/QUIC/multi-target CENTER-EXIT as named commands (all
     presuppose segmented initiative or a party, neither being taken on),
     and a cursor-navigable command row (the row is display-only --
     `Console::readKey` already maps a fixed key set). No save-format
     changes. Verified via a throwaway self-test
     (`character::availableCombatItems` across no items, one item, all
     four available, a Brooch/Staff already used today excluded, and a
     Brooch used yesterday available again), a clean `/W4` rebuild, and
     the piped smoke test (both of the user's real saves confirmed intact
     around the run). **Interactive verification required**, same
     `_getch()` limitation as every other combat-facing milestone -- see
     `docs/COMBAT_NOTES.md`'s "In-frame combat actions" section for the
     full design writeup and `docs/CURRENT_WORK.md` for the specific
     scenarios still needing a real playthrough.

116. A recruitable party companion, Phase 1 of a party system -- `NEXT UP`
     item 6 (recorded at Milestone 115), picked at the user's request to
     start work toward a real party. Offered a choice via `AskUserQuestion`
     between a full one-pass build and an incremental first slice, the
     user chose the smaller slice: **recruitment only, no combat yet** --
     the same "ship the smaller half first" precedent Milestone 113
     (monster groups) set before Milestone 114 (the grid). Research
     confirmed why the full version wasn't attempted in one pass:
     `GameLoop::runCombat` is already one large function built entirely
     around a single `character::Character`, and `combat::
     resolvePlayerAttack`/`render::MapRenderer::drawCombatFrame`/the HUD/
     `CharacterCreator` all assume exactly one PC exists. New sibling
     module `character::buildCompanion()` (`Companion.h`/`.cpp`) builds a
     fixed, deterministic level-1 Human Fighter (Bren Alder, Neutral Good)
     -- fixed ability scores and starting steel, never `character::roll`,
     reusing the same non-interactive rules functions `CharacterCreator::
     run()` calls -- so `game::SaveGame` only needs to persist a single
     `COMPANION 1` bool (`GameState::hasCompanion`) rather than serializing
     the companion's own fields; reloading just calls `buildCompanion()`
     again. Recruited via a new `RECRUIT <char>` zone-grammar line (`world::
     PointOfInterest::recruitsCompanion`, same "must already have a POI and
     a TALK line" validation as `BOAT`/`GRANTS_ITEM`/`QUEST`, but no id
     payload -- there's exactly one companion this phase) at `data/zones/
     solace.txt`'s new `K "Bren Alder"` POI; `GameLoop::talkTo` gained an
     Accept/Decline picker ("Join me" / "Not yet") in the same slot/shape as
     the existing `BOAT` block. Once joined, shown on the character sheet
     (a new terse "Companion:" block) and the overworld/zone HUD status
     panel (a "Companion: name HP x/y" line). Deliberately not attempted
     this phase, honestly flagged: the companion cannot fight (`runCombat`
     doesn't read `hasCompanion`/`companion` at all -- a fight plays out
     exactly as before, companion or not), cannot shop, level, or spend
     steel, cannot be dismissed once recruited, and has no independent
     position/glyph on the map -- all real, sourced gaps reserved for later
     phases (Phase 2: the companion fights, AI-controlled; Phase 3: a real
     multi-companion roster, player-directed control, deployment order,
     backstab, sweep, `UIC`), not oversights. Full design writeup:
     `docs/ARCHITECTURE.md`'s "Party companions" section,
     `docs/CHARACTER_NOTES.md`'s "Party companion" section, and
     `docs/COMBAT_NOTES.md`'s "Extending this later". Verified via a
     throwaway self-test (`character::buildCompanion()` called twice,
     asserting determinism across every field, plus sanity checks against
     `classInfo(Fighter)`), a clean `/W4` rebuild, and the piped smoke test
     (confirmed the new `RECRUIT` zone-grammar line and edited
     `solace.txt` load cleanly, and that all three of the user's real
     saves -- written before `COMPANION` existed -- still load correctly
     with no companion). **Interactive verification needed**, same
     `_getch()` limitation as every other `GameLoop`-facing milestone --
     talking to Bren Alder in Solace, declining then confirming the offer
     re-appears on a later visit, accepting and confirming the character
     sheet/HUD both show the companion, and saving/reloading to confirm the
     companion persists with identical stats. See `docs/CURRENT_WORK.md`.

117. Party combat, Phase 2 of the party system -- the companion actually
     fights. Continuing `NEXT UP` item 6 after Milestone 116's Phase 1.
     Before starting, the user was asked (`AskUserQuestion`) whether the
     companion should be able to take damage this phase or only deal it;
     chose **full mutual combat** over a smaller "free companion" (deals
     damage, can't be hit) slice -- a companion immune to harm would be a
     hollow half-measure, not a real ally. The companion (still Bren Alder,
     the one fixed level-1 Human Fighter from Milestone 116) now occupies
     its own cell on the Milestone 114 tactical grid, starting adjacent to
     the player; it acts automatically right after the player's own action
     each round -- AI-controlled, never opens a picker or asks the player
     anything (player-directed control stays Phase 3) -- attacking an
     adjacent alive monster instance, or stepping toward the nearest one
     via `combat::stepToward` and a new pure helper, `combat::
     chebyshevDistance` (`CombatGrid.h`/`.cpp`). Monsters now pick between
     the player and the companion as their melee target each turn: attack
     whichever they're adjacent to, coin-flip (`character::roll(1,2)`) if
     adjacent to both, or close on whichever is nearer if adjacent to
     neither -- the first real touch to `GameLoop::runCombat`'s
     single-`Character` assumption that Milestone 116's own design doc
     flagged as the reason Phase 1 stayed combat-free. Made possible with
     zero changes to `combat/Combat.h`/`.cpp`: `resolvePlayerAttack`/
     `resolveMonsterAttack`/`rollSavingThrow` already took a generic
     `const character::Character&`, so the companion's attacks/defenses
     reuse them exactly as Milestone 116's design doc predicted ("the
     companion is a real Character, not a parallel struct"). Companion HP
     is real and persists now: `GameState::companion.currentHp` is mutated
     during combat exactly like the player's own, and `SaveGame`'s
     `COMPANION` line grew a second, optional field to carry it -- a
     pre-Milestone-117 one-token `COMPANION 1` line still loads, correctly
     defaulting to full health (combat never touched the companion before
     this milestone). A knocked-out companion (HP <= 0) stops acting and
     being targeted for the rest of that fight, logged once, but does NOT
     end the fight -- only the player's own knockout does that. `Rest`/
     `BedRest` heal the companion the same way they already heal the
     player. `render::MapRenderer::drawCombatFrame` gained a companion
     glyph on the grid, an HP/AC line (a new `kCompanionCombatColor`,
     bright green, distinct from the player's white and the monsters' red),
     and a "(knocked out)" marker, reusing the existing `CombatMonsterView`
     struct as a presentation carrier rather than adding a parallel type.
     **Deliberately deferred, honestly flagged (not oversights)**: the
     Brooch of Imog's globe wards the player only, never the companion;
     Bozak's Magic Missile and Aurak's breath weapon stay hardcoded
     player-only special attacks (teaching them to pick between two targets
     is real extra scope, the same family as the already-deferred "no
     square-cursor for AoE" gap); Sivak's death-burst still only damages
     the player regardless of who lands the killing blow (a pre-existing
     simplification, not extended here); companion movement never
     provokes or takes opportunity attacks; there's no "finish off a downed
     ally" mechanic. Full design writeup: `docs/ARCHITECTURE.md`'s "Party
     companions" section, `docs/CHARACTER_NOTES.md`'s "Party companion"
     section, and `docs/COMBAT_NOTES.md`'s "Extending this later" section.
     Verified via a throwaway self-test (`combat::chebyshevDistance` across
     several coordinate pairs; a `SaveGame` round-trip proving a damaged
     companion's HP survives save/load exactly and that an old one-token
     `COMPANION 1` line still loads at full health -- both deleted after),
     a clean `/W4` rebuild, the piped smoke test (all three real saves
     moved aside to reach character creation cleanly, then restored
     byte-for-byte), and a direct check that the real `save1.txt` --
     already carrying a Milestone-116-vintage one-token `COMPANION 1` line
     -- still loads correctly through the real executable. **Interactive
     verification needed**, same `_getch()` limitation as every other
     combat-facing milestone -- see `docs/CURRENT_WORK.md` for the specific
     scenarios still needing a real playthrough.

118. A real multi-companion roster, Phase 3a of the party system --
     `NEXT UP` item 6, continued after Milestone 117 shipped full mutual
     combat for the one existing companion. The user picked to start Phase
     3, then, offered a further scope split via `AskUserQuestion` between
     "player-directed control (a UIC toggle)" and "a real multi-companion
     roster" for this first Phase-3 slice, chose the roster -- growing the
     *count* dimension while leaving the *control* dimension (still
     AI-only) for later, the same "ship the smaller half first" precedent
     every prior party-system phase has followed. Before planning, the
     real DQoK.pdf manual (already used to source the combat grid and the
     Hoopak) was re-read for its own "UIC" command, confirming it really is
     a separate, deferred feature ("You control the actions of PCs. The
     computer controls the actions of monsters, NPCs, and PCs set to
     computer control with the UIC command") and that party deployment
     order is a pre-combat camp-menu step, not something this slice needed
     to touch. `GameState::hasCompanion`/`companion` (a single slot) became
     `GameState::companions`, a `std::vector<game::RecruitedCompanion>`
     (each entry pairing a save-format id with a real `character::
     Character`) -- deliberately no hardcoded numeric cap, bounded by
     content (two companions) rather than a `kMaxPartySize` constant, per
     "no premature abstraction." A second companion, **Dessa Corrin** (a
     Human Thief, id `dessa_corrin`, Chaotic Good, fixed level-1 stats same
     as Bren Alder's own non-rolled convention), joins at Haven via a new
     `I "A Watchful Stranger"` POI -- deliberately another non-spellcaster,
     same reasoning Milestone 116 gave for Bren Alder's own class ("a
     caster companion raises 'can they memorize/cast' questions that
     belong in a later phase"). `RECRUIT <char>` grew a required id payload
     (`RECRUIT <char> <companion-id>`), cross-checked by `main.cpp` against
     new `character::isKnownCompanionId` once all zones load, the same
     deferred-cross-check shape already established for `QUEST`/
     `SHOP_LOCKED` ids -- `world::` still never references `character::`
     directly. Recruitment gating moved from a single `hasCompanion` bool
     to a per-companion-id check against the roster, so Bren Alder and
     Dessa Corrin can each be recruited independently, in either order.
     The real engineering core was generalizing `GameLoop::runCombat` from
     a binary (player vs. one companion) to an N-ary party: each companion
     gets its own starting grid position (a small alternating left/right
     pattern off the player) and its own combat glyph (`'c'`, `'d'`, ...);
     `companionActs()` now loops every companion in roster order; monster
     AI's old "adjacent to player / adjacent to companion / coin-flip"
     logic became a small local `PartyTarget` list (the player plus every
     alive companion) picked among uniformly at random via `character::
     roll(1, N)` when more than one is adjacent -- collapsing what used to
     be duplicated attack-resolution code (identical `resolveMonsterAttack`/
     poison-save/knockout-log shape written out twice) into one shared
     block, a real simplification alongside the added scope, not just
     more code. `render::MapRenderer::drawStatusPanel`/`drawCharacterSheet`/
     `drawCombatFrame` all grew their single-companion parameter into a
     `std::vector`. `SaveGame`'s `COMPANION` line grew a real id token
     (`COMPANION <id> <currentHp>`, one line per recruit, repeatable) --
     backward-compatible with the legacy one-token `COMPANION 1` line
     (mapped to id `bren_alder`). Confirmed with none of Bozak's Magic
     Missile/Aurak's breath weapon/the Brooch's globe/Sivak's death-burst
     needing any code change to stay player-only: `grep` confirmed none of
     those call sites ever referenced the companion at all. Full design
     writeup: `docs/ARCHITECTURE.md`'s "Party companions" section,
     `docs/CHARACTER_NOTES.md`'s "Party companions" section, and
     `docs/COMBAT_NOTES.md`'s "Extending this later" section. Verified via
     a throwaway self-test (`buildCompanionById` for both known ids plus a
     fail-fast unknown-id check; a `SaveGame` round-trip proving two
     recruited companions' ids/currentHp both survive save/load exactly,
     and that a hand-inserted legacy one-token `COMPANION 1` line still
     loads as Bren Alder alone at full health -- 16 assertions, all
     passed, deleted after), a clean `/W4` rebuild (zero new warnings), the
     piped smoke test (all three real saves moved aside to reach character
     creation cleanly, confirming the new zone grammar and `main.cpp`
     cross-check both load without error, then restored byte-for-byte),
     and a direct check that the real `save1.txt` -- which already carried
     a live Milestone-117-vintage `COMPANION 1 0` line -- still loads
     correctly through the real executable (slot listing shows "Mike,
     level 1 Human Fighter (Day 1)" with no error), the strongest possible
     confirmation of the legacy-compat path since it's a real save, not a
     synthetic fixture. **Interactive verification needed**, same
     `_getch()` limitation as every other combat-facing milestone -- see
     `docs/CURRENT_WORK.md` for the specific scenarios still needing a
     real playthrough.

**Follow-up, same session**: the user's own real playthrough recruited
Dessa Corrin alongside Bren Alder and fought with both (the real save file
confirms both surviving combat with distinct, correctly tracked HP) and
surfaced a real bug: a party wipe (`GameLoop::runCombat`'s `knockedOutBy`,
which restores the player to full HP and carries them back to the nearest
refuge) only healed the player -- a companion knocked out during that same
losing fight stayed at 0 HP indefinitely afterward, silently benched from
every subsequent fight until the next Rest/BedRest, since nothing else
ever revived a companion outside those two commands. Fixed by healing
every recruited companion to full inside `knockedOutBy`, right alongside
the player's own recovery. Verified via a clean `/W4` rebuild (zero new
warnings) and the piped smoke test (real saves moved aside and restored
byte-for-byte); not yet re-confirmed by the user in an actual party-wipe
scenario, since triggering one wasn't part of this session's own
playtesting. See `docs/ARCHITECTURE.md`'s "Party companions" section and
`docs/CHARACTER_NOTES.md`'s "Party companions" section for the updated
recovery description.

119. Thief backstab and Fighter sweep attacks -- `NEXT UP` item 6's last
     open pair besides player-directed control/deployment order, picked by
     the user to start this session. Both are real DQoK.pdf mechanics
     flagged since Milestone 113/114 as needing a second party member,
     which the multi-companion roster (Milestones 116-118) now provides.
     `References/DQoK.pdf` (re-read via a fresh `pdftotext -layout` pass,
     p.9-10 of the printed manual) gives the qualitative rule for both:
     sweep ("Fighter-types may also 'sweep' through several weak
     opponents in one combat round... automatically attacks all of the
     weak opponents") and backstab, in its own **positional** form
     distinct from the classic PHB surprise/unaware-target rule ("A thief
     'back stabs' if he attacks a target from exactly opposite the first
     character to attack the target. The thief may not 'back stab' if he
     has readied armor heavier than leather.") -- this project follows
     DQoK's positional version exclusively, its established preference
     when DQoK's own combat-chapter wording differs from the classic PHB
     text. `References/Player's Handbook (revised).pdf` (p.57, Table 30)
     supplied the numeric backstab damage multiplier DQoK doesn't print
     (level 1-4 = x2, 5-8 = x3, 9-12 = x4, 13+ = x5) and the +4 to-hit
     bonus, same "reuse the PHB's real numbers when DQoK is silent"
     convention already used for magic weapon Steel prices. Sweep's
     "weak opponent" threshold has no surviving numeric definition in
     DQoK's own extracted text either -- defined as `Monster::
     hpDiceCount <= 1` (`combat::isSweepEligible`), which is already how
     this project encodes a monster's real 2e Hit Dice, confirmed against
     `data/monsters.txt`'s Goblin/Kobold/Hobgoblin/Skeleton (real HD
     1-1/~1/2/1+1/1), the same "line troop" tier Milestone 113's GROUP
     feature already singled out; since every encounter is N copies of
     one `Monster`, eligibility is one check per fight, not per-instance.
     New pure helpers, same "extracted for unit-testability" family as
     `isAdjacent`/`stepToward`/`chebyshevDistance`: `combat::oppositeSide`
     (mirrors a position through a target -- `CombatGrid.h`/`.cpp`),
     `combat::isSweepEligible` (`Monster.h`/`.cpp`), `character::
     canBackstab` (armor + class-group gate, `Equipment.h`/`.cpp`,
     alongside `canWearArmor`), `character::backstabDamageMultiplier`
     (`Leveling.h`/`.cpp`, alongside `meleeAttacksThisRound`).
     `combat::resolvePlayerAttack` gained a `damageMultiplier` parameter
     (default 1) applied to the raw weapon die roll before Strength/magic
     bonuses (Table 30's own text: "multiplied... before modifiers... are
     applied"), and `AttackOutcome`/`game::describeDamage` were updated so
     the combat log renders it honestly (`[1d8 5 x3 +2 = 17]`) instead of
     silently implying an over-max die roll. The real design work was
     `GameLoop::runCombat`'s per-instance first-attacker tracking for
     backstab: initially planned as a per-round reset (matching a first
     instinct at "the first character to attack the target"), corrected
     during implementation to persist for the instance's whole lifetime
     in the fight instead, tracked by attacker *identity* (not a frozen
     position) so the opposite-side check always reflects that
     character's current position. The per-round design was found to be
     asymmetric and effectively broken: `playerActs()` always runs before
     `companionActs()` in both initiative branches, so with a per-round
     reset a Thief player could never backstab off a companion's own
     same-round engagement -- only companions could ever backstab around
     the player. Persistent tracking fixes this and matches the manual's
     text more literally besides (it says nothing about a per-round
     reset). Both abilities apply symmetrically to the player and to
     every companion via one shared `backstabBonus`/`adjacentWeakInstances`
     lambda pair in `playerAttacks`/`companionActs`, so e.g. Bren Alder
     (Fighter) holding one flank of a monster while Dessa Corrin (Thief)
     or a Thief player moves to the exact opposite grid side triggers a
     companion-assisted backstab, in either direction -- a real payoff of
     Milestone 118's roster having one of each class. Full design
     writeup: `docs/COMBAT_NOTES.md`'s "Thief backstab and Fighter sweep
     attacks" section, `docs/CHARACTER_NOTES.md`'s "Leveling / experience"
     and "Party companions" sections, `docs/ARCHITECTURE.md`'s "Party
     companions" section. Verified via a throwaway self-test
     (`oppositeSide` across all 8 offsets plus an involution check;
     `isSweepEligible` at HD 1/2/4; `canBackstab` across None/Leather/
     StuddedLeather/ChainMail and a non-Thief class; `backstabDamage
     Multiplier` across all four level-band boundaries; `resolvePlayer
     Attack`'s new multiplier parameter with a pinned 1-sided weapon die --
     26 assertions, all passed, deleted after), a clean `/W4` rebuild
     (zero new warnings), and the piped smoke test (real saves moved
     aside and restored byte-for-byte). **Interactive verification
     needed**, same `_getch()` limitation as every other combat-facing
     milestone -- see `docs/CURRENT_WORK.md` for the specific scenarios
     still needing a real playthrough.

**Follow-up**: that interactive playthrough surfaced a real bug first
(a queued/held movement key silently replaying as combat's first action,
wrecking backstab/sweep's exact-grid-position requirement -- fixed via
`Console::flushInput()` plus an Enter-gated dismissal screen, see
`docs/GOTCHAS.md`'s Input section). With that fixed, the user confirmed
backstab and sweep both work correctly in real fights. Milestone 119 is
now fully verified, nothing further outstanding.

120. Battle-map restyle -- a small, purely cosmetic pass, requested
     directly by the user ("make the battle map nicer looking, I don't
     like the x's around to start") rather than picked from `NEXT UP`.
     The "x's" turned out to be the tiled terrain glyph filling the
     combat grid's empty floor: Milestone 114 drew every empty cell as
     the real tile's `world::TerrainInfo::glyph`, so a forest encounter
     (11% encounter chance, one of the highest on the map) rendered 77
     `%` characters across the board, which reads as a field of x's and
     drowns out the `@`/companion/monster-letter glyphs that are the only
     cells a player actually reads during a round. The user picked the
     fix from three mocked-up options: uniform `.` floor + a bordered
     grid + the terrain named on its own label line, over "blank floor"
     (cleanest, but you can no longer count squares when planning a move,
     which matters given movement is a full round action) and "keep the
     terrain glyphs, just frame and center them" (fixes the layout but
     not the noise). Implemented entirely inside
     `MapRenderer::drawCombatFrame`: a `kSectionLabelColor` "Battlefield:
     forest" line built from `TerrainInfo::name`, a `+---...---+` /
     `|`-sided border around the 7 grid rows, and `kCombatFloorGlyph`
     (`.`) in place of `floorTerrain.glyph` in the empty-cell branch. No
     signature change (`floorTerrain` is still the same parameter, now
     read for `.name` instead of `.glyph`), no gameplay effect, no save-
     format effect, and the Milestone 115 `[X]` target-picker bracket is
     untouched and still legible in the edge columns where it now abuts
     the border directly. The manual's sourced concept is preserved --
     DQoK.pdf's "a detailed view of the terrain that the party was in" is
     now *stated* rather than tiled. Full writeup: `docs/COMBAT_NOTES.md`'s
     "Positional combat grid" section, "Presentation restyle" bullet.
     Verified with a throwaway render harness
     (`src/render/CombatFrameSelfTest.cpp` plus a temporary
     `combat_frame_self_test` CMake target -- drawCombatFrame needs no
     keyboard input, so unlike the rest of combat it *can* be exercised
     headlessly) printing three real frames: a group fight with two
     companions and one defeated instance, the same frame with the target
     picker open on instance B, and a solo grassland fight with no
     companions. All three rendered correctly; harness and CMake target
     deleted afterward, per the throwaway pattern. Then a clean `/W4`
     rebuild (zero warnings) and the piped character-creation smoke test.
     Side note worth recording: the "clean rebuild" step was done by
     `Remove-Item -Recurse -Force` on `build\`, which also wiped the
     runtime save slots (`save1.txt`/`save2.txt`/`save3.txt`) that live in
     `build\Debug` next to the exe -- gitignored, so nothing warned about
     it. The user confirmed the saves were disposable, so no harm done
     here, but the file-layout gotcha is real and is now noted in
     `docs/GOTCHAS.md` under Save/load for anyone mid-playthrough who
     would rather not redo one.

121. The Wayreth quest -- requested directly by the user: "the Towers of
     High Sorcery need to be added." Research turned up that the Tower of
     Palanthas was already in the game (Milestone 44, `data/zones/
     palanthas.txt` `POI T` -- sealed, no interior). The Tower of Wayreth
     was the real gap, and it comes with a genuine sourcing constraint:
     this project's own already-shipped Raistlin dialogue (`data/
     timeline.txt`, Milestone 72) says the Tower "does not stay where it
     was the day before... finds you rather than the reverse" --
     `References/TSR 2143 PG1 Players Guide to the Dragonlance Campaign.pdf`
     confirms this as real sourced lore (an NPC there says outright "Not
     even I could find the Tower of Wayreth"), not invented flavor. Put to
     the user directly: how to reconcile "add it" with "it can't be
     found." Their answer -- unfindable for everyone except a Mage, and
     even then reachable only through a quest, never a walk-up map
     location -- is what this milestone implements.

     Deliberately **no new `LOCATION`/zone file**. A persistent walkable
     Wayreth zone would need overworld coordinates, which either breaks
     "unfindable" (reachable on foot, by anyone) or risks a real softlock
     (unlike the SEA_LOCKED islands, each of which has its own return-boat
     POI, Wayreth has no physical place to put a return trip's arrival
     point). Instead the whole visit is a scripted round trip bundled
     entirely into one new quest's Accept->Complete flow.

     `wayreth_summons` (`data/quests.txt`) is offered by a new POI, "A
     Robed Stranger" (`data/zones/solace.txt` `POI R`), gated by a new
     compound condition `wayreth_eligible` (`game::conditionMatches`,
     `GameLoop.cpp`) -- `charClass == Mage && level >= 3`, the same shape
     as `sword_eligible`. An unmet `REQUIRE` means the Stranger has
     nothing to say about the quest at all, so no other class, and no
     Mage below level 3, ever sees a hint Wayreth exists; their base
     `TALK`/`TALK_AGAIN` text is deliberately mundane. One objective,
     `VISIT palanthas`, reuses already-shipped content for free: the Great
     Library's existing `SAY_IF L mage` Test-of-High-Sorcery flavor, and
     `.research/dwn_full.txt` (~line 3648)'s own detail that the Towers'
     surviving spellbooks were given to "the great library at Palanthas."

     A new reward flag, `REWARD_WAYRETH_ROBE` (`quest::Quest.h`,
     `QuestLoader.cpp`, same "named, specific, compile-time flag" shape as
     `REWARD_KNIGHT_SWORD`), does the real work on turn-in
     (`GameLoop::offerOrTurnInQuest`): assigns `character::RobeColor` by
     alignment and narrates one of three White/Red/Black outcome
     passages. Those three passages aren't new prose -- they're Milestone
     103's own text, **relocated verbatim** from `character::
     applyPendingLevelUps` (`Leveling.cpp`), because a quest's `COMPLETE`
     field is fixed text and can't branch by alignment the way this
     needed to. `Leveling.cpp`'s level-3 Mage branch now only foreshadows
     ("You feel, faintly, that something has taken notice of you") instead
     of resolving the Test outright; `character.robeColor` stays
     `RobeColor::None` (already its default, already hidden from the
     character sheet in that state) until this quest actually completes --
     consistent with the source material, since not every mage takes the
     Test the moment they're able to. Full writeup: `docs/CHARACTER_NOTES.md`'s
     "The Wayreth quest", `docs/QUEST_NOTES.md`'s "Shipped quests", and
     `docs/ZONE_NOTES.md`'s Solace section.

     Verified via the piped smoke test (confirms the new quest block and
     zone POI/binding all parse cleanly, and `main.cpp`'s cross-validation
     accepts it) and a clean `/W4` rebuild (zero new warnings).

     **Interactively verified in a follow-up session**, on a hand-crafted
     level-3 Mage save (Serath, Human, True Neutral -- values derived
     directly from this project's own tables, since `_getch()` can't be
     scripted to play up from level 1). Confirmed end to end: the Robed
     Stranger offers `wayreth_summons` at level 3; `VISIT palanthas`
     tracks correctly (quest moved Active -> ReadyToTurnIn only after
     Serath actually reached the Great Library and got Astinus's `SAY_IF
     L mage` line); turning the quest in back at Solace narrated the
     scripted trip to Wayreth and assigned `ROBE 2` (Red/Lunitari),
     exactly matching True Neutral per `character::robeForAlignment`; the
     character sheet reflected it correctly afterward. The one item not
     separately re-confirmed by walking a non-Mage character to the
     Stranger: the `wayreth_eligible` gate itself is a simple, direct
     boolean check, already read/reviewed, low risk.

     This same verification playthrough surfaced a real, unrelated gap in
     how the player actually gets to Palanthas from Solace -- see
     Milestone 122.

122. Port O'Call, a real round-trip Crossing ferry, and a Flotsam sea
     route -- all surfaced by Milestone 121's own verification
     playthrough (Serath, walking from Solace toward Palanthas, hit a
     genuine dead end at Crossing). Trying to keep walking north from
     Crossing across the Strait of Schallsea hit a single stray true-
     ocean pixel at overworld `(200, 182)` -- a real map-generation bug,
     found and a `MANUAL_TERRAIN_OVERRIDES` fix drafted, then deliberately
     reverted at the user's request in favor of a mechanic fix instead:
     the user wanted Crossing's own `K "The Ferry Keeper"` (flavor-only
     since Milestone 93) to actually grant passage.

     First shipped pointing `BOAT K` at `high_clerist_tower` directly --
     wrong once examined, since a strait ferry teleporting the player to
     an inland mountain fortress many tiles further north never made
     geographic sense. The user's own first suggestion for a better
     target, Caergoth, turned out on direct pixel-measurement (labeled
     grid-overlay crop against `References/dragonlancemap2.png`) to sit
     on the Straits of Algoni -- a different body of water entirely,
     well northwest of the Strait of Schallsea, ruled out on geography
     alone. The map itself names the real port town directly across the
     strait, already noted (but never given a real `LOCATION`) in
     Milestone 93's own research: Port O'Call. Added as a brand-new
     **`LOCATION port_ocall`** (`data/locations.txt`, `POS 202 180`,
     pixel-measured and terrain-confirmed walkable) with its own minimal
     zone, `data/zones/port_ocall.txt` (`D "The Dockmaster"`, granting the
     return leg). Final state: `BOAT K port_ocall 3` / `BOAT D crossing
     3` -- a real two-way ferry, 3 hours each way, down from the original
     24 now that it lands at the actual coastal spot instead of
     teleporting inland. `high_clerist_tower`/`palanthas` stay
     foot-reachable north from there (confirmed via a throwaway BFS, 93
     steps, no water crossed). The stray terrain tile at `(200, 182)`
     stays deliberately unfixed.

     **A durable principle the user stated directly, now written into
     `CLAUDE.md`'s "Restraint over completeness" bullet**: this project's
     sourcing-restraint rule governs the tracked Companions' documented
     timeline, not the player character's own overworld geography. A
     `LOCATION` can be added purely because it's real on the reference
     map and useful for the player's own free movement -- Crossing itself
     already cleared this bar at Milestone 93; Port O'Call is the second.

     Separately, the same session added a real sea route out of Flotsam:
     `data/zones/flotsam.txt`'s new `N "A Northbound Trader"`, granting
     `BOAT N kalaman 72`. Unlike Port O'Call, this one *is* directly
     novel-sourced -- `dwn_full.txt:944-948` states the historical
     Perechon was actually "heading for Kalaman, northwest of Flotsam,
     around the cape of Nordmaar" before the storm blew it into the Blood
     Sea of Istar and wrecked it. Deliberately a new POI rather than
     routed through the existing `H "The Harbor"` dockhand, whose
     `TALK`/`TALK_AGAIN`/`TOPIC` lines are all built around hyping the
     doomed Perechon and warning the storm makes it "a bad time to be
     shopping for a ship" -- granting a working, successful voyage through
     him would contradict his own already-shipped flavor (same call as
     Tarsis's Runner avoiding the already-pessimistic Old Sailor,
     Milestone 91). The Blood Sea itself stays uncrossable, consistent
     with `world::terrainFor`'s existing "no lore invented about ships
     crossing it" comment.

     **A real bug found by playing the new ferry, not by review: `BOAT`
     was one-time-per-NPC ever**, gated on `GameState::voyagesTaken`.
     Fine for the four original one-way "story advances" legs (Tarsis ->
     Ice Wall -> Southern Ergoth -> Sancrist -> Palanthas), broken the
     moment a genuine round-trip ferry existed -- Serath boarded, walked
     back to Crossing, and the Ferry Keeper had nothing left to offer.
     Fixed in `GameLoop::talkTo` (`src/game/GameLoop.cpp`): the offer no
     longer checks `voyagesTaken` at all (still populated on boarding,
     still saved/loaded, kept purely as a historical record). Re-offering
     doesn't reintroduce Milestone 88's "sail anywhere" problem -- still
     one narrow, specific point-to-point jump per NPC. Same pass also
     fixed the boarding log message, previously hardcoded to "days pass
     ... rises out of the fog" regardless of trip length -- wrong for a
     3-hour strait hop. Now branches on `candidate.boatHours < 24` for a
     short-crossing phrasing. This was the one real C++ change in this
     milestone; everything else was pure data. Clean `/W4` rebuild (zero
     new warnings).

     Verified via the piped save-slot smoke test (every new
     `LOCATION`/`BOAT` destination resolves cleanly) and, for the boat
     mechanics, by the user's own real play on Serath: boarded Crossing's
     ferry, walked back, boarded it again successfully post-fix. Full
     writeup: `docs/ZONE_NOTES.md`'s "Boats" section, `docs/MAP_NOTES.md`'s
     new "Port O'Call" section.

123. `wayreth_summons` redone -- user's direct complaint: the Test of High
     Sorcery, this project's biggest Mage-only milestone, was mechanically
     just one `VISIT palanthas` objective. "It can't just be a standard
     fetch quest."

     Re-checked `.research/dla_full.txt` (OCR of *Dragonlance Adventures*
     pp.34-35, "The Test of High Sorcery" -- already this quest's own
     citation, but not fully used): the book lists five real guidelines
     for what a Test should contain, including "at least one solo combat
     against an opponent who is two levels higher than the initiate" and
     the framing line "no one who comes is guaranteed of returning alive."
     Milestone 103's three outcome passages (`GameLoop.cpp`'s
     `REWARD_WAYRETH_ROBE` switch, left as-is by this first pass, revised
     by the second pass below) already narrate two of the other
     guidelines (repeated trials, a combat-against-an-ally beat). The
     solo-combat guideline was the one with zero representation,
     mechanical or narrative -- the gap this first pass closes.

     Added a second objective, `SLAY wight 1`, to the existing
     `data/quests.txt` block -- the same "mix objective kinds in one
     quest" pattern `named_in_fact` (VISIT + SLAY baaz) and
     `measure_of_roses` (VISIT + SLAY ogre) already established, not a new
     quest-engine stage system (a real per-trial sequence was considered
     and rejected -- `docs/QUEST_NOTES.md` has repeatedly and deliberately
     deferred a stage-index system, and one extra objective already closes
     the gap). `wight` (`data/monsters.txt`) was unused by any quest
     before this, carries no `TERRAIN_BIAS` (fits "the trial finds you"
     over "go stand in a specific biome," the same lore this quest's own
     offer text already leans on), and at 1,400 XP is markedly tougher
     than every other SLAY target in the roster except Troll/Ettin --
     deliberately the hardest solo fight offered so far, matching the
     source's "no one who comes is guaranteed of returning alive" framing
     (softened in practice by this project's standing "knocked out, not
     killed" combat rule). `OFFER`/`PROGRESS`/`COMPLETE` text was lightly
     rewritten to foreshadow and then acknowledge the fight, framing the
     Wight explicitly as the Test's own illusion/manifestation rather than
     a literal undead sentry guarding Wayreth, matching the existing
     outcome passages' own repeated "illusion" language. Reward raised
     from XP-only (150) to `REWARD_STEEL 75`/`REWARD_XP 250`, roughly
     matching/exceeding `measure_of_roses`'s 100/250 tier. `ACCEPT`'s core
     text, `REQUIRE wayreth_eligible`, `VISIT palanthas`, and
     `REWARD_WAYRETH_ROBE` are all unchanged. Also lightly touched
     `README.md`'s one-sentence mention of this quest so it doesn't
     undersell it.

     That first pass was pure content: zero `.cpp`/`.h` changes, zero new
     `REQUIRE`/`REWARD` flags, zero save-format changes. Verified via a
     clean `/W4` rebuild and the piped smoke test, which loads
     `QuestCatalog` at real program startup, proving `QuestLoader`
     accepts the new `SLAY` line and edited text cleanly.

     **Redone again, same session, after further pushback.** The user
     pointed out the deeper problem directly: winning the Wight fight via
     ordinary combat RNG isn't what the book is actually testing, and
     asked what should happen if the player "falls" -- it shouldn't be a
     dice roll, and (their own proposed fix) a character's Robe should be
     able to shift color based on an actual choice, "and vice versa."
     Re-researched properly this time, since the first pass leaned on
     `.research/dla_full.txt`'s column-garbled OCR alone:
     `.research/dla_layout.txt` (~line 2094) plus the fuller prose in
     *Players Guide to the Dragonlance Campaign* (`References/pg.txt:
     5417-5447`) both say the Test explicitly does **not** grade a
     declared alignment -- "less interested in the applicant's
     alignment... than whether he will use the power of magic in a
     responsible manner" (PG) -- yet each Robe's own "Minimum
     Requirements" in DLA is a check on conduct *during* the Test:
     passed "without having committed an act contrary to the laws of"
     good/neutrality/evil, respectively. So the original per-guideline
     summary above undersold it: Milestone 103's passages narrated
     repeated trials and an ally-combat beat, but the Robe itself still
     came from `character::robeForAlignment(state_.character.alignment)`
     -- a stat frozen at character creation, never what the character
     actually did. Both books also state, twice between them, that
     "failure means death" -- real, but incompatible with this project's
     unbroken "knocked out, not killed" invariant, so permadeath was
     never on the table; "failure" needed a different meaning here.

     Extended `GameLoop::offerOrTurnInQuest`'s `q->rewardWayrethRobe`
     block (`GameLoop.cpp`) rather than inventing a generic mechanic --
     same "named, specific, compile-time flag" shape this reward has
     always used. Once the Wight falls, it reshapes into "a face you'd
     trust with your back turned" (a new `drawDialogueFrame` scene,
     folding in DLA's still-otherwise-unaddressed "combat against a
     known ally" guideline for free) and poses a real three-option
     `drawPickerFrame` choice -- the same primitive Accept/Decline and
     the topic menus already use, in-fiction options only, no mechanical
     labels. The choice maps to a new `game::EthicChoice` enum
     (`GameLoop.h`, next to `conditionMatches`) and a new free function,
     `game::withEthic`, that swaps only the Good/Neutral/Evil axis of
     `character::Alignment` while preserving Lawful/Neutral/Chaotic --
     exact index arithmetic over the enum's own declaration order, no
     lookup table. **Per the user's explicit decision** (asked directly
     via a real fork): the choice overwrites `state_.character.alignment`
     itself, not just the Robe -- the first alignment mutation anywhere
     in this codebase after character creation. Every other `good`/`evil`
     `REQUIRE` already reads `c.alignment` live, so every other
     alignment-gated quest and NPC line picks this up automatically, the
     same "objectives are live queries over existing state" principle
     this project leans on everywhere else. The three outcome passages
     were lightly re-threaded (not rewritten -- nearly all of Milestone
     103's original wording survives) to read as one continuous scene
     instead of three independent vignettes.

     No changes to `data/quests.txt`, `wayreth_eligible`, the Robed
     Stranger POI, `character::robeForAlignment`, or the save format
     (`ALIGNMENT`/`ROBE` were already-persisted; this adds a second
     runtime mutation site for one of them). Verified via a clean `/W4`
     rebuild (zero new warnings) and the piped smoke test; `withEthic`'s
     arithmetic was checked by hand across all nine `Alignment` values
     rather than a dedicated self-test target -- `conditionMatches` and
     its `GameLoop.cpp` neighbors have never been isolated into one in
     this project's history, since that file's own dependency graph
     (world/combat/quest/render, all of it) makes a truly minimal
     throwaway target impractical; this class of logic gets verified by
     rebuild + direct read-through + interactive play instead, same as
     always. `save3.txt` (Serath) already has this quest `Complete`
     under the old shape either way and can't retest any of this. The
     picker, the setup scene, and the alignment-shift log line have no
     piped-testable surface at all (quest turn-in is well past character
     creation, where `_getch()` stops being pipeable) -- seeing any of it
     needs a fresh level-3+ Mage and the user's own keyboard, more
     acutely than the usual quest-milestone caveat. Full writeup:
     `docs/QUEST_NOTES.md`'s "Shipped quests" (extended in place) and
     `docs/CHARACTER_NOTES.md`'s "The Wayreth quest".

124. A weapons/armor shop and a magic shop in every town -- requested
     directly by the user. Before this, shop coverage was uneven: five of
     nine `TOWN`-flagged locations had a shop at all, and none of the six
     existing catalogs (`general`/`armory`/`market`/`salvage`/`bazaar`/
     `harbor`) ran a dedicated arcane-goods storefront -- magic weapons
     were bundled into whichever mundane catalog happened to include one.

     Added a seventh catalog, `character::ShopCatalog::Magic`
     (`Equipment.h`/`.cpp`'s `kMagicGoods` -- no armor/shield/weapon-upgrade
     slots at all, but every consumable/enchanted one: magic weapon,
     potion, Webnet, Brooch of Imog), plus `"magic"` to `ZoneLoader`'s
     `kValidCatalogs` and `GameLoop::shopCatalogFor`'s string->enum
     mapping -- the exact same three-file pattern every earlier catalog
     added. Real behavior change, called out rather than hidden: Webnet/
     Brooch stop being Solace-General-Store-exclusive, since every new
     `magic` shop carries them too (still buyable-only-by-Mage).

     Then closed the coverage gap town by town, reusing catalogs that
     already qualified (Solace's `general` already covers both roles at
     once -- no new POI there at all; Tarsis's `salvage` already covers
     magic; Kalaman's `bazaar` already covers weapons/armor) and adding
     only what was missing elsewhere: new `armory`-catalog POIs (Haven's
     Farrier's Forge, Tarsis's Scrap-Iron Forge, Palanthas's Garrison
     Armorer, Port O'Call's Netmender's Forge, Port Balifor's Smuggler's
     Stall) and new `magic`-catalog POIs (Haven's Relic Peddler's Cart,
     Kalaman's Curiosities Cart, Port O'Call's Beachcomber's Stall,
     Crossing's Waiting Merchant), all following the established
     "`SHOP` needs no `TALK` line" precedent (Palanthas's Harbor,
     Kalaman's Market Square) -- description only, no new dialogue tree.

     **Confirmed with the user directly**: this includes Crossing, Port
     Balifor, and Flotsam, all three previously documented
     (`docs/CHARACTER_NOTES.md`) as *deliberately* shopless (a two-POI
     ferry waypoint; a draconian-guarded harbor; smugglers who "ask no
     questions"). Rather than overturning that reasoning, each town's new
     shop(s) lean into it as black-market/smuggler commerce: Crossing's
     already-existing Quay POI picked up `SHOP Q armory` (a ferry-dock
     chandler, no new POI needed); Port Balifor's already-existing Pig &
     Whistle picked up `SHOP W magic` (the tavern that once hosted a
     red-robed illusionist's nightly show is a natural spot for small
     enchanted trinkets to change hands); Flotsam needed zero new content
     at all -- its existing Back Alley ("something ugly happened here")
     became `SHOP A armory` and its existing Saltbreeze Inn ("the
     ordinary rules of the town quietly stop applying") became `SHOP S
     magic`, both POIs whose flavor text was already describing exactly
     this kind of under-the-table trade.

     Net result: 13 shop POIs across all 9 towns (up from 6 across 5),
     every town covering both roles. Pure data + one small, mechanical
     enum/switch addition -- no new engine mechanism, same shape every
     earlier catalog used. Clean `/W4` rebuild (zero new warnings).
     Verified via the piped save-slot smoke test, which loads
     `ZoneCatalog::loadForWorld` (and therefore all nine hand-edited
     `GRID` blocks) before the save-slot menu even renders -- reaching
     that menu confirms every row-width and POI-reference check passed.
     All three save slots were occupied this session, so the test
     couldn't run past that menu into character creation; live
     shop-browsing/buying at the new and reused POIs still needs the
     user's own keyboard. Full writeup: `docs/ZONE_NOTES.md`'s "Shops:
     POIs you can buy from" and `docs/CHARACTER_NOTES.md`'s "Shops and
     catalogs" (renamed from "Six shops, six catalogs").

125. Finished part of Milestone 113's own leftover work: sourced `GROUP`
     data for 3 of the 12 monsters it left solo. Re-verified all 12
     candidates' real Monstrous Manual/Dragonlance Adventures "No.
     Appearing" field directly against rendered rulebook page images
     (Monster Manual pp.135, 272, 284, 326, 349, 360; Dragonlance
     Adventures pp.73-76, 78) rather than trusting Milestone 113's own
     citations at face value -- every figure matched, but two turned up a
     finding worth acting on rather than just re-confirming a number:
     Owlbear's real entry prints "1 (2-8)" and Ettin's prints "1 or 1-4"
     -- in both cases the *wandering* encounter (the only kind this
     project models; there's no lair concept) is a real, sourced 1, with
     the larger figure explicitly a lair-only or rare-gathered-band
     exception in the book's own prose. Grouping either would misread the
     source, so both move from "deferred" to permanently solo, a real
     finding not just a re-statement of Milestone 113's own deferral.

     That left 10 genuinely groupable monsters, but 7 (Ogre, Kapak,
     Bozak, Sivak, Aurak, Wight, Troll) are `MIN_TOWN_DISTANCE`-gated
     specifically for being too dangerous solo -- grouping them without
     re-tuning those gates risks the same untested difficulty spike
     Milestone 113 already declined to ship. **Asked the user directly**:
     keep that 7-monster tier solo for now, and ship only the 3 monsters
     that carry no distance/danger gate at all -- Thanoi, Ice Bear, and
     Giant Spider, each getting a new sourced `GROUP 1 4` line in
     `data/monsters.txt` (Thanoi real 1-20 clamped, Ice Bear real 1-4
     already under the cap and used unclamped like Black Bear's own line,
     Giant Spider real 1-8 clamped).

     Flagged rather than smoothed over: "no distance gate" isn't the same
     as "low danger" for two of these three. Thanoi (HD4, Ogre-comparable)
     and Ice Bear (HD6+2, XP707, Sivak-tier-comparable) only lack
     `MIN_TOWN_DISTANCE` because their `ONLY_TERRAIN` glacier lock already
     keeps them off a starting town's doorstep by a different mechanism --
     grouping them reopens a sliver of the same stacking-danger concern
     the 7-monster tier was just held back for, gated by terrain instead
     of tile distance. Giant Spider's own Milestone 113 exclusion was a
     separate concern: stacking its real, modeled poison bite (a saving
     throw per hit) up to 4-deep. All three shipped anyway this pass, at
     the user's explicit direction after this was flagged.

     Pure `data/monsters.txt` changes -- no C++ changes at all, since the
     `GROUP` grammar and its clamping logic already existed and was
     already exercised by 14 other monsters. Clean `/W4` rebuild (zero new
     warnings). Verified via the piped save-slot smoke test, which loads
     `MonsterCatalog` before the save-slot menu renders -- reaching that
     menu confirms the three edited blocks still parse cleanly. No
     piped-testable path to a live combat encounter exists regardless
     (`_getch()` blocks that) -- seeing an actual Thanoi/Ice Bear/Giant
     Spider band in a real fight still needs the user's own keyboard.
     Full writeup: `docs/COMBAT_NOTES.md`'s "Monster encounter groups,
     continued (Milestone 125)".

126. Atlas Chronology Re-sync -- the user asked whether `TSR 8448 The
     Atlas of the Dragonlance World.pdf` (pp.137-139, a real day-by-day
     chronology for Year 351-352 A.C.) matches the day numbers already
     shipped in `data/timeline.txt`. It didn't: every window past
     `pax_tharkas 10 12` had been an explicit, disclosed *guess* from
     vague textual cues, and the Atlas's real dates showed one of those
     guesses was off by a lot -- "roughly 2-3 months" between Pax Tharkas
     and Sturm's death was actually closer to 5 months, almost all of it
     between Sancrist Isle and the Tower siege.

     Re-derived every `PRESENCE`/`SUBJECT_WHEN` day-boundary in the file
     from the Atlas's own dates (Day 0 anchored to the existing `solace 0
     1`, 30-day months evidenced directly by the Atlas's own "9.30 -- Last
     day of autumn" and the fact no date in the chronology exceeds `.30`),
     at the user's explicit choice of the Atlas's real absolute day counts
     over a compressed/proportional rescale -- accepting that the tracked
     schedule now spans ~195 days instead of ~107. Confirmed first, via
     grep, that this is a pure data + doc change: no `.cpp`/`.h` file
     hardcodes any of the day numbers being moved.

     Applied via a throwaway, uncommitted Python script performing
     count-verified literal substring replacements (every occurrence of a
     given old value maps to exactly one new value everywhere in the
     file, confirmed by grep beforehand) rather than dozens of manual
     edits across 12 `CHARACTER` blocks. One real collision surfaced and
     was fixed: Flint and Tasslehoff each carry two `PRESENCE kalaman`
     windows, and the Atlas's literal dates would have put them one day
     apart -- `Timeline::presentAt` has no defined behavior for two
     overlapping windows at the same location for the same character, so
     the second window was pushed out to a clean, non-overlapping day
     instead, disclosed as an engineering adjustment, not an Atlas date.

     Found, but deliberately not built this pass: six real, Atlas-named
     waypoints the game still has no `LOCATION` for -- Que-shu, Hopeful
     Vale, Skullcap, Qualimori, Dragon Mountain, and Mount Nevermind (see
     NEXT UP). Adding any of them is a map-placement/zone-building effort,
     not a day-renumbering one.

     Verified via `--check-timeline`: zero *new* keyword-collision
     warnings despite several windows widening substantially (`ice_wall`
     5 days to 10, `silvanesti` 6 days to 13) -- the only warning that
     fires is the one pre-existing, already-documented Raistlin/Kitiara
     override. Clean `/W4` rebuild, zero new warnings, no `.cpp`/`.h`
     diff at all. Full writeup, including the complete before/after
     tables for both `PRESENCE` and `SUBJECT_WHEN`: `docs/TIMELINE_NOTES.md`'s
     "Atlas Chronology Re-sync (Milestone 126)".

127. Foghaven Vale (Huma's Tomb / the Dragon Mountain) -- requested after
     reviewing the world map against a newly-supplied reference,
     `References/TSR 9400 TM3 World Of Krynn Trailmap.pdf` (present in
     `References/` since 2011, never previously cross-referenced by any
     doc). That review pointed at two of Milestone 126's six unbuilt
     Atlas-named waypoints, Skullcap and Dragon Mountain; the user picked
     both.

     Direct research against the three tracked Chronicles novels
     (`dat_full.txt`/`dwn_full.txt`/`dosd_full.txt`) found Skullcap
     doesn't actually belong on that list -- it never appears in any of
     them, only in the *Legends* trilogy, as the ruin of Zhaman from the
     Dwarfgate War. The "map-to-Thorbardin's-door sidequest" description
     this project had attached to it was never sourced. Per the user's
     decision: dropped outright, not built, with the doc corrected rather
     than carried forward -- see `docs/TIMELINE_NOTES.md`'s "Foghaven
     Vale" section for the correction and its exact wording.

     Dragon Mountain checked out, and richer than expected:
     `dwn_full.txt` lines ~8330-9650 give the full Foghaven Vale/Huma's
     Tomb/Dragon Mountain scene, involving three tracked Heroes (Laurana,
     Flint, Tasslehoff) plus Gilthanas, Theros Ironfeld, Silvara (already
     a full `CHARACTER` since Milestone 95), and Fizban (already a full
     `CHARACTER`). It's also the destination Tasslehoff's own
     already-shipped `southern_ergoth` dialogue teases and never pays
     off -- "I'm fairly sure he asked me to come somewhere with him, and
     I said yes... I THINK I'm still invited!" -- this milestone is that
     payoff.

     **No new overworld `LOCATION`.** Same "no matching `LOCATION` of its
     own" pattern `data/zones/solace_inn.txt` established at Milestone
     16 -- Foghaven Vale is a secret place reached on foot from within
     Southern Ergoth, not a separately-walkable overworld tile, so no
     `ROAD_PAIRS`/`overworld.grid` regeneration was needed at all. A new
     `PORTAL M foghaven_vale` POI was added to `data/zones/
     southern_ergoth.txt`; the new `data/zones/foghaven_vale.txt` (three
     POIs: the Guardians, Huma's Tomb as the `TIMELINE_ANCHOR`, the
     Dragon's Throat) deliberately carries no `TIMELINE_LOCATION`
     override, so its effective timeline-location key defaults to its
     own zone id rather than borrowing Southern Ergoth's -- confirmed by
     reading `GameLoop.cpp`/`ZoneLoader.cpp`/`Zone.h`/`TimelineLoader.cpp`
     first, since a `PRESENCE` location-id is never cross-validated
     against `data/locations.txt`. Keeps the new content fully isolated
     from Southern Ergoth's own already-shipped `southern_ergoth 69 72`
     lines.

     Five `CHARACTER` blocks gained a new `PRESENCE foghaven_vale 69 72`
     window: Laurana, Flint, and Tasslehoff (new tracked-Hero content),
     plus a second window each for Fizban and Silvara, distinct from
     their existing `southern_ergoth 69 72` ones (different specific
     scenes from the same book stretch, not duplicates -- see
     `docs/TIMELINE_NOTES.md`'s "Foghaven Vale" section). Sturm gets
     nothing new -- he and Derek Crownguard split off toward the Knights'
     outpost before this scene, confirmed by the source text rather than
     assumed. Gilthanas and Theros Ironfeld stay off-stage, same
     precedent Southern Ergoth already set for named-but-untracked
     figures.

     Pure data change, no `.cpp`/`.h` touched. Verified via a clean
     `/W4` rebuild (zero new warnings, no diff) and `--check-timeline`
     (zero new keyword-collision warnings beyond the one pre-existing,
     already-documented Raistlin/Kitiara override). **Not interactively
     walked** -- reaching the new zone/portal/anchor requires
     `_getch()`-driven movement that can't be piped, same standing
     limitation this project always discloses for zone-interior content;
     the piped character-creation smoke test only confirms the new data
     files parse without throwing. Full writeup: `docs/TIMELINE_NOTES.md`'s
     "Foghaven Vale" section and `docs/ZONE_NOTES.md`'s "Foghaven Vale"
     section.

128. Terrain accuracy pass -- while mocking up a possible new "World Map"
     screen (zoomed-out travel view, not built this milestone -- see NEXT
     UP), the user asked to verify plains/forest/mountain/snow terrain
     against two references already in `References/` but never checked
     against the pipeline: the TSR 9400 Trailmap and TSR 8448 Atlas (both
     scanned, read by rendering pages to images). Found and fixed three
     real bugs in `data/overworld.grid` itself, independent of the World
     Map screen's fate: **zero grassland tiles anywhere on the continent**
     (`tools/generate_overworld.py`'s color classification mapped the
     relevant buckets entirely to `savannah`, which costs 2x grassland's
     movement time -- every off-road trip across open country had silently
     cost double since Milestone 2); a **Blood Sea over-classification
     leak** the Milestone 85 note already flagged but undersold ("a couple
     of stray dots... harmless") -- a flood-fill found 64 stray tiles, one
     cluster of 27 sitting on the route from Tarsis to Ice Wall Castle, not
     somewhere harmless; and **bog placed near Xak Tsaroth for the first
     time**, sourced to the Atlas's explicit "swamp" description of the
     approach, backing up flavor text this project already asserted but
     never actually placed as terrain. Fixed in `tools/generate_overworld.py`
     (an `INDEX_TO_TERRAIN` relabel plus new `MANUAL_TERRAIN_OVERRIDES`
     entries, each cross-checked against the two new references) and
     regenerated. Pure data change -- clean `/W4` rebuild, **zero
     `.cpp`/`.h` diff**, piped character-creation smoke test passed (real
     saves moved aside, restored after). Full writeup, sourcing, and every
     fixed coordinate: `docs/MAP_NOTES.md`'s "Terrain accuracy pass"
     section.

129. Bigger combat battlefield -- the user asked for the tactical combat
     grid to be "a bit bigger, like the SSI Gold Box games." Bumped
     `render::MapRenderer::kCombatGridWidth`/`kCombatGridHeight` from
     11x7 (77 cells) to 15x9 (135 cells) -- both constants
     `GameLoop::runCombat` already read for every placement/movement/
     bounds check, so no other code changed. Confirmed the new 47-column
     grid row (15 cells * 3 columns + a 2-column border) stays well under
     `kProseWrapWidth`'s floor of 71 even on the smallest supported
     console, so it can never wrap mid-row; width stays odd, preserving
     the player-centered symmetric monster-spawn invariant. Pure
     rendering-constant change -- clean `/W4` rebuild, zero new warnings,
     and the piped character-creation smoke test (real saves moved aside,
     restored after) confirms nothing else broke. **Not interactively
     played** -- `_getch()` can't be piped, so the actual in-game combat
     screen at the new size (roster layout, target-picker brackets,
     monster spread) hasn't been read back by a human yet; worth doing on
     the next fight. See `docs/COMBAT_NOTES.md`'s "Positional combat
     grid" section for the full sizing math.

130. Shallow water fixed -- the user flagged two problems with `r`
     ("shallow water") at once: it was walkable, and there was far too
     much of it (17,824 of 153,600 tiles, 11.6% -- more than mountains or
     glacier). A distance-transform BFS showed why: only ~26.5% of it sat
     within 2 tiles of real land; the rest, trailing off to 66 tiles from
     any shore, was open bay/strait interior (Good Bay, New Bay, Blood
     Bay, the Strait of Schallsea) the 32-color quantization happened to
     bucket the same as narrow coastal shallows. Fixed both at the
     generator level: `tools/generate_overworld.py` gained
     `shrink_river_to_coastal_fringe()`, a BFS-based pass reclassifying
     any `river` tile farther than 2 tiles from land to `ocean` (`r`
     dropped to 4,708 tiles, 3.1%); `Terrain.cpp`'s `r` entry flipped
     `passable` to `false`, matching ocean. Re-verified with the same
     throwaway-BFS method Milestone 87 established: no location's own
     `POS` tile is coded `r` (nothing needed nudging), and only
     `sancrist_isle`/`southern_ergoth` lose foot-reachability from
     Solace -- both already boat-only by design, so not a regression.
     `README.md`'s Status paragraph line about Crossing's water being
     "shallow enough to wade" was rewritten to match. Pure data +
     one-flag change -- clean `/W4` rebuild, zero unrelated `.cpp`/`.h`
     diff, piped character-creation smoke test passed (real saves moved
     aside, restored after). **Interactively confirmed working** by the
     user on real save data -- shallow water blocks movement and the
     shrunken coastline reads correctly in a real terminal. Full writeup,
     stats, and the one pre-road-classification false alarm found while
     re-auditing `ROAD_PAIRS`: `docs/MAP_NOTES.md`'s "Shallow water pass"
     section.

131. Four Atlas waypoints -- Que-shu, Hopeful Vale, Qualimori, Mount
     Nevermind -- closing out NEXT UP item 7, the user's pick from this
     session's backlog menu. Researched directly from `TSR 8448 The Atlas
     of the Dragonlance World.pdf` (read page-by-page as images -- it has
     no extractable text) plus `.research/dat_full.txt`/`dwn_full.txt` and
     a full-resolution crop of `References/dragonlancemap2.png`.

     **Que-shu** and **Hopeful Vale** are real overworld `LOCATION`s
     (`("solace", "que_shu")` and `("hopeful_vale", "thorbardin")` added to
     `ROAD_PAIRS`, both clean straight lines, zero impassable crossings).
     **Qualimori** and **Mount Nevermind** are directly labeled on the
     reference map but deliberately get no `LOCATION`/`POS` at all --
     their straight line to the nearest existing location crosses several
     tiles of real open bay, so both were built as `PORTAL`-only zones off
     `southern_ergoth.txt`/`sancrist_isle.txt` instead, the same pattern
     Milestone 127 used for Foghaven Vale. See `docs/MAP_NOTES.md`'s
     matching section for the full placement writeup and the water-
     crossing checks behind that call.

     A real chronology correction came out of the Hopeful Vale research:
     Milestone 126's re-dating pass had folded the Goldmoon/Riverwind
     wedding into the existing `thorbardin 33 41` window, but the Atlas's
     own day-by-day Pathways list puts it 16 days earlier and 13 days
     long (day 17-29) -- closing the day-16-to-32 gap that same session's
     own notes had flagged as a byproduct of the correction. Full
     chronology writeup: `docs/TIMELINE_NOTES.md`'s matching section.

     25 new `PRESENCE` blocks across 10 already-tracked characters (no new
     `CHARACTER`s, no `SUBJECT` lines touched): all 8 core Heroes at
     Que-shu (day 2, the on-page ruin scene) and Hopeful Vale (day 17-29,
     the wedding) plus Laurana at Hopeful Vale; Sturm/Flint/Tasslehoff/
     Fizban/Laurana/Silvara at Qualimori (day 70, a brief overnight stop);
     Tasslehoff and Fizban at Mount Nevermind (day 104 -- genuinely
     off-page in the source text, but a real, dated visit, not a
     retrospective-only device).

     Pure data change -- clean `/W4` rebuild, zero new warnings, no
     `.cpp`/`.h` diff. `--check-timeline` produced zero new
     keyword-collision warnings (only the one pre-existing, already-
     documented Raistlin/Kitiara override fires, same as every prior
     content milestone). Piped character-creation smoke test passed (real
     `save1-3.txt` moved aside, restored after) -- caught and fixed a real
     bug first (three hand-authored zone `GRID` rows one character too
     wide/narrow, `world::ZoneLoader`'s fail-fast width check caught it
     immediately). **Not interactively walked** -- same standing
     `_getch()` limitation this project always discloses for zone-interior
     content. Full writeups: `docs/MAP_NOTES.md`, `docs/TIMELINE_NOTES.md`,
     and `docs/ZONE_NOTES.md`, each with their own matching section.

132. Astinus of Palanthas, rebuilt closer to source -- the user asked for
     him to be able to answer *anything* about Krynn's past and present
     rather than a curated topic menu, to cut a conversation short after
     about five questions (annoyed at being kept from his work), and to
     react to future-events questions with curiosity rather than his usual
     brush-off. Researched via a fresh `pdftotext -layout` extraction of
     `TSR 2021 DragonLance Adventures.pdf` (pp.88-89's Astinus NPC writeup)
     and `Time_of_the_Twins_-_Margaret_Weis.pdf` (the Raistlin/Crysania/
     Astinus chapters), plus DLA's Solamnia and Silvanesti history sections
     for the new lore `SUBJECT`s.

     Dropped both `TOPIC` lines outright (both were already near-duplicated
     by existing `SUBJECT` entries, so nothing was lost) and grew his
     `SUBJECT` pool from 4 to 12: the Cataclysm, the gods (deflecting DLA's
     own "rumored to be Gilean" line rather than confirming or denying it),
     Knights of Solamnia, the Towers of High Sorcery, Huma and the First
     Dragonlance, draconians, the Qualinesti/Silvanesti split, and a
     dedicated future-events response distinct from the generic
     `SUBJECT_UNKNOWN` brush-off. Deliberately left out dwarves/Thorbardin
     and kender -- no confirming passage was pulled this session (the Atlas
     of the Dragonlance World PDF is image-only, no extractable text), and
     other NPCs already carry that flavor elsewhere.

     New engine-level grammar: `ASK_LIMIT <char> <n>` /
     `ASK_LIMIT_REACHED <char> <dialogue...>`, a generic, reusable
     "cap free-text questions per in-game day" pair (`world::Zone`/
     `world::ZoneLoader`, same "must already have a TALK line, fails fast
     if the two don't appear together" validation as TOPIC/SUBJECT), with
     the persistent day/count state living as named fields on
     `character::Character` (`lastAstinusAskDay`/`astinusQuestionsToday`),
     the same `hoursElapsed / 24` once-per-day convention and backward-
     compatible save format (`ASTINUSASKDAY`/`ASTINUSASKCOUNT`) as
     `lastRestDay`/`lastBroochUseDay`/`lastStaffCureDay`. `GameLoop::talkTo`
     folds the day-gate into the existing "Ask about something else..."
     menu option rather than adding new UI. Astinus is the first (and so
     far only) POI to use it; the cutoff line -- sourced from DLA's "he may
     be annoyed at the interruption" and *Time of the Twins*' "his pen
     ceased its eternal scratching" -- is the one moment in the whole file
     where the pen, otherwise described as never stopping, actually stops.

     Verified via a throwaway self-test (`ZoneLoaderSelfTest.cpp`, a
     minimal `world::Zone`/`ZoneLoader`/`ZoneTile`-only CMake target, same
     shape Milestone 82 used) confirming `palanthas.txt`'s POI `L` parses
     with `askLimit == 5`, 12 subjects, 0 topics, and that both orphan
     cases (`ASK_LIMIT` with no `ASK_LIMIT_REACHED` and vice versa) fail
     fast as designed. Clean `/W4` rebuild, zero new warnings. Piped
     character-creation smoke test passed (real `save1-3.txt` moved aside,
     restored after); a second piped run against an existing real save
     (predating the new `ASTINUSASKDAY`/`ASTINUSASKCOUNT` lines) reached
     the live game loop without error, confirming the backward-compatible
     load path. **Not interactively walked** -- same standing `_getch()`
     limitation this project always discloses; the five-question cutoff,
     the pen-stops beat, and the next-day reset all still need a real
     keyboard playthrough. Full writeup: `docs/ZONE_NOTES.md`'s "Ask about
     anything" and "Palanthas" sections.

133. Astinus follow-up: genuinely comprehensive world knowledge, an
     Intelligence+Wisdom check to buy more patience, and a harder refusal
     on identity questions -- three direct user requests in one session.
     Grew his `SUBJECT` pool from 12 to 47: the 7 remaining Heroes of the
     Lance, Kitiara, Verminaard, a broad war/dragonarmies overview, and one
     entry per remaining `data/locations.txt` `LOCATION` (24 places),
     reusing each location's own already-sourced `DESC` line reframed in
     his voice rather than re-deriving new research. Caught and fixed one
     real mistake in the process: a first draft of the High Clerist's
     Tower entry had a Knight reopening its sealed inner sanctum, which
     contradicts `docs/ZONE_NOTES.md`'s own "described, not modeled"
     precedent for that door -- fixed before it shipped. Also fixed a
     keyword collision the new place entries would have caused (Kinslayer
     War's own `qualinesti,silvanesti` keywords vs. the new standalone
     place entries of the same names) and, while implementing the identity
     check below, a latent ordering bug in Milestone 132's own
     self-identity entry (bare `who` was intercepting "who is `<anyone>`"
     questions before they ever reached their real answer).

     New engine-level grammar: `ASK_LIMIT` gained an optional second
     number (a hard cap) plus `ASK_LIMIT_EXTENDED <char> <dialogue...>`,
     required whenever that cap exceeds the soft one (`ASK_LIMIT L 5 10`
     for Astinus). The moment the soft limit is first reached,
     `GameLoop::talkTo` rolls the real PHB proficiency-check formula
     (Player's Handbook, revised, re-extracted this session) twice --
     Intelligence, then Wisdom, both must succeed -- a homebrew combination
     of two real checks, not itself a printed 2e mechanic. Success raises
     the day's effective cap (`character::Character::astinusDailyLimit`,
     new, saved via optional `ASTINUSDAILYLIMIT`, same backward-compatible
     shape as `lastRestDay` etc.); failure, or reaching the hard cap itself
     (no further checks ever rolled once there), falls through to the
     existing `ASK_LIMIT_REACHED`.

     Second new grammar: `SUBJECT_ENDS`, identical to `SUBJECT` but ends
     the conversation immediately after its text is shown. Telling "are
     you Gilean" apart from "tell me about Gilean" -- both contain the
     same word -- needed a real, small addition to the matcher itself, not
     just data: a keyword alternative can now be a `+`-joined word group
     (`you+gilean`, `who+you`, ...) that only matches when *every* word in
     it is present, alongside plain single-word keywords (a length-1
     group) which keep matching exactly as before -- `data/timeline.txt`'s
     own, separate `SUBJECT` grammar is untouched by this (no `+` syntax
     there, and `speechFromWindow` wraps its plain keywords into length-1
     groups). Astinus's `SUBJECT_ENDS L you+gilean,you+god,astinus+god,
     astinus+gilean,who+you,identity,really,truly` sits ahead of the plain
     gods/Gilean lore entry so a genuine identity challenge wins the tie.
     The conversation-ending entry is also filtered out of the "Ask about
     something else..." hint list -- meant to be discovered, not
     menu-suggested.

     Two more direct corrections landed in the same session, both from the
     user's own read of the shipped behavior:

     - **No hint list when the pool is meant to feel unbounded.** The
       "Ask about something else..." prompt was still listing all 46
       askable keywords (everything but the identity entry) as a
       suggestion line -- exactly the curated-menu feel the whole point of
       this pass was to move away from. New bare per-POI flag,
       `ASK_ANYTHING <char>` (same "must have a TALK line and at least one
       SUBJECT" family as SUBJECT itself), sets
       `world::PointOfInterest::suppressAskHints` /
       `game::Speech::suppressAskHints`; `GameLoop::talkTo` skips building
       the hint list entirely when set, and `render::MapRenderer::
       drawAskInputFrame` already renders correctly with an empty hint
       vector (no separate rendering change needed). Astinus (`ASK_ANYTHING
       L`) is the first and so far only use.
     - **The identity hard-stop didn't actually stop the day.** Reported
       directly: asking "are you a god" ended that conversation, but
       walking back into the library the same day still offered more
       questions -- `SUBJECT_ENDS` only ever `return`ed from `talkTo`
       without touching `lastAstinusAskDay`/`astinusQuestionsToday`/
       `astinusDailyLimit` at all, so the day-long lockout `ASK_LIMIT`
       running out already enforced correctly never applied to this path.
       Per the user: "be it hard stop or not, when it reaches the limit,
       there can be no audience with Astinus that day." Fixed by having
       the `endsConversation` branch mark the day exhausted the same way
       running out the ordinary count does (`astinusQuestionsToday` set to
       today's effective limit) before returning, whenever the POI has an
       `ASK_LIMIT` configured at all.

     Verified via a throwaway self-test (`ZoneLoaderSelfTest.cpp`, same
     minimal `Zone`/`ZoneLoader`/`ZoneTile`-only CMake target as Milestone
     132's own, run twice -- once for the original pass's checks, once
     more for `ASK_ANYTHING`'s own fail-fast cases): confirmed
     `askLimit`/`askLimitHardCap`/the two new dialogue fields, 47 subjects
     with exactly one `endsConversation` entry, `suppressAskHints == true`
     on Astinus, every fail-fast pairing case (`ASK_LIMIT`/
     `ASK_LIMIT_REACHED`/`ASK_LIMIT_EXTENDED`/`ASK_ANYTHING`, six cases in
     total), and -- via a local reimplementation of the tokenize-then-
     group-match logic, same dependency-avoidance reasoning as Milestone
     82's own self-test -- that "are you gilean"/"are you a god"/"who are
     you" hit the identity entry while "tell me about gilean"/"tell me
     about the gods"/"who is kitiara" correctly don't. Clean `/W4`
     rebuild, zero new warnings. Piped character-creation smoke test
     passed twice (real `save1-3.txt` moved aside, restored after each
     time); a piped run against an existing real save (predating
     `ASTINUSDAILYLIMIT`) reached the live game loop without error.

     A third correction, also from the user: once the day's audience is
     used up, Astinus himself shouldn't be the one turning the player away
     again -- "have an Aesthetic say that no audience will be granted."
     New `ASK_LIMIT_LOCKED <char> "<speaker>" <dialogue...>` is the first
     `PointOfInterest` dialogue field ever attributed to a name other than
     the POI's own (`GameLoop::talkTo` now picks a `speakerName` separate
     from `name`) -- every other field, from `TALK` through every
     `SUBJECT`, speaks as the same character reacting differently, never a
     genuinely different one. Required `askLimitExhausted` to be computed
     up front in `talkTo` (previously only computed where the ask-picker
     itself is built, further down) so this can override the greeting/
     `TALK_AGAIN` choice entirely, one precedence rung below `TALK_BEFORE`
     and above the ordinary `alreadyMet` check -- same "ongoing truth,
     re-checked every visit" shape as `TALK_BEFORE`, not a one-time event
     like `TALK_AFTER`. Astinus's own `ASK_LIMIT_LOCKED L "An Aesthetic"
     ...` intercepts the player in the vestibule, consistent with his own
     `TALK` line's "very few are let past this room" framing. Verified via
     a third throwaway self-test pass (same target, three more checks: the
     speaker/text parse correctly, `ASK_LIMIT_LOCKED` with no `ASK_LIMIT`
     configured throws, an unquoted speaker throws), a third clean `/W4`
     rebuild (zero new warnings), and a third piped character-creation
     smoke test (real saves moved aside, restored after).

     A fourth, smaller gap the user caught by simply trying it: Laurana --
     the Golden General, already a schedule-tracked `data/timeline.txt`
     character with her own established arc in this project (the Lord's
     map room, Whitestone command, the Dargaard Keep captivity) -- had no
     `SUBJECT` entry at all, an oversight in the original Hero/antagonist
     pass. Added (`SUBJECT L laurana`, keyword collision-checked against
     the other 47), bringing the pool to 48. Verified the same way as the
     rest of this milestone's data-only passes: clean `/W4` rebuild (no
     source changed, so no new warnings possible) and a piped character-
     creation smoke test (real saves moved aside, restored after).

     **Interactively confirmed** by the user on 2026-09-01: the
     ability-check roll extended the session to the 10-question hard cap
     and the cutoff there booted them out as designed; a separate attempt
     asking Astinus to confirm/deny he's Gilean ended the conversation
     immediately, same-day; and walking back into the library afterward
     was correctly blocked by an Aesthetic at the door rather than
     reaching Astinus again. Full writeup: `docs/ZONE_NOTES.md`'s "Ask
     about anything" and "Palanthas" sections.

134. World Map screen -- NEXT UP item 8's mocked-up-but-never-built
     zoomed-out continent overview, the user's pick from this session's
     backlog menu. Bound to `'o'`: `GameLoop::showWorldMap()` draws
     `render::MapRenderer::drawWorldMapFrame` once and blocks for a
     single keypress to dismiss, the same shape as `showHelp()`/
     `showSpellbook()` -- not a `GameState.mode`, not a travel/fast-travel
     mechanic (see `docs/ARCHITECTURE.md`'s matching entry for why that's
     deliberate).

     The mockup's importance-tier placeholders were explicitly flagged as
     unsourced; this session did that sourcing without needing fresh
     Atlas page-image research (the Atlas PDF has no extractable text,
     but every location's own `DESC` was already sourced at its own
     placement milestone, and `References/portcities.txt` -- already
     vetted, already used for prior port-town decisions -- covered the
     rest): Palanthas (`portcities.txt`: "the greatest harbor on Krynn"),
     Thorbardin (its own DESC: "a dwarven kingdom carved whole from the
     Kharolis Mountains"), and Tarsis (`.research/dat_full.txt`:
     "the legendary seaport city of Tarsis the Beautiful") got a new
     `SIZE LARGE`; Kalaman ("major port, the anchor of the whole
     northern trade route"), Neraka (its own DESC: "the dragonarmies of
     half a continent are gathered here"), and Port Balifor ("the
     eastern trade hub") got `SIZE MEDIUM`. Everything else, including
     Solace, stays the default/`Small` -- matching the mockup's own
     "Solace deliberately small" framing directly, since small is the
     default rather than a value needing its own sourcing pass.

     A real problem the mockup didn't account for: 11 of the 25
     locations (the whole Abanasinia/Kharolis cluster) sit close enough
     together on the 480x320 grid that any compression ratio fitting a
     console screen collapses them onto a handful of cells, so the
     mockup's planned inline text labels would overlap and garble there.
     Raised with the user directly during planning; resolved with **a
     side legend panel** (glyph + name, alphabetical, every location)
     instead of inline map labels -- the map itself never draws text,
     only glyphs on colored terrain, so it stays legible regardless of
     clustering. Full sourcing, the downsample math (proportional
     box-sampling, majority-vote terrain per cell, a 3:1 column:row
     ratio derived from canceling a terminal's ~2:1-tall character cells
     against the source grid's own 3:2 aspect), and the exact clustering
     numbers: `docs/MAP_NOTES.md`'s "World Map screen" section.

     New optional `SIZE <MEDIUM|LARGE>` line on `LOCATION` blocks
     (`world::MapSize`, default `Small`), parsed by `WorldLoader` the
     same fail-fast way as `TOWN`/`SEA_LOCKED`. `MapRenderer` gained its
     own adaptive sizing (`kWorldMapRows`/`kWorldMapColumns`), solved
     from both the console's height and width budgets the same way
     `configureLayout` already sizes the walking viewport, so the screen
     degrades to a smaller map + a truncated ("+N more") legend on a
     tiny terminal rather than overflowing it.

     Verified: a throwaway `WorldLoaderSelfTest.cpp` confirmed `SIZE
     MEDIUM`/`SIZE LARGE` parse correctly, an omitted `SIZE` line still
     defaults to `Small`, and `SIZE HUGE` throws the expected
     `locations.txt:<line>:` message -- then deleted, along with its
     temporary CMake target, per the standing self-test convention.
     Clean `/W4` rebuild, zero new warnings. Piped character-creation
     smoke test passed (no real save files existed this session to move
     aside). **Interactively confirmed** by the user on 2026-09-01 -- the
     World Map renders correctly, every location legible via the side
     legend.

135. Full-screen presentation -- the user's maximized terminal (~212x60
     character cells) was only rendering a 78x30 frame with the log panel
     capped at 60 columns, leaving a dead black gutter on the right and
     bottom of the window every frame. Root cause confirmed by direct
     code inspection: `MapRenderer::configureLayout` sized the map at
     `kPreferredViewportWidth`/`Height` and the log panel up to
     `kMaxLogPanelWidth`, but never gave the map any of the width/height
     left over once those targets were hit. The user asked for a
     graphics-designer's plan to fill the screen and look more like their
     own `References/cataclysm-dark-days-ahead.avif` reference, with a
     mockup of the end state before any code changed, while leaving game
     mechanics untouched. Confirmed no new rendering engine was needed --
     CDDA's own screenshot is the same terminal-ASCII technology this
     project already ships, just using more of it; the SFML trial from
     the backlog (item 3, still not pursued) stays irrelevant here.

     Three real mockups (Python scripts rendering the project's actual
     `data/overworld.grid`/`data/locations.txt`/`data/zones/solace.txt`
     at the user's real terminal font metrics and the shipped ANSI
     palette) were built and shown to the user before writing any C++,
     resolving three open design questions:
     - **The fix itself**: log panel width is computed first, same
       formula and `[kMinLogPanelWidth, kMaxLogPanelWidth]` clamp as
       before; the map now gets `contentWidth - kLogPanelGap - logWidth`
       -- whatever's left, floored at `kMinViewportWidth` rather than
       ceilinged at `kPreferredViewportWidth`. Map height lost its
       `min(kPreferredViewportHeight, contentHeight)` cap the same way.
       Reproduces the old 120x30-baseline numbers exactly (already "just
       enough room to hit preferred, no more") while giving the user's
       own ~212x60 console a 148x56 map instead of the old 78x30.
     - **Inline location labels** (the user's pick over staying glyph-
       only): `drawOverworldFrame` now builds a `MapCell{glyph, color,
       occupied}` buffer before writing anything out, then overlays each
       visible location's name beside its glyph (2 columns after, or
       immediately before if that doesn't fit), skipping the label
       entirely rather than truncating/overlapping/wrapping if neither
       position is free -- same restraint precedent as Milestone 134's
       side-legend decision, just applied inline since this viewport is
       far less dense than the World Map's 3:1 downsample.
     - **Indoors, the user's pick over a compact centered box or
       re-authoring all 29 zone files bigger**: every authored zone is at
       most 44x16 (`solace_inn.txt`), and `drawZoneFrame` used to
       wall-pad the leftover space with solid `#` glyphs -- at full
       screen size that would have meant a tiny room inside a huge
       fortress of wall texture, a real regression the mockup surfaced
       before it shipped. Replaced with: the zone centered in the
       viewport (`insetX`/`insetY`), a thin ASCII border where there's
       room for one (degrading to a single line, then to nothing at the
       documented minimum terminal size, where the inset is
       mathematically exactly 0 and this renders identically to the old
       wall-padded behavior), and the real surrounding overworld terrain
       drawn as a decorative backdrop outside it -- sampled from a new
       `world::OverworldGrid&` parameter on `drawZoneFrame`, centered on
       the player's own overworld position, which `GameState::x`/`y`
       already stay valid for while indoors. Movement/collision
       (`GameLoop::tryMoveZone`) is completely untouched -- still checks
       only `Zone::tileCodeAt`/`poiAt` in zone-local coordinates; the
       backdrop is never queried by game logic.

     Full mechanism and formulas: `docs/ARCHITECTURE.md`'s new
     "Full-screen presentation" section. Player-facing description:
     `README.md`'s Status paragraph. `docs/MAP_NOTES.md` and
     `docs/ZONE_NOTES.md` both got matching updates (the latter also
     correcting a now-stale `docs/GOTCHAS.md` entry that had described
     `drawZoneFrame` as depending on `Zone::tileCodeAt`/`poiAt`'s
     out-of-bounds fallback -- it now bounds-checks explicitly before
     ever calling them, though `tryMoveZone` still genuinely depends on
     that fallback and is noted as such).

     Verified via a throwaway, dependency-free `MapRendererSelfTest.cpp`
     (reimplementing the layout math, the label-placement collision rule,
     and the zone-inset math as standalone functions rather than pulling
     in MapRenderer.cpp's full world::/character::/combat:: dependency
     graph just to test three pure functions -- same reasoning as
     Milestone 133's own self-test): confirmed the new layout formula
     exactly reproduces the old one at the 120x30 baseline, demonstrated
     the actual fix numerically (78x30 -> 148x56 at a ~212x60 console),
     confirmed the label-placement helper never partially writes a row on
     a failed placement attempt, and confirmed the zone-inset is exactly
     0 at the documented minimum terminal size and positive/centered
     above it -- then deleted, along with its temporary CMake target, per
     the standing self-test convention. Clean `/W4` rebuild, zero new
     warnings. Piped character-creation smoke test passed (no real
     `save1-3.txt` existed this session to move aside). **Interactively
     confirmed** by the user on 2026-09-01 at their real maximized
     terminal -- the map fills the screen with no dead gutter, inline
     location labels read cleanly, and zone interiors show a correctly
     centered, bordered inset with sane surrounding backdrop terrain.

136. `what_the_tide_kept` -- a user-requested quest content pass. Direct
     re-check of every zone added since the last quest sweep
     (`reason_worth_giving`, Milestone 106) plus every zone that sweep
     already flagged as checked-and-rejected confirmed the project's own
     documented caution: the "reuse an already-written NPC's existing
     dialogue" method is genuinely exhausted. Every zone either already
     has a quest, was already checked-and-rejected (Crossing, Palanthas,
     Sancrist, Flotsam, Port Balifor, Tarsis, Pax Tharkas, Qualinesti,
     Neraka), or has zero talkable NPCs at all (Dargaard Keep, Foghaven
     Vale, Godshome, Hopeful Vale, Mount Nevermind, Qualimori, Que-shu --
     deliberately pure scenery/ruin/sacred-site zones). The one zone
     added since the last sweep, Port O'Call (Milestone 122), has only a
     Dockmaster with the same deliberately-mundane, no-thread dialogue as
     every rejected boat-logistics NPC.

     Put to the user directly: accept the well is dry, or add one new
     small NPC/hook, the only precedent for which this project has is
     Xak Tsaroth's Ruin-Scavenger and Southern Ergoth's Kaganesti
     Lookout. They chose to add one. Rather than inventing a wholly new
     character, this gives voice to one already half-there: Port
     O'Call's Beachcomber's Stall (`data/zones/port_ocall.txt` POI `B`)
     was a non-talkable shop-only POI whose own description already
     implied a voice and a hook ("the old woman minding it swears a few
     pieces still carry a working charm, salvaged off ships that never
     made land"). `B` gains `TALK`/`TALK_AGAIN`/`QUEST B
     what_the_tide_kept`; `SHOP B magic` stays untouched and open (she's
     already an active shop, unlike Flint's Smithy's `SHOP_LOCKED`
     precedent). A new POI, `W` "The Storm-Wrack," is the DELIVER
     source (`GRANTS_ITEM W drowned_sailors_locket`), matching
     `ore_for_the_forge`'s Ore Cart phrasing. `data/quests.txt`'s new
     `QUEST what_the_tide_kept`: one `DELIVER drowned_sailors_locket 1`
     objective, no `REQUIRE`, `REWARD_STEEL 35`/`REWARD_XP 80` -- the
     same tier as `ore_for_the_forge`/`seed_for_thorbardin`, the two
     prior DELIVER-only quests. Port O'Call has no `PRESENCE`/
     `TIMELINE_ANCHOR` (map-only, like Crossing), so the stricter
     novel-citation bar doesn't apply. Zero `.cpp`/`.h` changes -- pure
     data content. Full writeup: `docs/QUEST_NOTES.md`'s "Shipped
     quests" and updated "Extending this later" sections.

     Verified via a throwaway self-test (`QuestZoneSelfTest.cpp`:
     `QuestLoader` parsing the real, now-18-quest `data/quests.txt`
     confirming the new quest's requirement/objective/reward shape;
     `ZoneLoader` parsing the edited `port_ocall.txt`, confirming the new
     `W` POI, `B`'s new `TALK`/`QUEST` lines, and the `GRANTS_ITEM` all
     parse and resolve to the expected coordinates), then deleted along
     with its temporary CMake target per the standing self-test
     convention. Clean `/W4` rebuild, zero new warnings (no source
     changes). Piped character-creation smoke test passed (real
     `save1.txt` moved aside, restored after) -- confirms `main.cpp`'s
     cross-validation accepts the new `QUEST B what_the_tide_kept` zone
     binding against the loaded `QuestCatalog`. **Not interactively
     walked** -- same standing `_getch()` limitation this project always
     discloses; talking to the Beachcomber, finding the Storm-Wrack,
     delivering the locket, and confirming the reward/journal entry all
     still need the user's own keyboard.

137. Day-gated Astinus SUBJECT content, plus 3 new topics -- what started
     as "add more subjects to Astinus's ask-anything pool" (routine
     content work, same as Milestones 132/133) surfaced a real,
     already-shipped bug the user caught directly: zone-file `SUBJECT`
     has no day-range gate at all, so 12 of Astinus's 48 existing entries
     stated a specific dated story beat as flat fact regardless of what
     day the player actually asked -- Sturm's death, Kitiara revealed as
     a Dragon Highlord, Laurana's Golden General title, the Xak Tsaroth/
     Pax Tharkas/Que-shu/Hopeful Vale events, all reachable from day 0.

     Fixed with a new zone grammar keyword, `SUBJECT_WHEN <char>
     <day-start> <day-end> <keywords> <dialogue...>`, mirroring
     `data/timeline.txt`'s existing character-level `SUBJECT_WHEN`
     exactly (same `dayEnd == -1` open-ended sentinel and validation
     wording, `TimelineLoader.cpp`). `world::PointOfInterest::subjects`
     grew two `int` fields (`dayStart=0, dayEnd=-1` defaults, so every
     other zone file's existing `SUBJECT`/`SUBJECT_ENDS` content needed
     zero changes); `GameLoop::speechFromPoi` filters by the current day
     using the identical comparison `timeline::Timeline::subjectsFor`
     already uses, before anything reaches the ask picker.

     The 12 affected entries were rewritten as before/after
     `SUBJECT_WHEN` pairs, each gate day sourced directly from
     `data/timeline.txt`'s `PRESENCE` windows: `gods`/`goldmoon`/new
     `mishakal` at day 3 (Xak Tsaroth), `tanis`/`verminaard` at day 12
     (Pax Tharkas), `riverwind`/`que-shu` at day 2 (Que-shu),
     `hopeful,vale` at day 17, `sturm`/`kitiara` at day 160 (the High
     Clerist's Tower siege), `mage,wizard,raistlin`/`tasslehoff` at day
     168 (pinned to that exact `PRESENCE palanthas 168 168` line). **The
     "before" half is not a deflection** -- a first draft used thin "ask
     me again later" stubs, which the user corrected directly: Astinus
     should have a real answer for everything Dragonlance-related, not a
     stonewall. Each "before" half instead matches its "after" partner in
     length and voice, giving Astinus's genuine take using whatever's
     already fair game at that point (pre-game backstory, established
     reputation, general public war knowledge) and omitting only the one
     specific dated payoff -- e.g. `verminaard` before day 12 still names
     him and still describes his rule by chained village and mine shaft,
     just without "I record every death... the ink went down easier than
     usual on his." The "after" half in every pair keeps the exact
     previously-shipped text unchanged.

     Three new entries, sourced from `References/pg.txt` (Player's Guide
     to the Dragonlance Campaign): evergreen `SUBJECT`s for the Moons of
     Magic (Solinari/Lunitari/Nuitari's real 36/28/8-day cycles) and the
     Knights' actual Oath and Measure ("Est Sularus oth Mithas" -- "My
     Honor is My Life"), plus `mishakal,goddess` as a third
     `SUBJECT_WHEN` pair gated at the same day-3 reveal as `gods`/
     `goldmoon`. Full writeup, including the day-sourcing table and the
     "before" half's design reasoning: `docs/ZONE_NOTES.md`'s "Ask about
     anything" section.

     Verified via a throwaway `ZoneSubjectWhenSelfTest.cpp`: confirmed
     `SUBJECT_WHEN`/plain `SUBJECT`/`SUBJECT_ENDS` all parse and merge
     correctly side by side with the right day ranges (explicit for
     `SUBJECT_WHEN`, implicit 0/-1 for the other two); confirmed a
     malformed day range (`day-end < day-start`, negative `day-start`)
     fails fast with the expected `file:line` message; confirmed the
     day-gate filter matches at the exact day-159-vs-160 boundary
     `sturm`/`kitiara` depend on; and loaded the real, edited
     `palanthas.txt` end-to-end, confirming all 13 `SUBJECT_WHEN` pairs
     (26 entries) and the 2 new evergreen entries are present -- then
     deleted, along with its temporary CMake target, per the standing
     self-test convention. Clean `/W4` rebuild, zero new warnings. Piped
     character-creation smoke test passed (real `save1.txt` moved aside,
     restored after) -- confirms the new grammar loads cleanly in the
     real game data directory. **Not interactively walked** -- same
     standing `_getch()` limitation; confirming the day-159-vs-160
     transition and the new topics' actual in-game phrasing still needs
     the user's own keyboard.

138. Three more Astinus SUBJECT entries -- continuing the ongoing
     ask-anything content pass (per standing practice, treated as routine
     content work, not a new design). `kagonesti` (the wild elves --
     tattooed, forest-bred, historically kept as servants by the Silvanesti
     and Qualinesti alike), `gnomes,nevermind,tinkers` (Mount Nevermind's
     200-clan, 50-guild Grand Council; the Life Quest each gnome commits
     to young, completion of which is said to seat their soul beside
     Reorx), and `gully,aghar,dwarves` (the Aghar -- disowned by dwarves
     proper, believing Reorx abandoned them too, praying to ancestor
     spirits instead) -- all evergreen (no `SUBJECT_WHEN` gate needed,
     racial/cultural lore rather than a dated story beat), all sourced
     from `References/pg.txt` (Player's Guide to the Dragonlance
     Campaign). `kagonesti` deliberately omits the bare `elves` keyword
     already claimed by the existing `elves,kinslayer` entry, to avoid
     first-match-wins silently intercepting it. Zero `.cpp`/`.h`
     changes -- pure data content, same shape as Milestone 133/134's
     `pg.txt`-sourced additions. Full writeup:
     `docs/ZONE_NOTES.md`'s "Ask about anything" section.

     Verified via a piped character-creation smoke test (real `save1.txt`
     moved aside, restored after) against the built `data/zones/
     palanthas.txt` -- confirms `ZoneCatalog` still parses the file
     cleanly with the 3 new entries added, no keyword collisions. No
     rebuild needed (no source changes); the updated zone file was synced
     into `build/Debug/data/` before the smoke test. **Not interactively
     walked** -- same standing `_getch()` limitation.

139. Three more Astinus SUBJECT entries -- continuing the same ongoing
     ask-anything pass, inserted right after Milestone 138's gully-dwarves
     entry: `minotaurs,mithas,kothas` (the honor-and-arena culture of the
     two minotaur island-kingdoms), `ogres,ogre,irda` (common ogres as a
     coarsened remnant of the once-beautiful Irda, the "first-born" high
     ogres), and `reorx,graystone,gargath,greystone` (the dwarves' forge
     god, and the Graystone of Gargath's role in the origin of dwarves,
     gnomes, and kender). All evergreen (no `SUBJECT_WHEN` gate needed --
     racial/mythological lore, not a dated story beat), all sourced from
     `References/pg.txt` (the Minotaurs and "Ogre Irda (First-Born)" race
     writeups, and the Reorx god profile plus the Graystone origin story
     under the Gnomes history section). Zero `.cpp`/`.h` changes -- pure
     data content, same shape as Milestone 138.

     The Reorx entry deliberately reports rather than resolves a real
     tension in the source material itself: `pg.txt` gives dwarves a
     religious belief that Reorx made them directly "in the god's image,"
     while separately crediting his escaped Graystone of Gargath with
     transmogrifying gnomes into dwarves and kender -- two accounts of the
     same peoples' origin that the source never reconciles. Astinus's line
     reports that tension ("I have recorded both accounts faithfully...
     Reorx himself has been notably unhelpful in settling it") rather than
     inventing a tiebreaker, per `CLAUDE.md`'s sourcing discipline. The
     minotaurs entry deliberately omits the source's own origin legend for
     the race (high ogres transformed by the same Graystone) because that
     legend is set on Taladas, a different continent outside this game's
     Ansalon scope -- the dialogue gestures at "whatever legend explains
     where they first came from" without asserting details an Ansalon
     historian has no particular reason to vouch for. Full writeup:
     `docs/ZONE_NOTES.md`'s "Ask about anything" section.

     Verified via a piped character-creation smoke test (real `save1.txt`
     moved aside, restored after) against the built `data/zones/
     palanthas.txt` -- confirms `ZoneCatalog` still parses the file
     cleanly with the 3 new entries added, no keyword collisions with any
     of Astinus's existing 51 entries. No rebuild needed (no source
     changes); the updated zone file was synced into `build/Debug/data/`
     before the smoke test. **Not interactively walked** -- same standing
     `_getch()` limitation.

140. Combat grid starting positions pushed farther apart. The player's
     starting row moved from `kCombatGridHeight - 2` to `kCombatGridHeight
     - 1` (the very bottom row) and the monster row from a fixed `y = 2`
     to `y = 0` (the very top row), in `GameLoop::runCombat`
     (`src/game/GameLoop.cpp`) -- widening the initial Chebyshev gap from
     5 rows to the grid's full 8-row vertical span. The user found the
     original gap closed in one or two rounds, leaving little room for
     ranged options (Hoopak, offensive spells) to matter before melee. No
     grid-size change (still 15x9, `kCombatGridWidth`/`kCombatGridHeight`
     in `MapRenderer.h`, untouched since Milestone 129) -- purely the two
     starting-row constants. Companion starting positions are unaffected
     in logic (still placed adjacent to `playerPos` in x, same y), they
     just inherit the player's new bottom row. Movement bounds-checking
     (`playerMoves`'s destination validation, `combat::stepToward` for
     monster/companion AI) already clamps to `[0, kCombatGridHeight)`
     with no assumption of spare rows past either starting position, so
     no other code needed to change.

     Verified with a clean incremental rebuild (`cmake --build build
     --config Debug`), zero new `/W4` warnings, plus a piped
     character-creation smoke test confirming `World`/`ZoneCatalog`/
     `Timeline`/`MonsterCatalog` still load (this change touches no data
     files, only two integer constants in `GameLoop.cpp`). **Not yet
     interactively walked** -- same standing `_getch()` limitation;
     confirm on the next play session that the wider starting gap feels
     right in an actual fight.

141. Weapon Specialization for Fighters (PHB p.71-73, Tables 34/35,
     visually confirmed via rendered page images -- `phb.txt`'s OCR of
     these two dense tables was column-scrambled and not trustworthy). A
     Fighter may now choose to specialize in their weapon at creation
     (`character::Character::specializedWeapon`, offered as a
     `promptYesNo` in `CharacterCreator.cpp` right after the Knight of
     Crown offer, race-agnostic): +1 to attack rolls, +2 to damage rolls
     (`character::kWeaponSpecializationToHitBonus`/
     `kWeaponSpecializationDamageBonus`, folded into `playerThac0Bonus`/
     `playerDamageBonus` in `GameLoop::runCombat` alongside the existing
     Frostreaver bonus -- deliberately stacks with it, since this project
     doesn't track weapon-type identity), plus a faster attacks-per-round
     progression (Table 35's melee column: 3/2 at 1-6, 2/1 at 7-12, 5/2 at
     13+, replacing Table 15's non-specialist rates -- `character::
     meleeAttacksThisRound` gained a `specialized` parameter, no default,
     every call site updated explicitly; companions always pass `false`,
     no creation flow exists to ever set theirs). Shown on the character
     sheet as "(specialized)" next to the weapon name; persisted as
     `SPECIALIZED <0|1>` (same shape as `SHIELD`, optional on load like
     `BROOCHDAY`/`STAFFCUREDAY`). **Deliberate scope cut, written up in
     `docs/CHARACTER_NOTES.md`'s new "Weapon Specialization" section**:
     Table 34's full numbered proficiency-slot system (a -2/-5/-3/-3
     attack penalty for an off-class weapon) is NOT modeled -- this
     project's equipment model gives each class exactly one weapon
     lineage bought from a fixed class-specific shop catalog, so that
     penalty could never fire in this engine; tracking slots for it would
     be inert bookkeeping. Also updated the "Knights of Solamnia"
     write-up's stale "no weapon-proficiency system... to plug into"
     claim, since ordinary Fighter specialization now covers what a
     Knight of Solamnia loses by not being a real Cavalier (just without
     the Cavalier's free-of-cost guarantee).

     Verified with a throwaway self-test
     (`character::meleeAttacksThisRound`'s new specialist branch across
     rounds 1-6/7-12/13+, both parities, plus the non-specialist path and
     a non-Warrior class -- all passed, then the test file and its
     CMakeLists.txt target were deleted), a full clean rebuild (zero new
     `/W4` warnings across all 30 source files), and two piped
     character-creation smoke tests against the real save-slot menu (an
     empty slot, `save1.txt` left untouched) confirming the new prompt
     appears and works correctly for a Fighter (text, y/n handling, and
     the resulting summary screen all correct) and is correctly skipped
     for a Mage. **Not yet interactively walked in a real fight** -- same
     standing `_getch()` limitation; confirm on the next play session that
     a specialized Fighter's to-hit/damage/attack-rate all read correctly
     in actual combat.

142. Three new Monster Manual monsters: Harpy, Griffon, Stirge. Continues
     the Milestone 107/112 pattern (a batch of three, each visually
     confirmed against a rendered page image — `Monster Manual (2nd
     ed).pdf` is Acrobat-Capture OCR and unreliable for stat-block
     tables, same caveat as the PHB's Weapon Specialization tables) after
     confirming DLA's own "Common Creatures of Krynn" chapter is still
     dry beyond Ice Bear, so all three come straight from the Monster
     Manual: Harpy (p.184, AC7/HD7/THAC0 13, bite `DAMAGE 1 6 0`, XP 975),
     Griffon (p.178, AC3/HD7/THAC0 13, bite `DAMAGE 2 8 0`, XP 650), and
     Stirge (p.332, AC8/HD1+1 but "attacks as 4-Hit-Die" already baked
     into its printed THAC0 17, proboscis `DAMAGE 1 3 0`, XP 175). Harpy
     and Griffon are the roster's first flying/aerial predators; Stirge
     is a low-tier swarm pest. Roster grows from 26 to 29.

     Each multi-attack creature (Harpy/Griffon, both claw/claw/bite) is
     simplified to its single most damaging real die, the same treatment
     already used for the Ghoul/Owlbear/Troll/Black Bear/Ice Bear/Lizard
     Man. Special abilities stay unmodeled for the same reason those
     precedents do — no charm/status-effect or attached/ongoing-effect
     system exists for anyone yet: Harpy's real charming song (and its
     50%-chance bone-club weapon variant) and Stirge's real attach-and-
     drain (1d4 blood/round once its proboscis hits) are flagged in
     `data/monsters.txt`'s comments but not mechanically modeled, same
     restraint as the Kapak's paralysis-poison bite and the Owlbear's
     hug.

     Terrain-code honesty, called out explicitly in the file: this
     engine has no coast or subterranean terrain code (ocean/shallow
     water are both non-walkable, so no monster can ever be purely
     coastal or aquatic here), so Harpy's real "land or coast"
     Climate/Terrain becomes an invented, informed-not-transcribed
     `TERRAIN_BIAS . ^` (grassland+hills) rather than a literal coast
     restriction. Griffon's "Hills or mountains" and Stirge's
     forest-representable half of "Forests or subterranean" are both
     narrow and fully representable, so each gets a hard `ONLY_TERRAIN`
     instead, the same treatment already used for Thanoi/Ice Bear's own
     narrow, sourced restrictions (`ONLY_TERRAIN ^ A` and `ONLY_TERRAIN
     %` respectively).

     Danger gating: Harpy and Griffon are both HD7, one Hit Die above the
     Sivak/Troll pair (`MIN_TOWN_DISTANCE 35`) and below Ettin/Aurak
     (40/45) — both land at `MIN_TOWN_DISTANCE 35`, the same bracket as
     that HD6 pair, without one of the Aurak's own spellcasting-driven
     further gate. Matching the established policy that every
     `MIN_TOWN_DISTANCE`-gated monster stays solo despite real No.
     Appearing data supporting groups, neither carries a `GROUP` line.
     Stirge is low-danger by contrast (XP175, matching the Black
     Bear/Worg tier) and gets no distance gate at all, carrying `GROUP 4
     4` instead (real No. Appearing 3-30, clamped to this project's
     playability cap), the same "always a full band of 4" treatment
     already used for Goblin/Kobold/Lizard Man. STEEL isn't a sourced
     field in this project; Harpy/Griffon are given the same tier as
     their nearest HD neighbor, the HD6 Sivak (`STEEL 3 10 0`), and
     Stirge — `Intelligence: Animal (1)` — carries `STEEL 0 0 0` despite
     its real Treasure Type D, matching every other Animal-intelligence
     monster in the roster (Wolf, Worg, Black Bear, Ice Bear).

     No C++ source or `CMakeLists.txt` changes — every field used already
     has a loader/grammar keyword. README.md's Status paragraph was
     deliberately left untouched: checked against the Milestone 112
     commit (the closest precedent, same shape of change), which touched
     only `data/monsters.txt` and these same three doc files, not
     README — this project's README monster list has never been kept as
     an exhaustive enumeration (it's already missing several
     already-shipped monsters, e.g. Wight/Troll/Ettin), so adding to it
     here would be inconsistent with precedent, not a fix.

     Verified with a clean incremental rebuild (no source changed, so
     this was a no-op recompile, run anyway per the standard workflow)
     and a piped character-creation smoke test confirming `MonsterCatalog`'s
     fail-fast loader parses all three new blocks cleanly. **Not yet
     interactively walked** — same standing `_getch()` limitation; confirm
     on the next play session that a wilderness encounter on hills/
     mountains (Griffon), grassland/hills (Harpy), and forest (Stirge)
     all read correctly in a real fight.

143. A Widow's Due — a fourth DELIVER quest, at Kalaman. User asked for
     another pass at the give-a-silent-POI-a-voice move `what_the_tide_
     kept` (Milestone 136) proved out. A fresh scan of every `data/
     zones/*.txt` file for described-but-`TALK`-less POIs, this time not
     limited to zones added since the last sweep, found mostly
     deliberately-mute scenery/sacred-site POIs or deliberately off-stage
     political seats (Kalaman's own Lord's Keep; Qualimori's Speaker's
     House, also a `TIMELINE_ANCHOR`). One real standout: Kalaman's
     Curiosities Cart (`data/zones/kalaman.txt` POI `P`), shop-only since
     Milestone 141, whose own description already names a specific
     person and backstory — "a war widow's trade, picked up piece by
     piece from refugees who needed steel more than keepsakes."

     The hook ties into dialogue this same zone already shipped: the
     Kalaman City Watchman (POI `G`) already complains, in his existing
     `TALK` line, about "light fingers" he can never catch in the bazaar.
     The widow lost her late husband's wedding band to exactly that; a
     trader working the market's fringes has it now.

     **Sink** — POI `P` gains `TALK`/`TALK_AGAIN`, two `SUBJECT` entries
     (cart/wares/authenticity; refugees/war/widow), `SUBJECT_UNKNOWN`,
     and `QUEST P a_widows_due`. `SHOP P magic` stays open
     unconditionally, same reasoning as the Beachcomber's Stall. **Source**
     — a new POI, `F` "A Furtive Trader," with `TALK F`/`TALK_AGAIN F`
     and `GRANTS_ITEM F soldiers_wedding_band A Soldier's Wedding Band`
     — deliberately distinct item name from Milestone 136's `drowned_
     sailors_locket`. `QUEST a_widows_due` (`data/quests.txt`): one
     `DELIVER soldiers_wedding_band 1` objective, no `REQUIRE`,
     `REWARD_STEEL 35`/`REWARD_XP 80` — the same tier as every prior
     DELIVER-only quest. No C++ or `CMakeLists.txt` changes — pure data
     content, same as every prior DELIVER quest. See
     `docs/QUEST_NOTES.md`'s "Shipped quests" for the full design writeup,
     including why Kalaman's own `TIMELINE_ANCHOR` doesn't raise the
     sourcing bar for these two original, invented-not-sourced characters.

     Verified with a clean `/W4` rebuild (zero new warnings, no source
     changed) and a piped character-creation smoke test (an empty save
     slot used; the user's real save left untouched) confirming
     `ZoneCatalog` and `QuestCatalog`'s fail-fast loaders parse the edited
     `kalaman.txt` and the new quest block cleanly end-to-end. **Not yet
     interactively walked** — same standing `_getch()` limitation;
     confirm on the next play session that talking to the Curiosities
     Cart, finding the Furtive Trader, delivering the band, and the
     reward/journal entry all read correctly.

## NEXT UP

Not yet started -- a short menu of well-grounded backlog candidates, not
a commitment. Pick one (or something else) before starting the next
session's work.

1. ~~**Interactive confirmation of Milestone 92's boat decline option and new
   Sancrist Isle -> Palanthas leg**~~ -- confirmed at Milestone 97, via the
   user's own keyboard on real save slot 2: decline/topics/accept works
   correctly at all four legs, the "northeast" direction line and 96-hour
   clock advance are both correct. Also surfaced and fixed the Embarkation
   Officer's thin topic coverage in the same session. See
   `docs/MILESTONES.md` entry 97.
2. ~~**Real mechanics for Bozak/Sivak/Aurak Draconians**~~ -- the parts
   that ground out in this engine's real combat math shipped at Milestone
   99 (Bozak's Magic Missile, Aurak's breath weapon, Sivak's death-burst).
   What's left (magic resistance/saves for all three; Sivak's
   shapeshifting; Aurak's dimension door, mind control, change
   self/polymorph self, invisibility, full spell list, and three-stage
   death) is either a flat stat or genuinely doesn't fit this project's
   positionless, no-monster-persistence combat loop -- not being pursued
   further absent a concrete reason to revisit. See `docs/COMBAT_NOTES.md`'s
   "Extending this later" and `docs/MILESTONES.md` entry 99.
3. **SFML-backed rendering, in place of the raw Windows console** — tried
   as an isolated stage-1 trial (2026-08-24, its own branch, never merged,
   fully reverted): a second `ansalon_sfml_trial` CMake target (SFML 3.0.0
   via `FetchContent`) rendering the real overworld data as colored
   monospace glyphs in a resizable window, with zero changes to
   `render::Console`/`MapRenderer`/`game::GameLoop` or the real
   `ansalon_rpg` target. User's verdict after seeing it run: "looks almost
   exactly the same" as the terminal -- rejected. That's an honest result,
   not a failed trial: stage 1 was always just glyphs-in-a-window: no
   sprite/tile art, since the whole point was testing the dependency and
   window mechanics before investing in art. The real visual payoff (stage
   2, actual sprites) was never attempted, so it remains a real option, but
   only if revisited *with real art*, not as plain glyphs again -- see
   `docs/ARCHITECTURE.md`'s "Why `Console` is the only platform-specific
   file" for the scoping this trial confirmed still holds (only
   `render/Console.cpp`, `render/MapRenderer.cpp`, and `GameLoop.cpp`'s
   input-polling call sites would need to change for a real migration;
   every data loader and all game logic stays untouched either way).
4. **More locations surfaced by the Milestone 85 map re-derivation --
   mostly resolved this session (research complete, one already shipped).**
   Thorbardin and Sancrist Isle, the two strongest candidates, shipped at
   Milestone 86; the Strait of Schallsea itself gained a real waypoint
   (Crossing) at Milestone 93. This session checked the rest against the
   actual novels: Nordmaar, the separate Schallsea Island named in
   `References/portcities.txt` under the New Sea (distinct from the Strait
   of Schallsea Milestone 93 modeled), Caergoth, and New Ports never
   appear on-page in any of the three Chronicles novels -- pure map
   geography, correctly left unmodeled. Sanction is real and vivid but
   only ever reported/flashback dialogue, never visited on-page by a
   tracked Hero -- also stays out. Southern Ergoth turned out to be a real
   gap and shipped at Milestone 95, also correcting a standing error in
   this project's own `docs/TIMELINE_NOTES.md`. Nothing left open from
   this original item.
5. ~~**Flotsam (and Port Balifor)**~~ -- shipped at Milestone 96. Both
   gained their own `LOCATION` and zone; all five Heroes gained matching
   `port_balifor 35 64`/`flotsam 65 82` windows. Kitiara stayed flavor-only,
   per Milestone 50's precedent. See `docs/MILESTONES.md` entry 96 and
   `docs/TIMELINE_NOTES.md`'s "Port Balifor and Flotsam" section. ~~One
   related thread stays open: Laurana's own later Flotsam/Dargaard Keep
   captivity...~~ -- resolved at Milestone 98, with a correction: she was
   never at Flotsam at all (that was Tanis's own, separate captivity); her
   actual capture is now staged at a new Dargaard Keep location, through
   Flint and Tasslehoff's own witnessed account. See `docs/MILESTONES.md`
   entry 98.
6. **A party of up to six characters**, recorded at Milestone 115 as the
   single biggest remaining gap between this project and a real Gold Box
   game -- `References/DQoK.pdf`'s entire combat chapter assumes it:
   deployment order, front-line/back-line positioning, per-character
   turns, NPC control, unconscious-but-not-dead party members left on the
   field. Also what unblocks thief backstab, Fighter "sweep" attacks, and
   the multi-target EXIT/QUIC commands -- all real, sourced mechanics that
   structurally can't exist with a solo PC. Not proposed lightly: this
   would touch the save format, character creation, and every combat (and
   probably several non-combat) screen. **Phase 1 (one recruitable
   companion, no combat) shipped at Milestone 116. Phase 2 (the companion
   actually fights, AI-controlled, full mutual combat -- the first real
   change to `GameLoop::runCombat`'s single-`Character` assumption)
   shipped at Milestone 117. Phase 3a (a real multi-companion roster --
   `GameState::companions` is now a `vector`, and a second companion,
   Dessa Corrin, joins Bren Alder) shipped at Milestone 118. Thief
   backstab and Fighter sweep attacks -- both party-wide, applying
   identically to the player and to companions -- shipped at Milestone
   119, interactively confirmed working after this session's input-replay
   bugfix (see entry 119's follow-up above).** Player-directed control (a
   UIC-style toggle) and deployment order are **deliberately deferred for
   the foreseeable future**, by user decision (2026-08-30) -- not being
   pursued further absent a concrete reason to revisit, same posture as
   item 2's Draconian mechanics above. Companions stay AI-controlled only.
   The smaller gaps Milestone 117 deliberately deferred remain open too
   (Brooch/Magic Missile/breath weapon/death-burst all still player-only,
   no opportunity attacks from companion movement, no "finish off a downed
   ally"). See `docs/COMBAT_NOTES.md`'s "Extending this later" section.
7. ~~**Four real, Atlas-named waypoints with no `LOCATION` yet**~~ -- Que-
   shu, Hopeful Vale, Qualimori, and Mount Nevermind all shipped at
   Milestone 131. Que-shu and Hopeful Vale are real overworld `LOCATION`s;
   Qualimori and Mount Nevermind turned out to need the same portal-nested
   treatment as Dragon Mountain below (their nearest neighbor crosses real
   open water in a straight line). See `docs/MILESTONES.md` entry 131 and
   `docs/MAP_NOTES.md`/`docs/TIMELINE_NOTES.md`/`docs/ZONE_NOTES.md`'s
   matching sections. ~~Dragon Mountain~~ shipped at Milestone 127 (as a
   portal-nested zone off Southern Ergoth, no new overworld `LOCATION`
   needed). ~~Skullcap~~ was removed from this list at Milestone 127 -- it
   isn't actually sourced to any tracked novel; see that entry.
8. ~~**A "World Map" screen**~~ -- shipped at Milestone 134: the whole
   480x320 grid downsampled (proportional box-majority-vote sampling, 3:1
   column:row ratio) with a side legend instead of the mockup's originally
   planned inline labels (a real clustering problem in the actual location
   data made those unreadable -- see that entry), `SIZE MEDIUM/LARGE`
   footprints on Palanthas/Thorbardin/Tarsis/Kalaman/Neraka/Port Balifor
   sourced against `References/portcities.txt` and the novels, bound to
   `'o'`. See `docs/MILESTONES.md` entry 134 and `docs/MAP_NOTES.md`'s
   "World Map screen" section.
