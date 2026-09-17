# Current work

**Nothing in flight.** Milestone 204 (real DQoK Aim -- a free-look
targeting cursor -- plus auto-advancing round narration,
`ansalon_sfml_phase1` only) shipped 2026-09-16, fixing a real combat-feel
bug the user hit live: pressing `a` (Attack) while nothing was adjacent
used to still consume the round (letting monsters close in for free)
because initiative was rolled before checking for a legal target; on top
of that, every round's narration needed an Enter press per line. Both
fixed: `a` now opens a genuine zero-cost free-look cursor that only spends
the round once Enter confirms a legal target, and an ordinary round's
narration now auto-plays, stopping for input only at real decision points
(your turn, the encounter-open beat, and Won/Lost/Fled). Verified live
end-to-end via SendKeys/screenshot on a disposable save copy (real
`save1.txt` confirmed untouched by mtime) through a full 4-Skeleton fight,
including sweep re-triggering correctly and auto-advance correctly
stopping the instant the fight-ending round resolved. Full writeup:
Milestone 204 in `docs/MILESTONES.md` and its own section in
`docs/COMBAT_NOTES.md`. Not yet witnessed live: the ranged-weapon
(crossbow) half of the new Aim legality check — see the Playtest backlog
below.

Everything else recently shipped is verified and handed off — see
`docs/MILESTONES.md` for the full shipping history (each numbered entry
has its own writeup; this file tracks only what's still open, not a
running log).

**Concrete open item**: the quest system's progress-text revisit and its
six reward flags — especially the Wayreth Test of High Sorcery's
three-outcome ethical-choice scene. This is content/data work, not engine
work — research the sourced material first, per the usual workflow.

See the Playtest backlog below for what's implemented and verified (clean
rebuild + smoke test) but not yet walked with a real keyboard, and the two
Parked sections for long-shelved decisions with their own resumption
notes. Don't resume either parked topic, or the console-retirement
conversation (`docs/CONSOLE_RETIREMENT_PROPOSAL.md`), unprompted — wait
for the user.

## Playtest backlog

Implemented and verified via clean rebuild + launch smoke test, but not
yet fully walked live with a real keyboard. Full sourcing/detail for each
is in its `docs/MILESTONES.md` entry (linked below) — this list only
tracks what's still open and how to force it.

