# Current work

Milestone 146 (terrain classification smoothing for `data/overworld.grid`)
is implemented, documented, and verified via a full clean rebuild and a
piped character-creation smoke test -- see `docs/MILESTONES.md` entry 146
and `docs/MAP_NOTES.md`'s "Terrain classification smoothing" section. Landed
on branch `terrain-smoothing` (off `master`), not yet merged. **Not yet
interactively walked** -- worth doing on the next play session: walk the
overworld near Solace and a couple of other regions and confirm the terrain
reads as coherent regions rather than scattered noise, same as the mockups
already visually approved this session.

**One real open decision from this session, still unresolved:** an SFML
rendering spike (real graphical window + real pixel-art sprite tiles +
scrolling camera + per-tile animation, replacing the ASCII terminal for the
overworld map only) was built, hit and fixed a real bug (a dangling
`std::string&` reference in a hand-rolled JSON parser -- see the branch's own
history), and the user's verdict after seeing it was "looks a bit better."
That thread is parked, not abandoned or committed to: `git stash` on branch
`sfml-trial-2` holds `CMakeLists.txt`'s new `ansalon_sfml_trial` target and
`sfml_trial/main.cpp` (uncommitted). To resume: `git checkout sfml-trial-2 &&
git stash pop`.

Follow-on tile-art prototyping happened after that verdict, still purely as
Python/Pillow mockups in `References/` (gitignored, nothing wired into real
code): the user flagged that forest/grassland still looked like a repeating
"wallpaper" (every terrain code is one fixed 32x32 tile, so a large
same-terrain region shows the identical tree/mound/peak arrangement over and
over -- worse now that Milestone 146's smoothing makes those regions bigger
and more contiguous). Fixed in prototype by generating 6 art variants per
terrain code (forest, grassland, hills, mountains, savannah, bog, salt flat,
glacier -- every terrain whose motif is bold/icon-like enough to show the
repeat; ocean/shallow-water/road/Blood Sea/uncharted were already fine as
single tiles) and picking between them per-tile via a stable hash of grid
position (`(x*73856093) ^ (y*19349663)`, deterministic so it doesn't flicker
between renders). Visually confirmed a big improvement, especially on
mountains. User's call after seeing it: **"good enough, stop here for
now"** -- explicitly parked, not wired into `terrain_tileset.json` or the
stashed SFML trial code. If this resumes, the variant art itself already
exists (`References/variant_tiles6.png` = forest/grassland/hills,
`References/variant_tiles_extra.png` = mountains/savannah/bog/salt-flat/
glacier, each a 6-column strip) and the position-hash selection logic is
proven in the mockup script -- porting both into `terrain_tileset.json`'s
schema (single rect per code -> list of rects) and `sfml_trial/main.cpp`'s
tile lookup is the concrete next step, not a re-derivation.

Whether any of this (SFML itself, or the variant art on top of it) goes
further or gets fully reverted is still the user's call -- don't propose a
direction here without being asked; this project has a real history of
visual-change attempts (SFML with no art, twice-rejected ANSI truecolor
shading) that didn't land, and two more rounds this session ("just looking,
no action" on the first tileset preview; this variant-art round) that were
engaged with productively but still didn't convert into a commit-to-it
decision.

