# Ansalon: Age of Despair

A personal, non-commercial fan project: an ASCII, terminal-based RPG set on
the world of Krynn (Dragonlance) during the War of the Lance, using 2nd
Edition AD&D rules, presented Caves-of-Qud style — a colored ASCII overworld
you walk across in real time. The eventual goal is an open world where the
canon Heroes of the Lance (Tanis, Sturm, Raistlin, Caramon, Goldmoon,
Riverwind, Tasslehoff, Flint...) move through their real novel-timeline
locations, so the player can stumble into a "chance encounter" with them if
they happen to be in the same place at the same in-game time.

This is a fan project built for fun, not for profit, and is not affiliated
with or endorsed by Wizards of the Coast / the Dragonlance IP holders.

## Status

Starting the game begins with a save-slot menu -- up to 3 independent
characters, each shown with a summary (name/level/race/class, in-game day)
or "(empty)" -- pick one to continue, or an empty slot for an interactive,
colorized, screen-per-step character creation wizard: roll 4d6-drop-lowest ability
scores six times (reroll the whole set as many times as you like), then
freely assign each roll to an ability; a race screen — Elf and Dwarf
prompt a Dragonlance subrace, with the resulting ability adjustments shown
before/after, and both the race and class screens reject a choice your
rolled scores don't qualify for (demihuman races/classes also cap out at
a real, race-specific level once you're playing) — a class screen, and an
alignment screen (Kender can't pick an Evil alignment — Dragonlance
Adventures states plainly that none are known to exist; qualifying
Fighters can join the Knights of Solamnia, and Gnomes are always Tinkers),
your scores carried along and visible at every step, before dropping you
into the world with a real character behind the `@`.
Progress autosaves continuously. The overworld/zone screen is a wide, frameless, side-by-side
view — no box border, just `=`/`-` rule dividers — a one-line header
(name/class, an ASCII HP bar, in-game day/hour) above, the walkable map on
the left, and a labeled status panel on the right (current mode, the
location/zone you're standing in, your coordinates, AC/THAC0/Steel, then
a headed, persistent, scrolling event log narrating what's happened:
blocked moves, look results, stepping in/out of a location, a short line
after each fight, and location flavor when you arrive somewhere) rather
than a single message that vanishes the next frame. A present NPC only
ever announces their name on arrival ("Tanis is here.") — press `;` to
Look and see their full description (or, if more than one person is
around, pick who to look at first). That panel only ever shows the most
recent stretch of the log — press `v` at any time for a full-screen,
scrollable view of everything logged all session. Every *other* screen in
the game (character sheet, combat, shop, inventory, dialogue, pickers,
the full log) still renders inside a plain-ASCII `+`/`-`/`|` window
border with its own title bar, so nothing floats loose against the raw
terminal; the whole frame sizes itself to your actual terminal window at
launch rather than assuming a fixed size. Talking to someone colors their
name, and any cursor-list picker (who to talk to, what to ask about,
Accept/Decline) colors its selected row; a fight colors your own stat line
and the monster's separately — the same palette used elsewhere, not a
separate look. The whole continent is a
480×320 tile grid,
generated from
the reference map image, walked tile-by-tile in real time; named locations
(Solace, Tarsis, Xak Tsaroth, the High Clerist's Tower, Ice Wall Castle,
Silvanesti, Kalaman, Palanthas, Godshome, Neraka, Thorbardin, Sancrist
Isle, Crossing, Southern Ergoth, Port Balifor, Flotsam, Dargaard Keep,
...) sit on that
grid, most connected by
roads baked into the terrain — Ice Wall, Sancrist Isle, and Southern
Ergoth are the exceptions, three sea-locked stops reachable only by
arranging passage on a ship, first out of Tarsis, then onward from Ice
Wall (see below); Crossing, a ferry
waypoint on the strait north of Solace, has no road either but needs no
ship, since the water there is shallow enough to simply walk —
and every one of them now has a walkable interior (Enter to step in) —
including the Inn of the Last Home inside Solace, and Qualinost, the
elven capital, inside Qualinesti. Standing at a location can also reveal
canon Heroes of the Lance passing through on their own schedule — the
"chance encounter" engine described above now spans three novels: all
eight Heroes travel together through *Dragons of Autumn Twilight* (an
alternate path through Darken Wood, the climactic siege of Pax Tharkas,
then together again in Tarsis as *Dragons of Winter Night* opens), then
genuinely split for the first time — Sturm, Flint, and Tasslehoff continue
on, first to a dragon-orb quest at Ice Wall Castle, then shipwrecked by a
white dragon onto Southern Ergoth's refugee coast — captured, nearly
fought, and finally sheltered by elves scattered there from three
different homelands — before reaching Sancrist and then the siege
of the High Clerist's Tower and Sturm's Knighting and death, while Tanis,
Raistlin, Caramon, Goldmoon, and Riverwind are griffon-carried east to
Silvanesti instead, into a second dragon-orb crisis of their own — then
shelter a month at Port Balifor, funding onward passage with Raistlin's
traveling illusion act while Goldmoon's healing reputation quietly begins
to spread, before reaching the wreck-built port of Flotsam, where Tanis is
drawn into a dangerous entanglement with a Dragon Highlord while the
others wait out his unexplained absences and a ship is chartered into the
Blood Sea — and
then, as *Dragons of Spring Dawning* opens, Flint and Tasslehoff travel on
to Palanthas — witnessing, from the outside, the same night Raistlin's own
dragon-orb escape from the Blood Sea maelstrom lands him half-dead on the
Great Library's steps — before reaching Kalaman for its Spring Dawning
festival, while the rest of the surviving party washes ashore after a
shipwreck and reunites with them there, just as a Dragon Highlord's
ultimatum arrives — that same night, Flint and Tasslehoff watch a forged
letter lure Laurana away and lose her to an ancient, spectral knight in a
mountain clearing below Dargaard Keep — and then Tanis, Caramon, Flint,
and Tasslehoff travel on together into the hidden mountain hollow of
Godshome, where Flint dies
of a sudden, peaceful heart failure, and then Tanis, Caramon, and
Tasslehoff carry on into the walled Temple compound of Neraka itself for
the war's climax and ending — see `docs/TIMELINE_NOTES.md`. Fizban, the
eccentric old wizard, is the first canon character named and made
talkable beyond the eight Heroes themselves — travels with the party
from Qualinesti through the siege of Pax Tharkas, then resurfaces, still
unidentified as anything more than "Fizban," on Southern Ergoth's
refugee coast and again at both Godshome and Neraka.
Laurana is the second and largest: introduced as the Speaker of the
Suns's daughter at Qualinesti, she proves herself in the fighting at Pax
Tharkas, takes up an ancient blade to kill a Dragon Highlord at Ice Wall,
talks a shipwrecked standoff back from the brink of elf killing elf on
Southern Ergoth, forces the dragon orb to its limit and delivers Sturm's
eulogy the day he dies at the High Clerist Tower, rises to command the
war itself as the Golden General, is lured from Kalaman by a forged
letter and taken captive below Dargaard Keep, and reunites with Tanis in
a final captivity at Neraka.
Alhana Starbreeze, the Silvanesti princess who leads Tanis's half of the
party home by griffon, is talkable throughout the Tower of the Stars
crisis, at her father Lorac's side as he's freed from the dragon orb's
nightmare. Silvara, a silver dragon living disguised among Southern
Ergoth's Wilder Elves, is talkable there too, caught between the oath
she swore to stay out of the wars of elves and men and the shipwrecked
strangers she couldn't bring herself to leave to the sea. Kitiara, the Dragon Highlord responsible for Sturm's death and Laurana's
captivity, stays off the talk/topic picker by design — her defining
scenes surface as retrospective dialogue inside Tanis's, Laurana's, and
Caramon's own `TOPIC` entries instead. Lord Derek Crownguard and Lord
Gunthar Uth Wistan get the same treatment: Derek's doomed unauthorized
sortie against the besieging dragonarmy and Gunthar's political maneuvering
to see Sturm fully vindicated both surface only inside Sturm's and
Laurana's own `TOPIC` entries at the High Clerist's Tower. Stepping inside
a zone carries the encounter through too: find them
gathered at the Inn's fireplace, Haven's market, Xak Tsaroth's old well,
Qualinost's Hall of the Sky, Darken Wood's faded trail, the Tharkadan mine
entrance at Pax Tharkas, Tarsis's old dock, the Tower's Muster Yard, the
Wilder Elves' Camp on Southern Ergoth, the
Tower of the Stars in Silvanost, Kalaman's Market Square, Palanthas's
Great Library, the bare stone at Godshome where Flint fell, or the ruined
Temple Square at Neraka, not just
standing on the overworld tile. You can actually talk to them
(and to NPCs inside zones, like the Inn's Otik and Tika, Haven's Seeker
Guard, Darken Wood's Forestmaster, the Tower's Garrison Knight, Ice Wall's
own young Knight, a wary Silvanesti Sentry on Southern Ergoth's coast,
Silvanost's Warder, Kalaman's City Watchman,
Palanthas's Astinus and Knight of the Watch, and a deserting soldier
amid Neraka's own wreckage) — press `t`, grounded in
the original DL1-3 adventure
modules and, for the Heroes of the Lance, the Chronicles/Legends novels:
real reactive dialogue based on your own race/class/alignment, branching
topics to ask about, and real memory of whether you've spoken before, not
just one static line forever — carried whether you met them out in the
open or found them indoors. Every one of the 8 Heroes of the Lance can also be asked about anything by
typing it rather than only picking from the topic menu — real subjects
(self-identity, an opinion of each other Hero) get a real, in-character
answer, anything else gets an in-character non-answer instead of a menu
that simply doesn't offer it. Raistlin's own pool goes deepest, real
content at any of his eight stops: his eyes, his golden skin, the Test,
the Staff of Magius, his family, Kitiara — who gets a different answer at
the Inn than everywhere else, a specific letter-scene beat overriding the
general one. Every talkable zone-native NPC (Otik, Tika, Astinus, every
zone-native guard/knight/warder, and more — 22 in all) has the same
"ask about anything" ability too, two subjects each drawn from their own
established voice. Arrive somewhere after the Heroes have already
moved on and it shows, on thirteen POIs now: Otik at the Inn of the Last
Home, Haven's Seeker Guard, Xak Tsaroth's Ruin-Scavenger, Qualinesti's
Elven Sentinel, Darken Wood's Forestmaster, the Tower's Garrison Knight,
Ice Wall's Young Knight, the Silvanesti Warder, Palanthas's Knight of the
Watch, Kalaman's City Watchman, Pax Tharkas's Fortress Guard, Tarsis's Old
Sailor, and Neraka's Deserting Guard each have their own thing to say the
first time you talk to them once the Heroes' stay there has passed, even
if you'd already met them before the Heroes ever arrived. Some places, like Ice Wall Castle,
Southern Ergoth, and Sancrist Isle, sit on their own sea-locked landmasses
with no road to them at all — talk to the Knight's Runner in Tarsis and
he'll offer you passage to Ice Wall directly, an Ice Barbarian Guide there
offers passage onward toward Sancrist (though the crossing doesn't go as
planned), a Silvanesti Sentry on Southern Ergoth offers passage on to
Sancrist proper, and an Embarkation Officer there offers passage on to
Palanthas, each leg a ship's voyage of a few days rather than a
tile-by-tile walk across open water — you can say no and ask them about it
first, and the offer stands until you board. Traveling the wilds now risks a random encounter — goblins, kobolds,
hobgoblins, wolves, giant spiders, bugbears, ogres, gnolls, ghouls,
skeletons, zombies, Baaz/Kapak/Bozak/Sivak/Aurak draconians, or Thanoi
(Icewall Glacier's walrus-men), all sourced from a real
2e Monster Manual and, for the
Krynn-specific draconians and Thanoi, Dragonlance Adventures (no orcs, since Krynn
has none) — fourteen of the lower-danger monsters (goblins, kobolds,
hobgoblins, wolves, bugbears, gnolls, ghouls, skeletons, zombies, Baaz
draconians, worgs, black bears, lizard men, giant toads) now turn up in
real, book-sourced numbers rather than always solo, each shown with its
own HP and a letter (Goblin A, Goblin B, ...) so you can pick a target
when more than one's still standing — three of the draconians fight back with real, book-sourced
abilities beyond a plain weapon swing: Bozaks sometimes cast Magic Missile
instead of attacking, Auraks sometimes breathe a noxious cloud (save for
half damage, or take full damage and fight on blinded), and Sivaks burst
into flame with one last retaliatory hit as they fall — the chance of one
varies by terrain, roads safest and forest/
mountains riskiest, and which monster you draw leans toward that terrain too
(Bugbears more common in hills and mountains, Gnolls never on salt flats,
Thanoi more common on glacier, and so on) — resolved with real 2e attack/damage math, and the combat log shows that math for every
weapon swing (natural-roll, THAC0/AC, and damage-die breakdown, not just
the hit/miss result) (Enter to attack, `f` to flee, and a Mage or
Cleric can also `m` to cast — a real multi-level spellbook now, 49 spells
across Mage's 9 levels and Cleric's 7, sourced from an official TSR/SSI
Dragonlance computer game manual and cross-checked against the actual PHB
(damage, healing, blocking a monster's attacks, or a this-fight to-hit/AC
buff or debuff, depending on the spell), with exactly one spell left it
casts directly, with more than one a picker asks which); losing just
knocks you out and sends you back to Solace, it isn't permadeath — see
`docs/COMBAT_NOTES.md`. Press `r` to rest, once per in-game day: it heals
1 hit point (the DMG's real natural-healing rate) and, for a Mage or
Cleric, re-memorizes their standing spell loadout for the day (asking
first if you'd rather choose a new one) — no slots are available at all
until you have, sourced from the PHB's actual memorization/prayer rules —
see `docs/CHARACTER_NOTES.md`'s "Spellcasting" section. A real bed
heals faster: press `z` on the Inn of the Last Home's upstairs landing to
fully heal overnight instead, the same 8 hours as ordinary rest — see
`docs/CHARACTER_NOTES.md`'s "Rest and spell memorization" section. Winning
fights earns Steel Pieces (Krynn's own
post-Cataclysm currency, not gold) and experience, and that steel now has
somewhere to go — press `p` at a shop to buy real gear. Six shops now
exist across five towns, each with its own distinct catalog rather than
one shared list: Solace's General Store (the flagship, everything below)
and Flint's Smithy (armor/weapons only, locked until you deliver ore for
the `ore_for_the_forge` quest), Haven's Market Stalls (Leather/Studded
Leather/Hide armor and a potion), Tarsis's Old Sailor (a potion and a
salvaged enchanted weapon), Kalaman's Market Square (Leather/Studded
Leather/Hide/Chain armor, a weapon upgrade, a Hoopak for Kender, a
potion), and Palanthas's Harbor (all seven armor tiers, an enchanted
weapon, a potion). Where armor's on offer you can buy real Leather/
Studded Leather/Hide Armor/Chain Mail/Splint Mail/Plate Mail/Field Plate
(Mages and Tinkers can't wear armor at all, per the PHB's own rule) and a
weapon upgrade — every class has one now, down to a Mage's Quarterstaff
and a Tinker's Light Crossbow. A Kender character starts equipped with a
Hoopak instead of their class's usual starting weapon — a real sling-staff
sourced from the Dark Queen of Krynn computer game manual (neither core
rulebook stats it) — and can still buy their class's own weapon upgrade
later, since the two aren't mutually exclusive; the shops above also sell
a Hoopak outright, for the rare case a Kender needs a replacement.
Press `i` inside the shop to
switch to selling gear back for half its price. Purchases land in a real
carried inventory rather than being worn automatically — press `i`
outside a shop to see what you're carrying and equip it, which actually
changes your AC and damage in the next fight, swapping whatever you had
on back into your pack rather than losing it — see
`docs/CHARACTER_NOTES.md`'s "Equipment" section. Every shop carries a
Potion of Healing (2d4+2 hp, 200 stl, DMG-sourced and priced) — framed as
a scavenged pre-Cataclysm relic rather than a merchant's own brew, since
real clerical healing magic doesn't return to Krynn until Goldmoon's
Disks of Mishakal early in the story; drink one from the inventory screen
(`i`, `Enter`) or mid-fight (`i` again, spending your round on it instead
of attacking) — see `docs/CHARACTER_NOTES.md`'s "Potions" section. Most
shops also carry a "+1" enchanted weapon, one per class (an Ensorcelled
version of your class's own upgrade weapon) —
sourced from the DMG's magic-item tables and Dragonlance Adventures' own
"Magical Items of Krynn" chapter, and the first thing in the game to add
a real to-hit bonus beyond Strength. At Solace's General Store only, a
Mage can also buy a Webnet (negates a foe's next attack) or a Brooch of
Imog (blocks every attack for the rest of a fight, once per day) — both
used the same way as drinking a potion mid-combat — see
`docs/CHARACTER_NOTES.md`'s "Magic items" section. A Cleric who earns it
can wield the Staff of Striking/Curing instead: a permanent +3 weapon that
also calls on a once-per-day self-heal, mid-combat, the same way. A
Strength-13+ character who earns one from Ice Wall Castle's Young Knight
wields a Frostreaver -- a heavy battle axe of Icewall Glacier ice, "+4"
to hit and damage per Dragonlance Adventures, but only while actually
standing on glacier terrain; carry it anywhere else and it's just an
ordinary axe, same as the book's own "melts above freezing" weakness
implies. Enough experience means
real leveling — more hit points, a better THAC0, better saving throws, all
sourced from the PHB's level-by-level tables (a Knight of the Crown gets a
nod toward the Order of the Sword at 3rd level, and a Mage actually
undergoes the Test of High Sorcery and is Robed by alignment) — see
`docs/CHARACTER_NOTES.md`. The world finally has things to actually do:
press `g` at any time to check your quest journal, and talk to a
quest-giver to be offered one, track its progress, and turn it in for a
reward — the moment every objective's actually done, the game says so
itself ("...is ready to turn in -- return to..."), so you never have to
guess or walk back speculatively. Twelve ship so far — Solace's Notice
Board offers a bounty to clear three timber wolves off the south road;
Otik at the Inn of the Last Home, the Garrison Knight at High Clerist's
Tower, and Kalaman's City Watchman each have their own reason to send you
somewhere or against something; the Silvanesti Warder will only speak of
hers to a fellow Elf; once a Knight of the Crown has proven
themselves, High Clerist's Tower's Sword Knight can sponsor real
advancement into the Order of the Sword; once you've reached that
Sword rank, the Tower's Knight of the Circle can grant real Solamnic
Armor (AC 0, sourced directly from Dragonlance Adventures); at the top
of the chain, a Rose Knight at the same Tower can name a proven Sword
Knight into the Order of the Rose, Solamnia's highest rank; back in
Solace, Flint Fireforge's smithy — pure scenery until now — has a
journeyman who'll ask you to fetch raw ore from Pax Tharkas's contested
Tharkadan Mine and actually carry it back, this project's first quest
built around a real, granted-in-the-world object rather than a place
visited or a foe slain; and a Ruin-Scavenger picking through the sunken
ruins of Xak Tsaroth has a priest's rod they can't use and would rather
see go to someone who can, if a Cleric first clears out what's nested in
the old well shaft below; Ice Wall Castle's Young Knight has a
Frostreaver none of his own company is strong enough to lift, salvaged
off a dead Ice Folk raider, theirs for the taking if they're strong
enough to wield it and willing to thin the thanoi still testing the
castle's gate; and a displaced farmer sheltering in Thorbardin, driven
from Pax Tharkas, would trade real seed grain for a real shot at
planting something come spring — a Farmer's Cart in Haven has more than
its own fields will use this season, if you're willing to carry a sack
the distance — see `docs/QUEST_NOTES.md` for the full design, what
ships, and how more get authored.

If you're picking this project up fresh (human or AI), read
`docs/ARCHITECTURE.md` (why the code is shaped the way it is),
`docs/GOTCHAS.md` (non-obvious traps already hit and worked around),
`docs/MAP_NOTES.md` (how the overworld map data was generated),
`docs/ZONE_NOTES.md` (how walkable interiors are authored),
`docs/CHARACTER_NOTES.md` (which 2e rules are modeled, which are
deliberately deferred, and an important accuracy caveat),
`docs/TIMELINE_NOTES.md` (how the chance-encounter schedule is authored),
`docs/COMBAT_NOTES.md` (attack/damage math, and what's invented vs.
sourced), and `docs/QUEST_NOTES.md` (quest grammar, objectives, the
journal, turn-in flow) before making changes.

## Requirements

- CMake 3.20+
- A C++17 compiler. Developed against MSVC (Visual Studio Build Tools 2026).
  The code avoids MSVC-only extensions (`CMAKE_CXX_EXTENSIONS OFF`, no
  MSVC-only `_s` functions) so a future GCC/Clang build should need no
  source changes to the game itself — untested so far, but that's the
  intent. (Raw single-keypress input currently only has a Windows
  `<conio.h>` implementation; see `docs/ARCHITECTURE.md`.)
- Python 3 + Pillow (`pip install pillow`) — **only** if you need to
  regenerate `data/overworld.grid` via `tools/generate_overworld.py`. The
  built game itself has no Python/Pillow dependency at runtime.

## Building (Windows / MSVC)

From a plain PowerShell (no need to manually run `vcvarsall.bat` — the
Visual Studio CMake generator locates MSVC itself):

```powershell
cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
cmake --build build --config Debug
```

The built executable will be at `build\Debug\ansalon_rpg.exe`. It locates
`data/locations.txt`, `data/overworld.grid`, etc. (and up to three
save-slot files) next to itself — a post-build step keeps a `data/` copy
there automatically, so this needs no extra step (see the comment in
`CMakeLists.txt`).

If you have a different Visual Studio version installed, list available
generators with `cmake --help` and substitute the matching `-G` name.

### Alternative: Ninja from a Developer shell

If you prefer a single-config, faster incremental build, open a "Developer
PowerShell for VS 2026" (which pre-runs `vcvars64.bat` for you) and run:

```powershell
cmake -G Ninja -S . -B build
cmake --build build
```

### Sharing a build

To hand a playable build to someone who doesn't have this source tree,
run:

```powershell
powershell -File tools\package_release.ps1
```

This builds a Release exe and zips it with the data it needs into
`dist\AnsalonRPG.zip`. The recipient just unzips and runs
`ansalon_rpg.exe` — the MSVC runtime is statically linked, so no separate
Visual C++ Redistributable install is needed, just Windows 10+ with a
terminal at least 80x24 (Windows Terminal, or cmd/PowerShell — all
support the VT100 sequences the game relies on for color).

## Playing

Run the built `ansalon_rpg.exe`. It shows a menu of 3 save slots (each
autosaved continuously during play — see `docs/ARCHITECTURE.md`), any
occupied ones summarized by name/level/race/class/day; pick one to
continue that character (with a confirmation before a fresh character is
allowed to overwrite it), type `d1`/`d2`/`d3` to delete a slot's save
immediately (with its own confirmation), or pick an empty slot to open
character creation
(plain typed prompts — enter a name, keep or reroll your ability scores,
pick a race/class/alignment number, confirm). Once that's done, movement is immediate — no Enter key
needed:

- **Move**: `w a s d` for the 4 cardinal directions (no diagonals)
- **Enter** — step into a location's walkable interior (every location has
  one now), or step back out if you're standing on the `>` marker inside
  one
