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
    ultimatum (Kitiara) and the captured Golden General (Laurana) both stay
    unnamed, same off-stage-major-character precedent as Alhana/Derek/
    Gunthar. Verified via a throwaway self-test, a visual check of the
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

## NEXT UP

Not yet started — a short menu of well-grounded backlog candidates, not
a commitment. Pick one (or something else) before starting the next
session's work.

1. **Terrain-specific monster pools** — encounter *chance* now varies by
   terrain (Milestone 27), but which monster you fight is still
   uniform-random regardless of terrain. See `docs/COMBAT_NOTES.md`'s
   "Extending this later."
2. **More monsters** — Bozak/Sivak/Aurak Draconians, Thanoi (walrus-men,
   flavor-only at Ice Wall so far -- see Milestone 36), and other
   Monstrous Manual entries are still untouched; the higher-tier
   draconians are spellcasters/shapeshifters, real mechanics this project
   doesn't model yet. See `docs/COMBAT_NOTES.md`'s "Extending this later."

**Continuing *Dragons of Spring Dawning* after Kalaman (Milestone 39):**
per the research pass documented there, Palanthas (the Great Library, the
cursed-but-visible Tower of High Sorcery), Godshome (small, self-contained
-- Flint's death), and Neraka (the climax, fortress-zone idiom like Pax
Tharkas/Ice Wall) are the natural next candidates. Flotsam, Vingaard Keep,
Dargaard Keep, sunken Istar, and Sanction are recommended to stay
flavor-only dialogue rather than walkable zones -- see `docs/MILESTONES.md`
Milestone 39 and `docs/TIMELINE_NOTES.md` for the full reasoning.