Milestone 136 (`what_the_tide_kept`, a new DELIVER quest at Port O'Call),
Milestone 137 (`SUBJECT_WHEN` day-gated zone dialogue, fixing 12
already-shipped Astinus spoilers, plus 3 new topics), Milestone 138
(3 more Astinus topics -- Kagonesti, gnomes, gully dwarves), and Milestone
139 (3 more Astinus topics -- minotaurs, ogres/Irda, Reorx/Graystone of
Gargath) are all implemented, documented, and verified, but **none is yet
interactively walked** -- same standing `_getch()` limitation. Worth
doing on the next play session: for 136, talk to the Beachcomber's Stall
at Port O'Call, find the new Storm-Wrack POI, deliver the locket, and
confirm the reward/journal entry read correctly; for 137, talk to Astinus
early (before day 2/3/12/17) and confirm his "before" answers read
naturally, then again after day 160 (e.g. via a save with `hoursElapsed`
past that point) to confirm the "after" half still reads exactly as
before this milestone; for 138, ask him about Kagonesti, gnomes, and
gully dwarves and confirm the new entries read correctly; for 139, ask
him about minotaurs, ogres, and Reorx and confirm the new entries read
correctly.

The Astinus ask-anything pool is an ongoing, open-ended content thread --
expect more sessions like 137/138 adding further topics as they come up.

Milestone 140 (combat grid starting positions widened -- player now
starts on the bottom row, monsters on the top row, an 8-row gap instead
of 5) is implemented and verified via clean rebuild + piped smoke test,
but **not yet interactively walked**. Worth doing on the next play
session: start a fight and confirm the wider gap plays well (not so wide
that closing distance feels tedious).

Milestone 141 (Weapon Specialization for Fighters -- PHB Tables 34/35: a
Fighter may choose at creation to specialize in their weapon for +1
to-hit/+2 damage and a faster attacks-per-round progression; full
per-weapon proficiency slots deliberately not modeled, see
`docs/CHARACTER_NOTES.md`'s "Weapon Specialization" section for why) is
implemented, documented, and verified via a throwaway self-test, a clean
rebuild (zero new warnings), and two piped character-creation smoke tests
(prompt appears correctly for a Fighter, correctly skipped for a Mage),
but **not yet interactively walked in a real fight**. Worth doing on the
next play session: create a specialized Fighter and confirm to-hit,
damage, and the faster extra-attacks rate all read correctly against a
real monster.

Milestone 142 (three new Monster Manual monsters -- Harpy p.184, Griffon
p.178, Stirge p.332, all visually confirmed against rendered page images;
roster grows from 26 to 29; see `docs/MILESTONES.md` entry 142 for the
terrain-code-honesty and danger-gating reasoning) is implemented,
documented, and verified via a data-only rebuild and a piped
character-creation smoke test confirming `MonsterCatalog` parses the
three new blocks cleanly, but **not yet interactively walked**. Worth
doing on the next play session: trigger a wilderness encounter on hills/
mountains (Griffon), grassland/hills (Harpy), and forest (Stirge) and
confirm all three read correctly in a real fight.

Milestone 143 (`a_widows_due`, a fourth DELIVER quest at Kalaman -- the
give-a-silent-POI-a-voice move proven a second time on the Curiosities
Cart; see `docs/MILESTONES.md` entry 143 and `docs/QUEST_NOTES.md`'s
"Shipped quests" for the full design) is implemented, documented, and
verified via a clean rebuild (zero new warnings) and a piped
character-creation smoke test confirming `ZoneCatalog`/`QuestCatalog`
parse the new content cleanly, but **not yet interactively walked**.
Worth doing on the next play session: talk to the Curiosities Cart in
Kalaman, find the new Furtive Trader POI, deliver the wedding band, and
confirm the reward/journal entry read correctly.

Milestone 144 (Haste and a real Slow -- Haste is a new Mage 3rd-level spell
that doubles the player's attacks-per-round this fight, unblocked now that
Milestone 108's `meleeAttacksThisRound` exists; Slow's existing THAC0
penalty was corrected from an invented -2 to the real -4 and gained a new
+4 AC penalty on the monster, both sourced against the scanned PHB p.192/
196 -- see `docs/MILESTONES.md` entry 144 and `docs/CHARACTER_NOTES.md`'s
"Haste and a real Slow" section for the full sourcing and what's still
deliberately unmodeled) is implemented, documented, and verified via a
throwaway self-test, a full clean rebuild (zero new warnings), and a piped
character-creation smoke test confirming `Spellcasting`'s new spell list
parses cleanly, but **not yet interactively walked**. Worth doing on the
next play session: as a Mage, cast Haste in a fight and confirm the logged
attack count doubles; cast Slow on a monster and confirm both the THAC0 and
AC math show up correctly in the "Showing the math" bracketed breakdown.

Milestone 145 (three more Astinus SUBJECT entries -- `fizban`, `silvara`,
and `berem,everman`, shifting from Milestones 138/139's race/pantheon lore
to named characters the player meets later in the story, each gated with
a real `SUBJECT_WHEN` before/after pair sourced from `data/timeline.txt`'s
own PRESENCE/TOPIC windows; see `docs/MILESTONES.md` entry 145 and
`docs/ZONE_NOTES.md`'s "Ask about anything" section) is implemented,
documented, and verified via a piped character-creation smoke test
confirming `ZoneCatalog`/`Timeline` still parse `data/zones/palanthas.txt`
cleanly, but **not yet interactively walked**. Worth doing on the next
play session: ask Astinus about Fizban, Silvara, and Berem/the Everman
both before and after the relevant day thresholds (day 192/69/193
respectively) and confirm all six variants read correctly.

Ten milestones in a row (136-145) are now implemented but unplayed --
worth a dedicated playtest pass on the next session before piling on
more unverified content.

Milestone 147 (mountains render as `M` instead of sharing hills' `^`
glyph, found during a graphics-design review of `world/Terrain.cpp`'s
glyph/color table -- see `docs/MILESTONES.md` entry 147 and
`docs/ARCHITECTURE.md`'s "Mountains get their own glyph" section) is
implemented and verified via a clean rebuild and a piped
character-creation smoke test, but **not yet interactively walked**.
Worth doing on the next play session: confirm mountains and hills now
read as visually distinct regions, both on the overworld and the World
Map screen (`'o'`) -- especially somewhere the two terrains sit
adjacent, e.g. near the Kharolis/Vingaard ranges.

Milestone 148 (region-boundary highlighting -- reverse-video (`\x1b[7m`)
on any overworld cell bordering a different *region*, modeled on
`References/cataclysm-dark-days-ahead.avif`'s dotted region outlines) is
implemented, documented, and verified, on the second attempt: a first
implementation (comparing raw adjacent terrain codes) was reverted the same
session after a headless render probe showed it flagged 46.5% of all land
cells -- pure static, not an outline. The fix bakes a much coarser "region"
classification offline (`tools/generate_overworld.py`'s
`compute_region_layer`, a new `data/overworld_regions.grid` file,
`world::OverworldGrid::regionCodeAt()`) rather than computing anything from
raw terrain codes or at game load time (a C++ timing check showed the
latter costs 894ms-1.3s in Debug -- too slow). Re-verified via a headless
render probe against the real save/grid: 5.0% border density on the
overworld viewport, 21.5% on the World Map screen, both visually confirmed
to trace real coastlines/biome boundaries cleanly. See `docs/MILESTONES.md`
entry 148, `docs/MAP_NOTES.md`'s "Region layer for boundary highlighting"
section, and `docs/ARCHITECTURE.md`'s "Region-boundary highlighting"
section (which also tells the full first-attempt story). **Not yet
interactively walked** -- a render probe can approximate but not fully
substitute for seeing this in a real terminal; worth confirming next play
session, alongside 147's check, on both the overworld viewport and the
World Map screen.