- `t` — talk to whoever's here (a canon character the timeline places at
  your current location today, or a talkable NPC inside a zone — any key
  dismisses the reply). If more than one character is present at once,
  `t` first asks who — up/down selects, Enter talks, `q`/Esc cancels.
  Characters remember whether you've spoken before: the first
  conversation is their real line, every one after is a real second line
  where one's been written (all 8 Heroes of the Lance and every zone NPC
  have one), or a quick nod of recognition otherwise. At Solace, the 8
  Heroes may also react differently depending on your character's own
  race/class/alignment, and offer a follow-up topic to ask about — up/
  down selects, Enter asks, `q`/Esc leaves the conversation. Where "Ask
  about something else..." appears on that menu (any of the 8 Heroes, or
  any talkable zone-native NPC), picking it lets you type any subject
  instead — Enter submits, Esc cancels back to the menu.
- `p` — browse/buy at a shop POI (six now, across five towns, each with
  its own catalog — see "Status" above) — up/down selects an item, Enter
  buys it, `i` switches to selling gear from your inventory back for half
  its price, `q`/Esc leaves the shop. Purchases go to your carried
  inventory, not straight onto your body.
- `i` — outside a shop: view your carried items and equip one — up/down
  selects, Enter equips (swapping in whatever you were wearing before),
  `q`/Esc leaves
