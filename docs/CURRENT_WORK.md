# Current work

**Nothing in flight.** The quest system's engine-confirmation checklist
closed 2026-09-15: all three tracked quests (`road_wolves`, `a_widows_due`,
`ore_for_the_forge`) are now confirmed live end-to-end on the SFML build —
see `docs/QUEST_NOTES.md` for each quest's own writeup. Everything else
recently shipped is verified and handed off; see `docs/MILESTONES.md` for
the shipping history.

**Concrete open item**: the quest system's progress-text revisit and its
six reward flags — especially the Wayreth Test of High Sorcery's
three-outcome ethical-choice scene. This is content/data work, not engine
work — research the sourced material first, per the usual workflow.

A real rendering bug found live 2026-09-16 while playtesting Milestone
180 (text clipping mid-word in the character-creation wizard and
save-slot menu) was fixed the same session — see Milestone 193 in
`docs/MILESTONES.md` for the root cause and fix; confirmed live.

**Line of sight (Milestone 189) fully confirmed live 2026-09-16** — both
halves left open after Milestone 193 (the blocked-shot refusal message
and the Fireball epicenter's LOS-vs-splash behavior) were witnessed this
session; see `docs/COMBAT_NOTES.md`'s Line of Sight section for exactly
what was seen. Nothing left open for this milestone.

Otherwise, see the Playtest backlog below for what's implemented and
verified (clean rebuild + smoke test) but not yet walked with a real
keyboard, and the two Parked sections for long-shelved decisions with
their own resumption notes. Don't resume either parked topic, or the
console-retirement conversation (`docs/CONSOLE_RETIREMENT_PROPOSAL.md`),
unprompted — wait for the user.

## Playtest backlog

Implemented and verified via clean rebuild + launch smoke test, but not
yet fully walked live with a real keyboard. Full sourcing/detail for each
is in its `docs/MILESTONES.md` entry (linked below) — this list only
tracks what's still open and how to force it.

- **Bigger battlefield + real movement** (Milestone 185) — still open:
  movement actually running out mid-round (refusing a further step with
  "You have no movement left this round"); monster/companion AI closing
  distance at their own differing rates (a fast Wraith/Spectre vs. a slow
  Zombie/Mummy/Boring Beetle, easiest to force by fighting each with a
  fresh character on open terrain).
- **VIEW command + status tags** (Milestone 186) — still open: a Status
  line actually appearing on a viewed character's card (needs a
  debuff/buff active — memorize/cast Slow or Hold Monster and check both
  the card and the sidebar roster line for the same tag); Escape/Q
  cancelling the picker without consuming a round.
- **Battlemap walls + wall-aware pathing** (Milestone 188) — still open:
  a diagonal corner-cut against a wall specifically logging "Blocked:
  can't cut across the wall." (needs a corner-shaped gap, not a solid
  rectangle); monster/companion AI visibly detouring around a wall
  cluster (a held-in-place retry was inconclusive — needs a monster that
  has to route around, not just toward, a wall); the remaining 5 of 9
  terrains (bog, salt flat, savannah, glacier, road).
- **Multi-square creatures + Blue Dragon** (Milestone 190) — not yet
  interactively confirmed at all. Needs an Ogre/Troll (1x2), Griffon
  (2x1), or the Blue Dragon (2x2, salt-flat terrain — which doesn't
  currently appear anywhere in the generated grid, see `docs/MAP_NOTES.md`
  — or a debug/dev save placed near one) to actually come up in combat:
  confirm the marker renders multi-cell (not just its anchor), the
  footprint can be walked around without overlap, melee/ranged/
  opportunity-attack targeting works from any side of it, a wall-blocked
  shot refuses via its nearest visible edge, and the Dragon's lightning
  breath fires ~30% of the time with real damage.
- **Look command** (Milestone 182) — still open: `'l'` with an overworld
  NPC/canon Hero actually present, or a `TIMELINE_ANCHOR` tile with one —
  needs a canon Hero to actually be nearby per the timeline schedule.
