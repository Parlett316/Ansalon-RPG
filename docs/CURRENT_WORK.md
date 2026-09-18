# Current work

**In flight: dialogue portraits for the full canon/companion roster (14
characters), 2-3 expressions each.** Plan so far: one "normal" portrait per
character, plus a tailored second expression for whoever triggers
`SUBJECT_UNKNOWN` (an unrecognized ask-input question) -- not a uniform
"confused" label, but matched per-character to how their own
`SUBJECT_UNKNOWN` line already reads (puzzled for most, irritated for
Raistlin/Flint, unbothered/delighted for Tasslehoff and Fizban, guarded
for Alhana, wary for Silvara, composed for Laurana and Riverwind). Step 1
done: Fizban/Laurana/Alhana/Silvara didn't have a `SUBJECT_UNKNOWN` line
at all yet (the other 8 canon Heroes did) -- added one for each, in-voice,
matching the established pattern (all 4 previously fell through to a
generic system line, "gives you a blank look... I'm not sure what you
mean," not a bug, just impersonal). Verified via a launch smoke test
(`ansalon_sfml_phase1`, Mike's real `save1.txt` -- "timeline loaded"
confirms the new lines parse). Step 2 done 2026-09-17: full portrait
checklist built and filename convention settled -- see
`docs/TOWN_ART_PROMPTS.md`'s "Portrait checklist (full 14-character
roster)". Resolves the plan to 2 expressions per character (one normal +
one `SUBJECT_UNKNOWN`-reaction, mood read off each character's own line,
not a uniform "confused"), 26 new images to source (Otik/Tika's normal
portraits already exist); reaction filename is `<normal-stem>_unknown.png`
(e.g. `tanis_unknown.png`). Step 3 done 2026-09-17: novel-research pass complete, full art-direction
prompts (physical description + normal/reaction expression) written for
all 12 canon Heroes, Astinus (a zone POI missed by the first pass' sweep
of `data/timeline.txt`'s `CHARACTER` roster -- caught when the user
asked, then confirmed no one else in the game's other 29
`SUBJECT_UNKNOWN` lines has comparable depth), and Otik/Tika's reaction
shots -- see `docs/TOWN_ART_PROMPTS.md`'s "Dialogue portrait prompts"
section, sourced from the actual Chronicles/Legends novel text in
`.research/`. Ready-to-paste generator prompts (28 total) also written
to `docs/PORTRAIT_PROMPTS.md`.
**Open decision surfaced by that pass, not yet resolved**: portrait
backgrounds. The 12 Heroes cross 25 distinct zones total between them
(Tanis alone has 16); at the time this was written only 11 of those had
plate art, ruling out a true per-location background matrix. **That
constraint is gone as of Milestone 216** -- every zone now has plate art,
so option 3 (runtime compositing the portrait over the zone's own plate)
is now actually feasible, not just theoretical. Still not decided which
of the three options in that doc's "Portrait backgrounds" section
(generic backdrop / one signature location baked in / runtime
compositing) to pursue -- prompts are written background-agnostic in the
meantime, and the existing portrait art was generated under option 1
(generic backdrop), so switching to option 3 later would mean
regenerating it.
**Not started yet**: picking a backgrounds option (ask the user), then
sourcing the actual portrait art, and the code to select which
expression to show, when -- `loadDialoguePortraitTexture`
(`sfml_phase1/main.cpp:547`) currently loads exactly one static texture
per character id for a whole conversation; nothing yet decides "show the
alternate expression right now."

Milestones 205-212 all shipped 2026-09-17: zone
landmark plates, the Gold Box town menu (originally piloted on Solace
alone, **now auto-derived for any zone with real plate art, confirmed
live end-to-end**), dialogue portraits (redesigned bigger at Milestone
210, then **fixed-size + paginated at Milestone 211** after live feedback
that a long response was shrinking the portrait instead of staying
consistently large — both confirmed live), the town menu's Leave row now
naming the actual parent zone ("Back to Solace" from the Inn, not "Leave
town" — Milestone 212, also confirmed live), and the real-art placement
along the way that found and fixed four real rendering/correctness bugs.
Menu rows are auto-derived from data the zone file already has (one row
per actionable POI, keyed by its own already-declared character —
including, as of Milestone 210, a zone's `TIMELINE_ANCHOR` POI even with
no `TALK` line of its own), and selecting one reuses the exact same
`shopBegin`/`restBegin(true)`/`dialogueBegin` a walking player would
trigger. Deliberate, documented trade-off: inside a menu-town zone, a
letter always means "go there or nothing," so Inventory (`I`)/Journal
(`G`)/Rest (`R`) are unreachable while inside its town screen (its own
destinations use those letters).

**Milestones 213-215 also happened same day, and net out to this final
state**: the non-combat Overworld/Zone screen keeps its right-hand status
card (name/level/race/class, HP, time-of-day, indoor zone name — Milestone
201's original content), reinstated at Milestone 215 after Milestone 213's
bottom-command-bar experiment turned out too cluttered live (status info,
an arrival's message burst, and the command hint line all stacked in one
strip) — the map is back to reserving `sidebarWidth` on the right, no
bottom bar outside combat. Along the way, Milestone 214 found and fixed a
real, **pre-existing** gap unrelated to any of this layout churn: the SFML
build never had console's automatic "`<Hero> is here.`" arrival
announcement at all (`GameLoop::announceOverworldTile`/`announceZoneTile`).
That fix's *substance* survived the 213→215 layout reversal, just moved
surfaces — the status card now also shows a **live** "who's here" line,
recomputed every frame straight from the player's current position (same
"always accurate" idiom HP/time already use), while the original log-side
push (arrival description + Hero lines into the plain scrolling `log`)
stays for the full-log overlay (`v`) and location flavor text. Combat's own
card + bottom bar were unaffected by all three milestones throughout.
Clean rebuild (zero new `/W4`) + launch smoke test against Mike's real
`save1.txt` passed at every step; the final on-screen result (status card
layout/sizing, the live Hero line actually appearing/disappearing
correctly) is **not yet interactively confirmed** — see the Playtest
backlog below.

**Real art now exists for all 29 documented zones + 4 dialogue
portraits**, as of Milestone 216 (2026-09-17): every zone in
`docs/TOWN_ART_PROMPTS.md`'s "Per-location prompts" checklist now has a
real `assets/plates/<zoneId>.png` — the original 12 (Solace, Palanthas,
Kalaman, Neraka, Pax Tharkas, Qualinost, Silvanost, Tarsis, Thorbardin,
Xak Tsaroth, High Clerist's Tower, the Inn of the Last Home) plus 17 more
uploaded and sorted in this session (Crossing, Dargaard Keep, Darken
Wood, Flotsam, Foghaven Vale, Godshome, Haven, Hopeful Vale, Ice Wall,
Mount Nevermind, Plains of Dust, Port Balifor, Port O'Call, Qualimori,
Que-shu, Sancrist Isle, Southern Ergoth). Otik and Tika now have both
portrait expressions (`solace_inn_O.png`/`_unknown.png`,
`solace_inn_Y.png`/`_unknown.png`). Same upload also included 2
byte-identical duplicate files under wrong names, deleted as dead weight
— see Milestone 216 in `docs/MILESTONES.md` for the full sort-out.
**Open QA note, unresolved**: 2 of the 17 new plates (`darken_wood.png`,
`mount_nevermind.png`) have a faint illegible signature-flourish baked
into a shadowed corner — not confirmed with the user, flag if noticed
live. Full writeups: Milestones 205-212 and 216 in `docs/MILESTONES.md`,
the SFML section of `docs/ARCHITECTURE.md`, `docs/ZONE_NOTES.md`'s "Town
menus" (the new `L`/`l`-reservation authoring rule applies to any zone
now, not just explicitly-flagged ones).

**Gotcha worth remembering**: `cmake --build build --target X --clean-first`
cleans the *entire* build tree, not just `X` — building a second target
with `--clean-first` right after silently deletes the first target's
already-built binary. Build every target you need in one invocation (no
`--target`, or list them all) if using `--clean-first`.

The Gold Box town menu's individual destination hotkeys, the Inn round-
trip (Inn ↔ Solace, both directions, including the corrected Leave
label), and Leave are all now confirmed live — see the Playtest backlog
below for the couple of narrower items still open (shop/bed screens
specifically returning to the menu afterward, and the "Nothing here by
that name." message).

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

- **Exploration status card + live Hero-presence line** (Milestones 201,
  reinstated with the new line at 215, superseding 213/214's bottom-bar
  detour) — a live look already caught and fixed two real bugs, both the
  same unwrapped-text class: the name/level/race/class line ran off the
  panel, then (a screenshot with a Day-0 character at Solace, all eight
  canon Heroes' opening windows overlapping at once) every Hero-presence
  line did too. Both now wrap like every other long line in this file.
  Still needs a fuller look in both Overworld and an indoor zone: does the
  status card size and position cleanly at a real maximized resolution
  with both wrap fixes in place, including that all-eight-Heroes case; does
  combat's own card+bottom bar look completely untouched when a fight
  starts right after (confirms the `kCombatBottomBarHeight`/
  `combatBottomBarBg` revert-rename didn't regress combat's own layout);
  and, the real test, does the live Hero-presence line actually appear
  the instant the player is standing where a Hero is scheduled and
  disappear the instant they're not. Forcing a real Hero encounter needs a
  disposable save edited to a known `PRESENCE <location> <day-start>
  <day-end>` window from `data/timeline.txt` (same technique Milestone
  182's Look-command verification used): teleport to that location, set
  `hoursElapsed` inside the window, and confirm the status card shows
  "`<Name> is here.`" with no `T`/`L` needed first. Also worth checking a
  zone's own `TIMELINE_ANCHOR` POI (e.g. Thorbardin's Great Hall) the same
  way, and that the log-side push (still separate, feeding the full-log
  overlay and the location's flavor text) is unaffected.
- ~~**Dialogue portraits** (Milestone 208; redesigned bigger Milestone
  210; fixed-size + paginated Milestone 211)~~ — confirmed live
  2026-09-17 in its final (paginated) form: real conversations with both
  Otik and Tika showed the portrait at the same large, fixed size
  regardless of response length — Otik's long `TALK_AFTER` paginated
  underneath it instead of shrinking it ("press Enter to see more"
  through the middle pages, "continue" on the last), Tika's short
  greeting showed the same size portrait with just one page. Both fully
  uncropped (contain-fit). Picker-delegated states (Otik's own quest
  offer and topic list) still correctly show no portrait. Nothing left
  open for this milestone.
- ~~**Zone landmark plates** (Milestone 205), ordinary + portal-nested~~
  — confirmed live 2026-09-17: the Inn of the Last Home (a portal-nested
  zone) showed its plate full-window on arrival, cover-fit filling the
  frame with no letterboxing. Nothing left open for this milestone.
- ~~**Gold Box town menu auto-derivation + hotkey-collision fix**
  (Milestone 210)~~ — confirmed live 2026-09-17: the Inn auto-converted
  to a menu with zero `TOWN_MENU` flag, its `TIMELINE_ANCHOR` fireplace
  row present and gracefully showing "no one here"; Palanthas and High
  Clerist's Tower (both real `L`-collision fixes) each showed exactly one
  `[L]` row after their rename, and `Leave` correctly returned to the
  overworld from both (via direct `ZONE`/`ZONEPOS`/`ZONESTACK` edits to a
  disposable save, not a long overworld walk — a fresh, no-stack
  zone-entry in both cases). Nothing left open for the auto-derivation or
  the collision fix specifically.
- ~~**Gold Box town menu Inn round-trip + Leave label**
  (Milestones 206, 212)~~ — confirmed live 2026-09-17: walking back *out*
  of the Inn via its `[L]` row (now correctly labeled "Back to Solace,"
  Milestone 212) landed cleanly back on Solace's own menu, which still
  correctly showed its real banner art (exercising the `leaveCurrentZone`
  plate-reload-for-the-parent fix found during Milestone 206's original
  implementation) and its own accurate "[L] Leave town" label (its
  `zoneStack` is empty — a true top-level menu town). Nothing left open
  for the round-trip or the label fix specifically.
- **Gold Box town menu, pilot: Solace** (Milestones 206-207) — the
  *visual layout* is confirmed live (Milestone 207), and the `I` (Inn)
  destination is confirmed live (Milestone 209, reaching Otik/Tika) —
  opens correctly and stayed open through a full conversation. Still
  open: `B`/`G`/`S`/`K` (Solace's own Notice Board/General Store/Smithy/
  Bren Alder) individually confirmed to open the right screen and return
  to the menu afterward; a live walk from the overworld onto a menu-town's
  tile and pressing Enter to trigger `enterZone` that way specifically
  (every live confirmation so far has either started a save already
  inside the zone or portal-triggered from Solace's own menu); and the
  "Nothing here by that name." message for an unmatched letter.
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