- `c` — view your character sheet (any key dismisses it); a Mage or
  Cleric gets an extra `s` option there to see their full spell roster by
  level, separately from the sheet's own terse "memorized today" line
- `v` — view the full scrollable event log (everything logged this
  session — arrivals, blocked moves, look results, combat outcomes —
  not just the live side panel's recent tail), up/down to scroll,
  `v`/`q` to return
- `g` — view your quest journal (any key dismisses it). A talkable POI
  marked as a quest-giver offers a quest the first time you talk to it
  (Accept/Decline picker), shows a progress line on later visits, and
  turns it in with a reward once its objectives are met — see
  `docs/QUEST_NOTES.md`.
- `l` — look around (overworld: names the nearest notable place and its
  direction; inside a zone: everything is already on screen, so there's
  nothing further to reveal)
- `q` or Esc — quit

Walking into impassable terrain (open ocean, the Blood Sea, walls, trees) is
blocked with a message. Standing exactly on a named location or a
non-NPC point of interest shows its description; an NPC (a canon
character whose schedule places them there today, or a talkable zone POI)
only announces their name — Look (`;`) shows their full description, and
offers a picker to choose whom if more than one is present.
Traveling away from named locations carries a chance of a random encounter,
which takes over the screen: **Enter** attacks, **`m`** casts a memorized
spell if you're a Mage or Cleric (asks which, if more than one is
memorized and unspent today), **`f`** flees. Losing a fight knocks you out (HP capped at 1) and
sends you back to Solace rather than ending the run. The game sizes
itself to your terminal automatically at startup (queries the real
visible console window, not just the scrollback buffer, and shrinks the
map/log frame to fit — see `docs/ARCHITECTURE.md`); the absolute
minimum is 72 columns × 20 rows, below which it prints a clear error
and exits rather than trying to render something broken. This adapts
once at launch, not continuously — resizing your terminal window
mid-session won't reflow the frame until you restart.