- **Native character creation** (Milestone 180) — **mostly confirmed live
  2026-09-16**, driven via SendKeys/screenshot against a disposable test
  character in the previously-empty Slot 3 (Mike's and Regan's real
  Slot 1/2 saves were never touched): the final summary's "No" restart
  path correctly resets the wizard all the way back to a blank Name
  step; the Elf/Dwarf subrace step correctly gates Silvanesti/
  Qualinesti/Kagonesti Elf and Hill/Mountain Dwarf by the rolled scores
  (same inline "your ability scores don't qualify" pattern as the race
  step, confirmed on both a Dwarf and, on a second roll, an Elf); and
  the save-slot menu's overwrite branch (List -> occupied slot -> "Continue
  X?" -> No -> "Start a new character in Slot N? This will overwrite...”
  -> No safely backs out to the list with the slot untouched) all work
  correctly. Also incidentally confirmed the delete-confirmation branch
  (used to clean up the disposable test slot afterward). Still open, by
  the user's own request this session (skipped deliberately, not
  forgotten): the Gnome-forced-Tinker path specifically. The same live
  session also found and fixed a real text-clipping bug in this wizard
  and the slot menu — see Milestone 193 in `docs/MILESTONES.md`.
- **Stale startup banner fix** (Milestone 181) — not yet interactively
  confirmed (no desktop/GUI access the session it shipped in).
- **Combat spellcasting** (Milestone 169) — still open, hardest to force:
  get monsters to act first (retry until initiative favors them) on a
  round where `M` is pressed, and confirm a knockout that round means the
  spell was never actually cast (still shows as memorized afterward).
- **Combat item use** (Milestone 170) — not separately confirmed: the
  2+-item picker and its Escape/Q cancel, Staff of Curing (no current
  save carries one), the Brooch's once-per-day gate and companion
  exclusion, and a monsters-act-first knockout preventing an item from
  being consumed.
- **Thief backstab** (Milestone 171) — still not interactively confirmed
  (Fighter sweep half is confirmed, see Milestone 171). Needs a
  Thief-type party member in light-or-no armor (e.g. Dessa Corrin at
  Haven, once recruited) positioned opposite whoever first attacked an
  instance; confirm "Backstab! " logs with a visibly larger damage
  number, and no bonus from any other square or in heavier armor.
- **Same-cell combat-movement collision fix** (Milestone 172, both
  builds) — not yet re-confirmed live for the specific case: retreat from
  an adjacent monster on a round where it can close the distance,
  confirm no overlap.
- **Dialogue** (Milestone 161) — the quest placeholder log line at a POI
  marked `QUEST` (e.g. Kalaman's Curiosities Cart) not yet separately
  confirmed.
- **Ask-input** (Milestone 162) — still open, needs Astinus specifically:
  `ASK_LIMIT_LOCKED`'s greeting override on a same-day return visit, and
  the extension-roll *fail* path (pure chance which branch fires live).
- **Inventory** (Milestone 165) — still open: the Webnet/Brooch of
  Imog/quest-item no-op message (no quest item currently carried).
- **152** — Fireball/Delayed Blast Fireball's area-damage math is proven
  on the SFML build. Still open, low priority: walking the *console*
  (`ansalon_rpg`) build's own `pickTarget` epicenter-picking UI, which
  hasn't been exercised at all (same standing `_getch()` limitation).
- **Astinus SUBJECT topics, day-gated** — ask Astinus (Palanthas) about
  each before/after its gate day to confirm both halves read correctly:
  Kagonesti/gnomes/gully dwarves/minotaurs/ogres-Irda/Reorx (137/138/139,
  no specific gate), Fizban (before/after day 192), Silvara (day 69),
  Berem/the Everman (day 193), Gilthanas (day 69), Lord Soth (day 190),
  Ariakas (day 193), Alhana Starbreeze (day 51), Porthios (day 70).
- **Griffon/Stirge encounters** (Milestone 142) — Harpy is confirmed live
  (2026-09-15); Griffon (hills/mountains) and Stirge (forest) still
  haven't come up — trigger wilderness encounters there.
- **False "ocean" pockets inside Qualinesti/Silvanesti forest** — fixed
  (see `docs/MAP_NOTES.md`'s "Fixing false 'ocean' pockets inside
  forest"), not yet walked live. Worth deliberately walking north and
  west from Regan's saved position (`save2.txt`, `POS 176 214`), and
  generally through Qualinesti near Bianost/Dark Tower, to confirm no
  more spurious "Blocked: cannot walk onto the ocean." Data-only change —
  applies to `ansalon_rpg` too, not just the SFML build.

## Parked: Dragonlance Adventure modules as full quests

**Raised 2026-09-11, backburnered before any planning/code.** The user
asked about incorporating the classic DL adventure modules (`References/`
already has DL1 *Dragons of Despair*, DL2 *Dragons of Flame*, DL3
*Dragons of Hope*) as full in-game adventures/quests, not just sourcing
flavor text.

Research done before parking, so a future session doesn't have to
re-derive it:

- **Real design tension**: `docs/QUEST_NOTES.md` records that canon
  Heroes are deliberately never quest-givers — they're "weather," and
  the player character isn't one of them. The DL modules' text is
  written to literally *be* played as the Heroes' own party (built-in
  pregens, Goldmoon/Riverwind joining mid-module) — a faithful port
  would mean either breaking that rule or having the player reenact
  Tanis's party's own documented plot beats. Not resolved — the user
  didn't pick a framing (echo/parallel content vs. literal Heroes' path)
  before backburnering.
