# Ansalon: Age of Despair

A personal, non-commercial fan project set on the world of Krynn
(Dragonlance) during the War of the Lance, using 2nd Edition AD&D rules.
The primary build (`ansalon_sfml_phase1`, via SFML) renders a real
pixel-space overworld, zone interiors, and combat — the actual reference
map, walked in real time, with placeholder shapes/text standing in for
sprite art that hasn't been made yet. An earlier terminal build
(`ansalon_rpg`), presented Caves-of-Qud style as a colored ASCII
overworld, stays in the tree as a legacy/reference build rather than
being retired. This has been a presentation migration, not a systems
one — the rules/content described below apply to both, and most of
"Playing" below describes the primary SFML build specifically (see that
section for the console build's own, different controls).

**The SFML build now saves.** It autosaves back to the save file you
launched it with after every action — combat wins, leveling, resting,
shopping, equipping all persist across a close/relaunch, the same
"autosave after every action" convention the legacy console build has
always used. The save path itself is hardened (atomic write-then-rename,
rotated backups of the last few good saves, a version header that
refuses to misparse a save from a future build) — see
`docs/ARCHITECTURE.md`'s Save/load section — so a crash or force-close
mid-write can't corrupt your save. The legacy console build
(`ansalon_rpg`) remains available if you'd rather use it instead. See
"Playing" below.

The eventual goal is an open world where the canon Heroes of the Lance
(Tanis, Sturm, Raistlin, Caramon, Goldmoon, Riverwind, Tasslehoff,
Flint...) move through their real novel-timeline locations, so the player
can stumble into a "chance encounter" with them if they happen to be in
the same place at the same in-game time.

This is a fan project built for fun, not for profit, and is not affiliated
with or endorsed by Wizards of the Coast / the Dragonlance IP holders.

The overworld map (`References/dragonlancemap2.png`) used to place every
`LOCATION` in `data/locations.txt` — and, in the SFML build, rendered
directly as the overworld itself — is used with permission of its author,
**paercebal** ([www.paercebal.org](https://www.paercebal.org/HtmlKrynnMaps/index.html)),
built on the original map by **AtenOkke**. Both are credited here per the
terms of that permission.

## Status

### Character creation & saves

- The game opens on a **save-slot menu**: up to 3 independent characters,
  each shown with a summary (name/level/race/class, in-game day) or
  `(empty)`. Pick one to continue, or an empty slot to start character
  creation.
- **Character creation** is an interactive, colorized, screen-per-step
  wizard:
  - Roll **4d6-drop-lowest** ability scores six times (reroll the whole set
    as many times as you like), then freely assign each roll to an ability.
  - **Race screen** — Elf and Dwarf prompt a Dragonlance subrace, with the
    resulting ability adjustments shown before/after. Both the race and
    class screens reject a choice your rolled scores don't qualify for
    (demihuman races/classes also cap out at a real, race-specific level
    once you're playing).
  - **Class screen**.
  - **Alignment screen** — Kender can't pick an Evil alignment (Dragonlance
    Adventures states plainly that none are known to exist); qualifying
    Fighters can join the Knights of Solamnia, and Gnomes are always
    Tinkers.
  - Every Fighter is offered real **Weapon Specialization** (+1 to hit, +2
    damage with their weapon, and faster extra attacks as they level),
    sourced from the Player's Handbook's own optional rule.
  - Your scores are carried along and visible at every step, before
    dropping you into the world with a real character behind the `@`.
- Progress **autosaves continuously**.

### The overworld & interface

*(The specifics below describe the legacy console build's ASCII
presentation. The primary SFML build shows the same information — party
status, event log, HP, in-game time — in real pixel space instead, with
its own control list under "Playing" below.)*

- The overworld/zone screen is a wide, frameless, side-by-side view — no
  box border, just `=`/`-` rule dividers: a one-line header (name/class, an
  ASCII HP bar, in-game day/hour) above, the walkable map on the left, and
  a labeled status panel on the right (current mode, the location/zone
  you're standing in, your coordinates, AC/THAC0/Steel, then a headed,
  persistent, scrolling event log).
- The event log narrates what's happened — blocked moves, look results,
  stepping in/out of a location, a short line after each fight, and
  location flavor on arrival — rather than a single message that vanishes
  the next frame. Press `v` at any time for a full-screen, scrollable view
  of everything logged all session.
- A present NPC only ever announces their name on arrival ("Tanis is
  here."); press `;` to Look and see their full description (or pick who
  to look at first, if more than one is around).
- Every other screen (character sheet, combat, shop, inventory, dialogue,
  pickers, the full log) renders inside a plain-ASCII `+`/`-`/`|` window
  border with its own title bar, so nothing floats loose against the raw
  terminal.
- The whole frame sizes itself to your actual terminal window at launch
  rather than assuming a fixed size — a big, maximized window shows far
  more of the surrounding land at once, with each visible location's name
  captioned inline beside its glyph wherever there's room.
- Stepping into a location's walkable interior frames it with a thin
  border and shows the real surrounding countryside outside that border as
  backdrop (on any terminal bigger than the smallest one this game
  supports), rather than a blank or walled-off room.
- Color is consistent throughout: talking to someone colors their name,
  any cursor-list picker colors its selected row, and a fight colors your
  own stat line and the monster's separately — the same palette used
  elsewhere.

### The map & its locations

- The whole continent is a **480×320 tile grid**, generated from the
  reference map image, walked tile-by-tile in real time.
- Named locations — Solace, Que-shu, Tarsis, Xak Tsaroth, the High
  Clerist's Tower, Ice Wall Castle, Silvanesti, Kalaman, Palanthas,
  Godshome, Neraka, Thorbardin, Hopeful Vale, Sancrist Isle, Crossing,
  Southern Ergoth, Port Balifor, Flotsam, Dargaard Keep, Port O'Call, and
  more — sit on that grid, most connected by roads baked into the terrain.
  - **Ice Wall, Sancrist Isle, and Southern Ergoth** are the exceptions:
    three sea-locked stops reachable only by arranging passage on a ship
    (see "Sea travel" below).
  - **Crossing**, a ferry waypoint on the strait north of Solace, has no
    road either — its water can't be waded, and every traveler pays the
    Ferry Keeper for a real round-trip boat ride across to Port O'Call on
    the far shore.
- Every location now has a **walkable interior** (`Enter` to step in) —
  including the Inn of the Last Home inside Solace, and Qualinost, the
  elven capital, inside Qualinesti.
- Plus **secret places** reached only on foot from within a location
  rather than from the overworld: Foghaven Vale (hidden inland from
  Southern Ergoth's coast), Qualimori (a Qualinesti refugee camp reached
  the same way), and Mount Nevermind (the gnomes' mountain city, reached
  on foot from Sancrist Isle).
- Press `o` at any time for a read-only **World Map** screen — the whole
  continent downsampled to fit your terminal, every location marked (a
  handful of the biggest, like Palanthas and Thorbardin, get a bigger
  footprint) alongside a legend naming all of them. Reference view only,
  no way to travel from it.

### Chance encounters: the Heroes of the Lance

Standing at a location can reveal canon Heroes of the Lance passing through
on their own schedule. The "chance encounter" engine now spans three
novels:

- **Dragons of Autumn Twilight**: all eight Heroes travel together —
  fleeing Solace to find Goldmoon's home village of Que-shu already
  dragon-razed, an alternate path through Darken Wood, the climactic siege
  of Pax Tharkas, a hidden mountain refuge at Hopeful Vale where Goldmoon
  and Riverwind marry — then together again in Tarsis as *Dragons of
  Winter Night* opens.
- **Dragons of Winter Night**: the party genuinely splits for the first
  time.
  - Sturm, Flint, and Tasslehoff continue on to a dragon-orb quest at Ice
    Wall Castle, then are shipwrecked by a white dragon onto Southern
    Ergoth's refugee coast — captured, nearly fought, and finally
    sheltered by elves scattered there from three different homelands,
    with a brief stop at the Qualinesti refugee camp of Qualimori on the
    way inland.
  - Flint and Tasslehoff (Sturm splitting off alone toward the Knights'
    outpost) follow a silver dragon to Huma's Tomb and the hollow Dragon
    Mountain, before reaching Sancrist and the siege of the High Clerist's
    Tower — Sturm's Knighting and death.
  - Tanis, Raistlin, Caramon, Goldmoon, and Riverwind are griffon-carried
    east to Silvanesti instead, into a second dragon-orb crisis —
    sheltering a month at Port Balifor (funding onward passage with
    Raistlin's traveling illusion act while Goldmoon's healing reputation
    quietly spreads) before reaching the wreck-built port of Flotsam,
    where Tanis is drawn into a dangerous entanglement with a Dragon
    Highlord while the others wait out his absences and a ship is
    chartered into the Blood Sea.
- **Dragons of Spring Dawning**:
  - Flint and Tasslehoff travel on to Palanthas — witnessing, from the
    outside, the same night Raistlin's own dragon-orb escape from the
    Blood Sea maelstrom lands him half-dead on the Great Library's steps —
    before reaching Kalaman for its Spring Dawning festival, where the
    rest of the surviving party (washed ashore after a shipwreck) reunites
    with them just as a Dragon Highlord's ultimatum arrives.
  - That same night, Flint and Tasslehoff watch a forged letter lure
    Laurana away and lose her to an ancient, spectral knight in a mountain
    clearing below Dargaard Keep.
  - Tanis, Caramon, Flint, and Tasslehoff travel on together into the
    hidden mountain hollow of Godshome, where Flint dies of a sudden,
    peaceful heart failure.
  - Tanis, Caramon, and Tasslehoff carry on into the walled Temple
    compound of Neraka itself for the war's climax and ending.
- See `docs/TIMELINE_NOTES.md` for the full schedule design.

Other canon characters are talkable too, with their own arcs:

- **Fizban**, the eccentric old wizard, is the first canon character named
  and made talkable beyond the eight Heroes — travels with the party from
  Qualinesti through the siege of Pax Tharkas, then resurfaces (still
  unidentified as anything more than "Fizban") on Southern Ergoth's
  refugee coast, at Qualimori en route inland, and asleep inside the
  hollow Dragon Mountain nearby — a side trip to Mount Nevermind with
  Tasslehoff to have a dragon orb identified — and again at both Godshome
  and Neraka.
- **Laurana** is the second and largest arc: introduced as the Speaker of
  the Suns's daughter at Qualinesti, she proves herself at Pax Tharkas,
  takes up an ancient blade to kill a Dragon Highlord at Ice Wall, talks a
  shipwrecked standoff back from the brink on Southern Ergoth, stands
  before Huma's own tomb, forces the dragon orb to its limit and delivers
  Sturm's eulogy, rises to command the war as the Golden General, is
  lured from Kalaman by a forged letter and taken captive below Dargaard
  Keep, and reunites with Tanis in a final captivity at Neraka.
- **Alhana Starbreeze**, the Silvanesti princess who leads Tanis's half of
  the party home by griffon, is talkable throughout the Tower of the Stars
  crisis, at her father Lorac's side as he's freed from the dragon orb's
  nightmare.
- **Silvara**, a silver dragon living disguised among Southern Ergoth's
  Wilder Elves, is talkable there too — caught between the oath she swore
  to stay out of the wars of elves and men and the shipwrecked strangers
  she couldn't bring herself to leave to the sea — and talkable again
  after leading them inland to the secret refuge of Foghaven Vale.
- **Kitiara**, the Dragon Highlord responsible for Sturm's death and
  Laurana's captivity, stays off the talk/topic picker by design — her
  defining scenes surface as retrospective dialogue inside Tanis's,
  Laurana's, and Caramon's own `TOPIC` entries instead. **Lord Derek
  Crownguard** and **Lord Gunthar Uth Wistan** get the same treatment:
  Derek's doomed unauthorized sortie and Gunthar's political maneuvering
  to see Sturm vindicated both surface only inside Sturm's and Laurana's
  own `TOPIC` entries at the High Clerist's Tower.
- Stepping inside a zone carries the encounter through too — find them
  gathered at the Inn's fireplace, Haven's market, Xak Tsaroth's old well,
  Qualinost's Hall of the Sky, Darken Wood's faded trail, the Tharkadan
  mine entrance at Pax Tharkas, Tarsis's old dock, the Tower's Muster
  Yard, the Wilder Elves' Camp on Southern Ergoth, Huma's Tomb in Foghaven
  Vale, the Tower of the Stars in Silvanost, Kalaman's Market Square,
  Palanthas's Great Library, the bare stone at Godshome where Flint fell,
  or the ruined Temple Square at Neraka — not just standing on the
  overworld tile.

### Talking to people

- Press `t` to talk to whoever's here — canon Heroes on their schedule, or
  NPCs inside zones (the Inn's Otik and Tika, Haven's Seeker Guard, Darken
  Wood's Forestmaster, the Tower's Garrison Knight, Ice Wall's own young
  Knight, a wary Silvanesti Sentry on Southern Ergoth's coast, Silvanost's
  Warder, Kalaman's City Watchman, Palanthas's Astinus and Knight of the
  Watch, and a deserting soldier amid Neraka's own wreckage).
- Dialogue is grounded in the original DL1-3 adventure modules and, for
  the Heroes of the Lance, the Chronicles/Legends novels: real reactive
  dialogue based on your own race/class/alignment, branching topics to ask
  about, and real memory of whether you've spoken before — carried
  whether you met them out in the open or found them indoors.
- **Ask about anything**: every one of the 8 Heroes can be asked about
  anything by typing it rather than only picking from the topic menu —
  real subjects (self-identity, an opinion of each other Hero) get a
  real, in-character answer, anything else gets an in-character
  non-answer instead of a menu that simply doesn't offer it. Raistlin's
  own pool goes deepest, with real content at any of his eight stops: his
  eyes, his golden skin, the Test, the Staff of Magius, his family,
  Kitiara (who gets a different answer at the Inn than everywhere else).
- Every talkable zone-native NPC (Otik, Tika, Astinus, every zone-native
  guard/knight/warder, and more — 22 in all) has the same "ask about
  anything" ability, with two subjects each drawn from their own
  established voice.
- **Astinus** is the one exception, rebuilt with no topic menu at all and
  no suggested-keywords list — his pool (48 subjects) is meant to feel
  genuinely unbounded, so the prompt is just a blank line to type into,
  covering not just Krynn-wide lore (the Cataclysm, the gods, the
  Knights, the Towers of High Sorcery, Huma, draconians, the elven split)
  but every Hero of the Lance, Laurana, Kitiara, Verminaard, and every
  place on the overworld map.
  - Ask him more than five questions in a day and he'll decline to answer
    any more until the next one — unless he judges you worth the extra
    time first (a real Intelligence-and-Wisdom check can buy five more,
    up to a hard cap of ten).
  - Push him to confirm or deny he's the god Gilean, though, and he
    refuses outright, ends the conversation on the spot, and won't grant
    another audience for the rest of that day — same as running out of
    questions the ordinary way. Try walking back into the library after
    either kind of cutoff and one of his Aesthetics turns you away before
    you even reach him. Asking about Gilean as a topic still works fine;
    it's the direct challenge to his own identity he won't tolerate.
- Arrive somewhere after the Heroes have already moved on and it shows: on
  thirteen POIs now (Otik at the Inn of the Last Home, Haven's Seeker
  Guard, Xak Tsaroth's Ruin-Scavenger, Qualinesti's Elven Sentinel, Darken
  Wood's Forestmaster, the Tower's Garrison Knight, Ice Wall's Young
  Knight, the Silvanesti Warder, Palanthas's Knight of the Watch,
  Kalaman's City Watchman, Pax Tharkas's Fortress Guard, Tarsis's Old
  Sailor, and Neraka's Deserting Guard), each has their own thing to say
  the first time you talk to them once the Heroes' stay there has passed,
  even if you'd already met them before the Heroes ever arrived.

### Sea travel

*(Sea travel now works in both builds — the SFML build's Talk offers the
same Board/Not yet choice the console build does.)*

Some places — Ice Wall Castle, Southern Ergoth, and Sancrist Isle — sit on
their own sea-locked landmasses with no road to them at all.

- Talk to the Knight's Runner in Tarsis for passage to Ice Wall.
- An Ice Barbarian Guide at Ice Wall offers passage onward toward Sancrist
  (though the crossing doesn't go as planned).
- A Silvanesti Sentry on Southern Ergoth offers passage on to Sancrist
  proper.
- An Embarkation Officer there offers passage on to Palanthas.
- Flotsam's own Harbor has a similar offer from a different, plainer
  trader — an ordinary run north to Kalaman around the cape (not the
  doomed ship the Heroes' own history sailed on).

Each leg is a ship's voyage of a few days rather than a tile-by-tile walk
across open water. You can say no and ask about it first — the offer
stands until you board, and (unlike a one-way passage arranged for you)
comes back again even after you've sailed it once.

### Companions

*(The SFML build's dialogue now offers recruitment too, confirmed working
at the keyboard — the same as described below. A companion, however
recruited, fights alongside you in SFML combat, sweep/backstab included.)*

- Solace and Haven each have a would-be companion of their own: talk to
  **Bren Alder** in Solace or **Dessa Corrin** in Haven and they'll ask to
  join you (or you can put them off for later) — the start of a real
  party, and you can recruit both, in either order.
- Each companion shows up on your character sheet and status panel, and
  actually fights alongside you: they take their own spot on the tactical
  combat grid and attack automatically each round (no orders to give them
  yet — that's still ahead). Monsters may go after any one of you, and a
  real knock can put a companion down for the rest of that one fight
  without ending it — healed back up the same way you are, by resting or
  a real bed.
- A Fighter adjacent to two or more weak enemies at once (goblins,
  kobolds, hobgoblins, skeletons) automatically **sweeps** and hits them
  all in one round instead of picking just one.
- A Thief standing on the exact opposite side of a monster from whichever
  ally engaged it first lands a **backstab** — better odds to hit and
  real bonus damage.
- Both work the same whether it's you or a companion doing the swinging.

### Random encounters & combat

- Traveling the wilds risks a random encounter — goblins, kobolds,
  hobgoblins, wolves, giant spiders, bugbears, ogres, gnolls, ghouls,
  skeletons, zombies, Baaz/Kapak/Bozak/Sivak/Aurak draconians, or Thanoi
  (Icewall Glacier's walrus-men) — all sourced from a real 2e Monster
  Manual and, for the Krynn-specific draconians and Thanoi, Dragonlance
  Adventures (no orcs, since Krynn has none).
- Fourteen of the lower-danger monsters (goblins, kobolds, hobgoblins,
  wolves, bugbears, gnolls, ghouls, skeletons, zombies, Baaz draconians,
  worgs, black bears, lizard men, giant toads) turn up in real,
  book-sourced numbers rather than always solo, each shown with its own
  HP and a letter (Goblin A, Goblin B, ...) so you can pick a target when
  more than one's still standing.
- Fights play out on a real bordered **tactical grid** (`w`/`a`/`s`/`d` to
  move), captioned with the terrain you're fighting on, sourced from an
  actual SSI Gold Box *Dark Queen of Krynn* manual:
  - Melee attacks need you adjacent to your target.
  - A Tinker's Light Crossbow can shoot anyone on the board until an
    enemy closes in (then it's refused outright).
  - Monsters close the distance when they're not next to you, and pulling
    back from an adjacent enemy risks a free opportunity attack.
  - Choosing a target happens right on the grid — the enemy under the
    cursor is bracketed (`[A]`) and marked in the HP list.
  - Combat shows a real command row of what's legal that round (`ATTACK`,
    `MOVE`, `CAST`, `USE`, `FLEE`) instead of bare hotkey hints.
- Three of the draconians fight back with real, book-sourced abilities
  beyond a plain weapon swing: Bozaks sometimes cast Magic Missile instead
  of attacking, Auraks sometimes breathe a noxious cloud (save for half
  damage, or take full damage and fight on blinded), and Sivaks burst into
  flame with one last retaliatory hit as they fall.
- The chance of a random encounter varies by terrain (roads safest,
  forest/mountains riskiest), and which monster you draw leans toward
  that terrain too (Bugbears more common in hills and mountains, Gnolls
  never on salt flats, Thanoi more common on glacier, and so on).
- Resolved with real 2e attack/damage math — the combat log shows that
  math for every weapon swing (natural-roll, THAC0/AC, and damage-die
  breakdown, not just the hit/miss result).
- `Enter` to attack, `f` to flee, and a Mage or Cleric can also `m` to
  cast (see "Magic" below). Losing just knocks you out and sends you back
  to Solace — it isn't permadeath. See `docs/COMBAT_NOTES.md`.

### Magic: spells & items

- A Mage or Cleric can press `m` mid-fight to cast from a real
  multi-level spellbook — 50 spells across Mage's 9 levels and Cleric's
  7, sourced from an official TSR/SSI Dragonlance computer game manual
  and cross-checked against the actual PHB (damage, healing, blocking a
  monster's attacks, or a this-fight to-hit/AC buff or debuff, depending
  on the spell). With exactly one spell left it casts directly; with more
  than one, an in-frame chooser asks which (same for `i`'s
  Potion/Webnet/Brooch of Imog/Staff of Curing when more than one is
  usable — the grid, HP list, and log all stay on screen for every
  choice).
- **Fireball and Delayed Blast Fireball are real area attacks**: pick a
  target on the tactical grid the same way as any other spell, and every
  enemy near that point takes the blast too, not just the one you aimed
  at.
- Every shop carries a **Potion of Healing** (2d4+2 hp, 200 stl,
  DMG-sourced and priced) — framed as a scavenged pre-Cataclysm relic
  rather than a merchant's own brew, since real clerical healing magic
  doesn't return to Krynn until Goldmoon's Disks of Mishakal early in the
  story. Drink one from the inventory screen (`i`, `Enter`) or mid-fight
  (`i` again, spending your round on it instead of attacking).
- Most shops also carry a **"+1" enchanted weapon**, one per class (an
  Ensorcelled version of your class's own upgrade weapon) — sourced from
  the DMG's magic-item tables and Dragonlance Adventures' own "Magical
  Items of Krynn" chapter, and the first thing in the game to add a real
  to-hit bonus beyond Strength.
- At Solace's General Store, or any of the magic shops, a Mage can also
  buy a **Webnet** (negates a foe's next attack) or a **Brooch of Imog**
  (blocks every attack for the rest of a fight, once per day) — both used
  the same way as drinking a potion mid-combat.
- A Cleric who earns it can wield the **Staff of Striking/Curing**
  instead: a permanent +3 weapon that also calls on a once-per-day
  self-heal, mid-combat, the same way.
- A Strength-13+ character who earns one from Ice Wall Castle's Young
  Knight wields a **Frostreaver** — a heavy battle axe of Icewall Glacier
  ice, "+4" to hit and damage per Dragonlance Adventures, but only while
  actually standing on glacier terrain; carry it anywhere else and it's
  just an ordinary axe, same as the book's own "melts above freezing"
  weakness implies.

### Resting & recovery

- Press `r` to rest, once per in-game day: it heals 1 hit point (the
  DMG's real natural-healing rate) and, for a Mage or Cleric,
  re-memorizes their standing spell loadout for the day (asking first if
  you'd rather choose a new one) — no slots are available at all until
  you have, sourced from the PHB's actual memorization/prayer rules. See
  `docs/CHARACTER_NOTES.md`'s "Spellcasting" section.
- A real bed heals faster: press `z` on the Inn of the Last Home's
  upstairs landing to fully heal overnight instead, the same 8 hours as
  ordinary rest. See `docs/CHARACTER_NOTES.md`'s "Rest and spell
  memorization" section.

### Shops & equipment

Winning fights earns Steel Pieces (Krynn's own post-Cataclysm currency,
not gold) and experience. Press `p` at a shop to buy real gear — every
town now has both a weapons/armor shop and a magic shop (thirteen shop
POIs across all nine towns in total), each with its own distinct catalog
rather than one shared list:

- **Solace**: General Store (the flagship, everything below, doubling as
  both roles at once) and Flint's Smithy (armor/weapons only, locked
  until you deliver ore for the `ore_for_the_forge` quest)
- **Haven**: Market Stalls (Leather/Studded Leather/Hide armor and a
  potion), its new Farrier's Forge (full armor/weapons), and Relic
  Peddler's Cart (magic goods)
- **Tarsis**: Old Sailor (a potion and a salvaged enchanted weapon) and
  its new Scrap-Iron Forge (full armor/weapons)
- **Kalaman**: Market Square (Leather/Studded Leather/Hide/Chain armor, a
  weapon upgrade, a Hoopak for Kender, a potion) and its new Curiosities
  Cart (magic goods)
- **Palanthas**: Harbor (all seven armor tiers, an enchanted weapon, a
  potion) and its new Garrison Armorer (weapons/armor, including the
  weapon upgrade the Harbor doesn't sell)
- **Port O'Call**: new Netmender's Forge (weapons/armor) and
  Beachcomber's Stall (magic goods)
- **Crossing**: Quay (weapons/armor) and new Waiting Merchant (magic
  goods)
- **Port Balifor**: Pig & Whistle (magic goods) and new Smuggler's Stall
  (weapons/armor)
- **Flotsam**: Back Alley (weapons/armor) and Saltbreeze Inn (magic
  goods)

The last three reuse already-written POIs in towns previously left
shopless on purpose — a ferry waypoint, a draconian-guarded harbor, a
smugglers' haven — reframed as black-market/smuggler commerce rather than
open storefronts, which fits each town's character better than a
contradiction of it.

- Where armor's on offer you can buy real Leather/Studded Leather/Hide
  Armor/Chain Mail/Splint Mail/Plate Mail/Field Plate (Mages and Tinkers
  can't wear armor at all, per the PHB's own rule) and a weapon upgrade —
  every class has one now, down to a Mage's Quarterstaff and a Tinker's
  Light Crossbow.
- A Kender character starts equipped with a **Hoopak** instead of their
  class's usual starting weapon — a real sling-staff sourced from the
  *Dark Queen of Krynn* computer game manual (neither core rulebook stats
  it) — and can still buy their class's own weapon upgrade later, since
  the two aren't mutually exclusive; shops also sell a Hoopak outright,
  for the rare case a Kender needs a replacement.
- Press `i` inside the shop to switch to selling gear back for half its
  price.
- Purchases land in a real carried inventory rather than being worn
  automatically — press `i` outside a shop to see what you're carrying
  and equip it, which actually changes your AC and damage in the next
  fight, swapping whatever you had on back into your pack rather than
  losing it. See `docs/CHARACTER_NOTES.md`'s "Equipment" section.

### Leveling

Enough experience means real leveling — more hit points, a better THAC0,
better saving throws, all sourced from the PHB's level-by-level tables (a
Knight of the Crown gets a nod toward the Order of the Sword at 3rd level,
and a Mage feels, faintly, that something has taken notice of them). See
`docs/CHARACTER_NOTES.md`.

### Quests

*(The SFML build doesn't track quest state at all yet — its `G` journal
says so plainly, quest-giver dialogue logs a placeholder line instead of
offering one, and one quest-locked shop shows its own placeholder rather
than resolving the lock. Quests work fully in the console build.)*

Press `g` at any time to check your quest journal, and talk to a
quest-giver to be offered one, track its progress, and turn it in for a
reward — the moment every objective's actually done, the game says so
itself ("...is ready to turn in -- return to..."), so you never have to
guess or walk back speculatively. Nineteen ship so far:

- **Solace's Notice Board** offers a bounty to clear three timber wolves
  off the south road.
- **Otik** at the Inn of the Last Home, the **Garrison Knight** at High
  Clerist's Tower, and **Kalaman's City Watchman** each have their own
  reason to send you somewhere or against something.
- The **Silvanesti Warder** will only speak of hers to a fellow Elf.
- Once a **Knight of the Crown** has proven themselves, High Clerist's
  Tower's **Sword Knight** can sponsor real advancement into the Order of
  the Sword; once you've reached that Sword rank, the Tower's **Knight of
  the Circle** can grant real Solamnic Armor (AC 0, sourced directly from
  Dragonlance Adventures); at the top of the chain, a **Rose Knight** at
  the same Tower can name a proven Sword Knight into the Order of the
  Rose, Solamnia's highest rank.
- Back in Solace, **Flint Fireforge's smithy** — pure scenery until now —
  has a journeyman who'll ask you to fetch raw ore from Pax Tharkas's
  contested Tharkadan Mine and actually carry it back, this project's
  first quest built around a real, granted-in-the-world object rather
  than a place visited or a foe slain.
- A **Ruin-Scavenger** picking through the sunken ruins of Xak Tsaroth has
  a priest's rod they can't use and would rather see go to someone who
  can, if a Cleric first clears out what's nested in the old well shaft
  below.
- **Ice Wall Castle's Young Knight** has a Frostreaver none of his own
  company is strong enough to lift, salvaged off a dead Ice Folk raider,
  theirs for the taking if they're strong enough to wield it and willing
  to thin the thanoi still testing the castle's gate.
- A displaced farmer sheltering in **Thorbardin**, driven from Pax
  Tharkas, would trade real seed grain for a real shot at planting
  something come spring — a Farmer's Cart in Haven has more than its own
  fields will use this season, if you're willing to carry a sack the
  distance.
- **Port O'Call's Beachcomber** swears not everything the strait washes
  up is junk, and wants back whatever the storm-wrack down the shore is
  still hiding.
- **Kalaman's Curiosities Cart**, a war widow trading in whatever
  refugees will part with for steel, wants back the one thing she never
  meant to sell — her late husband's wedding band, lifted off her own
  cart by a trader working the market's fringes.
- Back in Solace, an unremarkable **hooded figure** at the corner of the
  square has nothing to say to most travelers at all, but a Mage who's
  reached 3rd level finds them waiting with a different offer entirely —
  the Tower of Wayreth itself moves through its forest at its own
  pleasure and can't be walked to on any map, so this is the only way
  there. What Palanthas's Great Library still remembers of the old paths
  is only the opening move; the Conclave's own trial, met alone and in
  earnest combat, still has to be lived through before any Robe is
  earned.

See `docs/QUEST_NOTES.md` for the full design, what ships, and how more
get authored.

---

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

This configures and builds every target, including the primary
`ansalon_sfml_phase1`, which lands at
`build\Debug\ansalon_sfml_phase1.exe`. Run it with no arguments to get its
own save-slot menu and character creation wizard, entirely in the window:

```powershell
.\build\Debug\ansalon_sfml_phase1.exe
```

(A save path is still accepted directly too, for a quick dev launch
against a known save: `.\build\Debug\ansalon_sfml_phase1.exe
build\Debug\save1.txt`. A post-build step keeps `data/` and the reference
map image populated next to the exe automatically, so this needs no extra
step.)

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

To hand a runnable copy to someone who doesn't have this source tree, run:

```powershell
powershell -File tools\package_playable_release.ps1
```

This builds the graphical exe and stages it with the data/map it needs
and a `README.txt`, into `dist\AnsalonRPG-Playable-v<N>.zip`. The
recipient unzips and double-clicks `ansalon_sfml_phase1.exe` directly —
no wrapper script needed, since it takes no required argument anymore and
Explorer already sets the working directory to the exe's own folder — its
own save-slot menu and character creation wizard handle everything from
there, entirely in the window. The MSVC runtime is statically linked, so
no separate Visual C++ Redistributable install is needed. Saves persist
normally across relaunches.

### Legacy console build (`ansalon_rpg`)

The original ASCII/terminal build still builds alongside the SFML target
above and lands at `build\Debug\ansalon_rpg.exe`. It locates
`data/locations.txt`, `data/overworld.grid`, etc. (and up to three
save-slot files) next to itself — its own post-build step keeps a `data/`
copy there automatically.

To hand a playable build of this version to someone who doesn't have this
source tree, run:

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

The primary build is `ansalon_sfml_phase1` — real pixel-space rendering,
no terminal required. Run it with no arguments and it opens its own
save-slot menu, where you can create a brand-new character (name, race,
class, ability scores, alignment, etc., all as graphical pickers/text
entry) or continue an existing one, entirely in the window:

```powershell
.\build\Debug\ansalon_sfml_phase1.exe
```

It autosaves back to whichever save file you picked/created (see the note
near the top of this file). A save path is still accepted directly too,
for a quick dev launch against a known save
(`.\build\Debug\ansalon_sfml_phase1.exe build\Debug\save1.txt`), skipping
the slot menu entirely.

Movement is immediate — no Enter key needed:

- **Move**: `W A S D` or arrow keys, 4 cardinal directions (no diagonals)
- **Enter** — step into/out of a location's walkable interior, or attack
  in combat
- `T` — talk to whoever's here (a canon character the timeline places at
  your current location today, or a talkable zone NPC — a picker asks
  who first if more than one is present). Covers greetings, repeat-visit
  lines, aftermath/anticipation text, the topic picker, free-text "Ask
  about something else...", boat-voyage accept/decline (Board/Not yet),
  and companion recruit accept/decline (Join me/Not yet); quest offers
  still log a placeholder line instead of actually opening.
- `P` — browse/buy at a shop POI; `I` while inside toggles to selling.
  Purchases land in your carried inventory, not straight onto your body.
- `I` (outside a shop) — view carried items; Enter equips a weapon/
  armor/shield, drinks a Potion, or explains why a combat-only item
  (Webnet, Brooch of Imog) or quest item can't be used here.
- `C` — character sheet (any key dismisses it); a Mage or Cleric gets an
  `S`/Down option there for the full spellbook.
- `V` — full scrollable event log, up/down to scroll, `V`/`Q` to return.
- `G` — quest journal. This build doesn't track quest state yet, so it
  says so plainly rather than showing anything.
- `O` — read-only World Map: the real reference map scaled down, a
  marker at every location plus your own position, and a side legend.
- `/` — help screen (command reference).
- `R` — rest, once per in-game day: heals 1 HP and, for a Mage/Cleric,
  walks through re-memorizing spells for the day.
- `Z` — bed rest, standing on a real bed inside a zone (e.g. the Inn of
  the Last Home's upstairs landing): a full heal instead of 1 HP,
  otherwise the same as Rest.
- `Q` or Escape — asks "Are you sure you want to end your adventure?"
  rather than quitting immediately.
- Look (`L`) isn't implemented in this build yet.

In combat: **Enter** attacks whoever's under the grid cursor, `W A S D`
moves on the grid, **`M`** casts a memorized spell (asks which, if more
than one memorized), **`I`** uses an item — Potion/Webnet/Brooch of
Imog/Staff of Curing (asks which, if more than one usable), **`F`**
flees. A Fighter adjacent to 2+ weak enemies sweeps automatically; a
correctly-positioned Thief backstabs automatically — no key needed for
either. Losing a fight knocks you out and sends you back to the nearest
refuge rather than ending the run.

### Legacy console build

Run the built `ansalon_rpg.exe`. It shows a menu of 3 save slots (each
autosaved continuously during play — see `docs/ARCHITECTURE.md`), any
occupied ones summarized by name/level/race/class/day; pick one to
continue that character (with a confirmation before a fresh character is
allowed to overwrite it), type `d1`/`d2`/`d3` to delete a slot's save
immediately (with its own confirmation), or pick an empty slot to open
character creation
(plain typed prompts — enter a name, keep or reroll your ability scores,
pick a race/class/alignment number, confirm). The primary SFML build now
has its own equivalent save-slot menu and character creation wizard (see
"Playing" above) — this console version is an independent, still-fully-
working alternative, not a required first step anymore. Once character
creation's done, movement is immediate — no Enter key needed:

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
- `p` — browse/buy at a shop POI (thirteen now, across all nine towns,
  each with its own catalog — see "Status" above) — up/down selects an item, Enter
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
- `o` — view a read-only "World Map" overview of the whole continent,
  downsampled to fit the screen with a side legend listing every
  location by name (any key dismisses it) — purely a reference view, no
  way to travel or warp from it
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