## Regenerating the map

`data/overworld.grid` is generated from the reference map image by
`tools/generate_overworld.py` — see `docs/MAP_NOTES.md` for the full
process. Short version:

```powershell
pip install pillow
python tools/generate_overworld.py
```

## Project layout

```
src/world/     overworld: named places (Location, World, WorldLoader) and
               walkable terrain (Terrain, OverworldGrid); interiors:
               ZoneTile, Zone, ZoneLoader, ZoneCatalog
src/timeline/  canon character schedule for chance encounters (Timeline,
               TimelineLoader)
src/character/ 2e AD&D rules content (Race, CharClass, Ability, Alignment,
               Knighthood, WizardOrder, Leveling, Spellcasting, Equipment,
               Dice) and the interactive CharacterCreator wizard
src/combat/    monster roster + attack/damage/initiative math (Monster,
               MonsterCatalog, MonsterLoader, Combat)
src/render/    ASCII presentation + raw keyboard input (Console, MapRenderer)
src/game/      orchestration (GameState, GameLoop, SaveGame) and main.cpp
data/          locations.txt + zones/*.txt + timeline.txt + monsters.txt
               (hand-authored), overworld.grid (generated)
tools/         generate_overworld.py -- offline map generator, dev-only, not
               part of the shipped game
docs/          ARCHITECTURE.md, GOTCHAS.md, MAP_NOTES.md, ZONE_NOTES.md,
               CHARACTER_NOTES.md
```

See `docs/ARCHITECTURE.md` for the reasoning behind this split and where
future systems (the War-of-the-Lance timeline/encounter engine, combat) are
meant to attach.