- **The existing `data/zones/xak_tsaroth.txt` already does the "echo"
  approach well**, unprompted by this discussion: a small 10-POI
  overlook/plaza slice, explicitly commented as "not the full sunken
  dungeon," set *after* the Heroes passed through (a scavenger NPC
  recalls "eight strangers... one coughed something awful... a kender
  went down a stairwell"), with an original `staff_of_striking_curing`
  quest inspired by (not transcribed from) Goldmoon's blue crystal staff
  / the Disks of Mishakal. Good precedent to build from if this resumes.
- **DL1's actual dungeon is much bigger than that stub** — "Lost City of
  the Ancients" / "Descent into Darkness" / "Lair of the Dragon" run to
  60+ numbered areas (draconian patrols, Fewmaster Toede, the dragon
  Onyx/Khisanth's lair), plus a full wilderness-travel chapter before
  even reaching Xak Tsaroth. A faithful full port of just DL1 is a
  multi-session undertaking on its own; DL2/DL3 would each be similar in
  size. Proposed (not agreed) scope if this resumes: one location at a
  time, starting with deepening Xak Tsaroth's existing stub into a real
  multi-room descent (a dozen-plus new POIs, 2-3 sourced monster
  encounters, one substantial original quest chain) as its own
  milestone, rather than attempting the whole 60-area dungeon at once.

Don't resume this unprompted — ask which framing (echo vs. literal) and
which location to start with, same as any other new milestone.

## Parked: SFML rendering + variant tile art (round 2, `sfml-trial-2`)

**Superseded in direction, not necessarily obsolete.** This is round 2's
tile-atlas approach (hand-drawn 32px terrain tiles); the active migration
above instead renders the real map image directly, pixel-space. Still
worth keeping this note around in case tile-based art becomes relevant
again for a screen the real map can't represent (a zone interior, say) —
just don't assume it's the plan for the active migration above.

Not abandoned, not committed to — purely the user's call whenever (or if)
they revisit it. Don't propose pushing this further unprompted; this
project has a real history of visual-change attempts that didn't land
(SFML with no art, twice-rejected ANSI truecolor shading), and this
round's two checkpoints ("looks a bit better", then "good enough, stop
here for now") were engaged with productively but still didn't convert
into a commit-to-it decision.

What exists: an SFML window rendering real overworld data with actual
pixel-art sprite tiles (scrolling camera, per-tile animation) hit and
fixed a real bug (a dangling `std::string&` in a hand-rolled JSON
parser). On top of that, Python/Pillow mockups added 6 art variants per
terrain code (forest, grassland, hills, mountains, savannah, bog, salt
flat, glacier), picked per-tile via a stable position hash so large
same-terrain regions — bigger now after Milestone 146's smoothing — stop
reading as repeating wallpaper.

To resume:

- SFML trial code: `git checkout sfml-trial-2 && git stash pop`
  (uncommitted `CMakeLists.txt` target `ansalon_sfml_trial` +
  `sfml_trial/main.cpp`).
- Variant art already exists and is proven, just not wired in:
  `References/variant_tiles6.png` (forest/grassland/hills) and
  `References/variant_tiles_extra.png`
  (mountains/savannah/bog/salt-flat/glacier), each a 6-column strip; the
  position-hash selection logic (`(x*73856093) ^ (y*19349663)`) is
  proven in the mockup script. Porting both into `terrain_tileset.json`'s
  schema (single rect per code -> list of rects) and
  `sfml_trial/main.cpp`'s tile lookup is the concrete next step, not a
  re-derivation.