- **Real DQoK Aim cursor + auto-advancing narration** (Milestone 204) —
  confirmed live end-to-end (free-look/zero-cost, live status text, Escape
  cancel, commit/sweep, auto-advance, and Won staying manual — see that
  milestone's own writeup for the full list). Still open: the
  ranged-weapon (Light Crossbow) half of the new `combatAimLegality`
  check — "Nothing in your line of sight." and "An enemy is too close to
  fire your crossbow!" while Aiming — implemented but not eyeballed live
  (no current save carries a crossbow; would need one added to a
  disposable test save, or a fresh character built with one).
- ~~**Gold Box UI font** (Milestone 196)~~ — now fully closed. Confirmed
  live on the save-slot menu, the full character-creation wizard through
  Summary (Milestone 197), the character sheet + inventory screens
  (Milestone 198), the combat HUD (Milestone 199's live Black Bear fight),
  and, 2026-09-16 (Milestone 202), the last four screens: shop, spellbook,
  journal, help. That same check found and fixed a real stale-text bug —
  the Help screen still taught "wasd = move" after Milestone 200 removed
  WASD movement entirely — see Milestone 202. Nothing left open for this
  item.
- ~~**Combat sidebar companion-line wrap fix** (Milestone 198)~~ — moot:
  Milestone 199 deleted the entire old combat sidebar (roster + log) this
  fix lived in, replacing it with a per-unit card + bottom bar. Companion
  HP is no longer shown passively at all (by design, see Milestone 199) —
  nothing left to verify here.
- **DQoK-style combat HUD** (Milestone 199) and its **key/card/turn-follow
  refinements** (Milestone 200) — confirmed live end-to-end across a solo
  Black Bear fight (199) and a solo Wight fight (200): arrival, the new
  DQoK-lettered Idle command bar (`a`/`c`/`u`/`v`/`f`/`d`), paced hit/miss/
  movement messages, the compact card tracking HP live, the turn-follow
  camera/card correctly handling a monsters-act-first round, and a clean
  `Fled` exit back to the Overworld (confirmed working this session,
  struck from "still open" below). ~~The secondary target card in compact
  format~~ is now closed too — Milestone 204's own live 4-Skeleton fight
  confirmed the Aiming cursor's secondary card (its replacement for
  `PickingTarget`'s old attack-picker card) against a real 2+-monster
  group. Still open: the turn-follow card/camera specifically following a
  *companion's* turn (needs a fight with a living companion in the party —
  Bren Alder was knocked out in the test save used), `c`/`u` (Cast/Use —
  no fight so far has had a caster/item to test with), the new "preparing
  to cast/breathe" beat (needs a Bozak/Aurak Draconian, `MIN_TOWN_DISTANCE
  25`/`45` wilderness), and the `Lost` end state (needs a losing fight or a
  debug/dev save at low HP).
- **Bigger battlefield + real movement** (Milestone 185) — a real
  wilderness encounter (3 Bugbears, hills terrain, spawned at dist 24)
  was fought live 2026-09-16 and confirmed the bigger-battlefield spawn
  distance and monsters closing in over several rounds, but not the two
  specific open sub-items: movement actually running out mid-round
  (refusing a further step with "You have no movement left this round");
  monster/companion AI closing distance at their own differing rates (a
  fast Wraith/Spectre vs. a slow Zombie/Mummy/Boring Beetle, easiest to
  force by fighting each with a fresh character on open terrain).
- ~~**VIEW command + status tags** (Milestone 186)~~ — confirmed live
  2026-09-16: casting Hold Monster on a Gnoll (bog terrain) showed
  `[Held]` on its sidebar roster line and `Status: Held` on its own View
  card, the same tag both places; Escape on the View picker returned to
  Idle combat with zero state change (no round consumed). Nothing left
  open for this milestone.
- **Battlemap walls + wall-aware pathing** (Milestone 188) — hills
  terrain reconfirmed live 2026-09-16 (a real wall-clustered hills
  battlefield fought end-to-end), and bog/glacier/road all newly
  confirmed live 2026-09-16 too (each a real wilderness encounter on
  that terrain, every one rendering multiple wall clusters; bog also
  logged a real `Blocked: cannot walk onto a wall.` refusal). **`salt_flat`
  and `savannah` are confirmed to have zero tiles anywhere in the current
  `data/overworld.grid`** (direct script scan, not inference) — genuinely
  unreachable through any live encounter, not just unconfirmed; their
  `data/battlemaps/` files exist and do carry walls (20/37 tiles) but can
  only ever be seen via a debug/dev save forcing that terrain code. Still
  open, on the 3 reachable terrains: a diagonal corner-cut against a wall
  specifically logging "Blocked: can't cut across the wall." (every wall
  cluster actually encountered this session, on any terrain, was a solid
  rectangle with no corner-shaped gap); monster/companion AI visibly
  detouring around a wall cluster (a held-in-place retry was inconclusive
  — needs a monster that has to route around, not just toward, a wall).
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
- ~~**Look command** (Milestone 182)~~ — confirmed live 2026-09-16:
  teleported into Thorbardin's Great Hall (`TIMELINE_ANCHOR H`) with the
  clock inside the Heroes' real `PRESENCE thorbardin 33 41` window;
  `'l'` listed all eight canon Heroes at once, and selecting Tanis showed
  his exact `PRESENCE` line from `data/timeline.txt` verbatim. Nothing
  left open for this milestone.
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
  (used to clean up the disposable test slot afterward). The same live
  session also found and fixed a real text-clipping bug in this wizard
  and the slot menu — see Milestone 193 in `docs/MILESTONES.md`.
  **The last open piece, the Gnome-forced-Tinker path, was confirmed live
  2026-09-16**: rolling a Gnome showed "As a Gnome, you are a Tinker --
  Krynn's gnomes know no other calling." with no class choice offered.
  Milestone 180 now has nothing left open.
- ~~**Stale startup banner fix** (Milestone 181)~~ — confirmed live
  2026-09-16: the save-slot menu and banner render correctly at full
  window size on launch, no stale/undersized view.
- **Combat spellcasting** (Milestone 169) — general casting confirmed
  live 2026-09-16 (Regan's copied save cast Hold Monster and another
  spell mid-fight; also surfaced real LOS-vs-targeting interaction, "Your
  Hold Monster finds no target in sight." when no enemy was in sight at
  cast time). Still open, hardest to force: get monsters to act first
  (retry until initiative favors them) on a round where `M` is pressed,
  and confirm a knockout that round means the spell was never actually
  cast (still shows as memorized afterward).
- **Combat item use** (Milestone 170) — the Brooch's once-per-day gate
  **confirmed live 2026-09-16**: one in-combat use (Globe of
  Invulnerability, absorbed a Gnoll's hit) set the gate, and a second
  `USE` attempt the same fight/day showed "You have nothing to use."
  Companion exclusion not separately exercised (the only companion
  present was already knocked out, so never got a turn to try). Still
  open: the 2+-item picker and its Escape/Q cancel (no current save
  carries two usable items at once), Staff of Curing (no current save
  carries one), and a monsters-act-first knockout preventing an item
  from being consumed.
- **Thief backstab** (Milestone 171) — still not interactively confirmed
  (Fighter sweep half is confirmed, see Milestone 171), despite a real
  attempt 2026-09-16 (added `COMPANION dessa_corrin` directly to a
  disposable save's companion line — deterministic given the id, same as
  a real recruit — then fought three separate wilderness encounters
  trying to engineer the geometry). **Genuinely hard to force, not just
  unlucky**: Dessa's own AI usually swings at a monster (even a miss
  counts) before the player can reposition to be its first attacker, and
  once she's `firstAttackerId` on an instance she can never backstab
  around her own anchor. The one time the player did land the first hit
  and get positioned opposite Dessa (Mike north, Dessa south of the same
  Gnoll, confirmed by both sidebar positions and dist), the player's own
  hit finished the Gnoll before Dessa's turn came around, so there was
  nothing left to backstab. A 4-Gnoll pile-on afterward got dangerous
  (player 6/26 HP, Dessa 1/6 HP) and was fled rather than pushed further.
  Best forcing strategy for next attempt: a single, tougher (higher-HP)
  monster rather than several weak ones, so the anchor-setting hit
  doesn't also finish it — and ideally engage it somewhere Dessa is
  genuinely delayed (behind a wall corner, across a gap) so the player
  reliably gets the first swing. Incidentally reconfirmed live: the
  sourced "free strike" opportunity-attack-on-retreat message ("As you
  pull back, the Gnoll B gets a free strike! It hits you for 4.").
- **Same-cell combat-movement collision fix** (Milestone 172, both
  builds) — not yet re-confirmed live for the specific case: retreat from
  an adjacent monster on a round where it can close the distance,
  confirm no overlap.
- ~~**Dialogue** (Milestone 161)~~ — confirmed live 2026-09-16, via a
  `QUEST`-tagged POI in Kalaman (the City Watchman, `bazaar_road_raiders`,
  the same pattern as the Curiosities Cart): talking to him produced a
  real, fully working quest offer (Accept/Decline, logged "Quest
  accepted: Raiders on the Bazaar Road."), not a placeholder — the
  original placeholder-line concern turned out to be moot, superseded by
  the quest system shipping in full since Milestone 161.
- **Ask-input** (Milestone 162) — still open, needs Astinus specifically:
  `ASK_LIMIT_LOCKED`'s greeting override on a same-day return visit, and
  the extension-roll *fail* path (pure chance which branch fires live).
- **Inventory** (Milestone 165) — the Brooch of Imog's "That can only be
  used in combat." no-op **confirmed live 2026-09-16** (Regan's copied
  save carried one), and its in-combat once-per-day gate **also confirmed
  live 2026-09-16** (see Milestone 170, below). Still open: the same
  outside-combat no-op for a Webnet or an actual quest item (no quest
  item currently carried in either real save), and the Brooch's companion
  exclusion specifically.
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
  haven't come up — a 2026-09-16 session walked hills terrain near
  Kalaman hunting one (also multi-square-footprint bait, see Milestone
  190 below) but triggered a Bugbear encounter instead. Griffon needs
  `MIN_TOWN_DISTANCE 35`, genuinely far wilderness travel — try again
  further from any town.
- ~~**False "ocean" pockets inside Qualinesti/Silvanesti forest**~~ — fixed
  (see `docs/MAP_NOTES.md`'s "Fixing false 'ocean' pockets inside
  forest"), and **reasonably confirmed live 2026-09-16**: a direct script
  scan of `data/overworld.grid` found zero `~`/`!` tiles within a 30-tile
  radius of Regan's original saved position (`save2.txt`, `POS 176 214`),
  and a live walk north/west from that point (disposable copy) produced
  no spurious "Blocked: cannot walk onto the ocean." (cut short by an
  unrelated real wilderness encounter, fled to end the check cleanly).
  Consider this closed barring a future report of a specific spot still
  showing the bug. Data-only change —
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
