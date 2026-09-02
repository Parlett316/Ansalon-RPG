# Current work

Milestones 146 (terrain classification smoothing), 147 (mountains get glyph
`M`), and 148 (region-boundary highlighting) are all implemented,
documented, and interactively confirmed (2026-09-02, real `save1.txt`) --
see `docs/MILESTONES.md` entries 146-148. All three sit on branch
`terrain-smoothing` (off `master`), **not yet merged** -- merge timing is
the one open call left on this thread.

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

Milestone 136 (`what_the_tide_kept`, a new DELIVER quest at Port O'Call)
and Milestone 140 (combat grid starting positions widened -- 8-row gap
instead of 5) are both **interactively confirmed** (2026-09-02, real
`save1.txt`): the quest's reward/journal entry read correctly, and the
wider combat gap read correctly in a real wilderness fight.

Milestone 137 (`SUBJECT_WHEN` day-gated zone dialogue, fixing 12
already-shipped Astinus spoilers, plus 3 new topics), Milestone 138
(3 more Astinus topics -- Kagonesti, gnomes, gully dwarves), and Milestone
139 (3 more Astinus topics -- minotaurs, ogres/Irda, Reorx/Graystone of
Gargath) are all implemented, documented, and verified, but **none is yet
interactively walked** -- same standing `_getch()` limitation, and all
three require a trip to Palanthas to talk to Astinus, not yet made. Worth
doing once there: talk to Astinus early (before day 2/3/12/17) and confirm
his "before" answers read naturally, then again after day 160 (e.g. via a
save with `hoursElapsed` past that point) to confirm the "after" half still
reads exactly as before this milestone; ask him about Kagonesti, gnomes,
and gully dwarves (138) and minotaurs, ogres, and Reorx (139) and confirm
those entries read correctly.

The Astinus ask-anything pool is an ongoing, open-ended content thread --
expect more sessions like 137/138 adding further topics as they come up.

Milestone 141 (Weapon Specialization for Fighters -- PHB Tables 34/35: a
Fighter may choose at creation to specialize in their weapon for +1
to-hit/+2 damage and a faster attacks-per-round progression; full
per-weapon proficiency slots deliberately not modeled, see
`docs/CHARACTER_NOTES.md`'s "Weapon Specialization" section for why) is
**interactively confirmed** (2026-09-02, a fresh specialized Fighter vs. a
Giant Spider: observed +2 to-hit/+5 damage exactly matches STR's own
+1/+3 plus specialization's +1/+2). The faster attacks-per-round rate at
higher levels remains unconfirmed (this character is level 1) but isn't
worth a dedicated grind on its own -- the formula is shared with
Milestone 140's grid-gap work and already covered by the throwaway
self-test.

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
deliberately unmodeled) is **interactively confirmed** (2026-09-02, a
level-20 Mage vs. Kobold Skirmishers, via a disposable test save leveled
by editing `EXP` to the level-20 threshold and letting the real level-up
code run): Haste landed two attacks in one round; Slow showed the target's
AC jump from 7 to 11 in the bracket, the sourced +4. The paired -4 THAC0
penalty shares the same `result.amount` in code but wasn't directly
observed (the target died before attacking back).

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

Remaining unplayed from this stretch: 137, 138, 139, 142, 143, 145 (six
milestones) -- worth continuing the playtest pass before piling on more
unverified content.

Milestone 149 (three more Astinus SUBJECT entries -- `gilthanas`, `soth`,
and `ariakas,emperor`, continuing Milestone 145's shift to named characters
the player meets later in the story, each gated with a real `SUBJECT_WHEN`
before/after pair sourced from `data/timeline.txt`'s own PRESENCE/TOPIC
windows and verified against the actual `References/` PDFs; see
`docs/MILESTONES.md` entry 149 and `docs/ZONE_NOTES.md`'s "Ask about
anything" section) is implemented, documented, and verified via a piped
character-creation smoke test confirming `ZoneCatalog`/`Timeline` still
parse `data/zones/palanthas.txt` cleanly, but **not yet interactively
walked**. Worth doing on the next play session: ask Astinus about
Gilthanas, Lord Soth, and Ariakas both before and after the relevant day
thresholds (day 69/190/193 respectively) and confirm all six variants read
correctly.
