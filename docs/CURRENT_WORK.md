# Current work

**Full SFML engine migration -- decided and started, Phase 1 shipped and
confirmed.** On branch `sfml-trial-3` (off `master`; do not merge without
an explicit go-ahead). History: the round-3 real-map spike (real
`dragonlancemap2.png` as the overworld, `sfml_trial/main.cpp`) got a
"continue down this path" verdict, which led to two real production data
fixes shipped straight to `master` (Milestone 153/154, then a follow-on
Milestone 155 -- see `docs/MAP_NOTES.md` and `docs/MILESTONES.md`, not
repeated here), then real collision-checked player movement in the trial.
Full history is in git log / `docs/MILESTONES.md`, not repeated here --
this file only tracks what's still actionable.

**The user then committed to a full migration** (overworld, zones, and
combat all move to SFML -- not the "hybrid" alternative, which research
found would need an SFML window and the Windows console coexisting
mid-session, no precedent in this codebase) and to **real pixel-space
rendering per screen**, not a monospace-grid recreation (already tried
once in August, rejected as "looks almost exactly the same"). See
`docs/ARCHITECTURE.md`'s SFML section for the architecture this unlocked
and the one real wrinkle it surfaced (`Console`'s methods are `static` on
a concrete class, not an interface -- a true drop-in swap needs `Console.h`
restructured via PIMPL, deferred until a phase actually needs
`GameLoop::run()`'s full dispatch).

**Phase 1 shipped this session: a real, pixel-space overworld screen.**
New standalone target `ansalon_sfml_phase1` (`sfml_phase1/main.cpp`, not
reusing `GameLoop::run()` -- see the `Console` wrinkle above) that loads a
real save file read-only via `game::SaveGame`, walks the real map with
real collision (extending what the trial proved), and renders a real
pixel-space sidebar (character name/level/race/class, HP, in-game day/
time, a short event log) using SFML text against a placeholder Consolas
system font -- not a terminal recreation. Unimplemented keys (zone entry,
talk, shop, inventory, journal, rest, etc.) log a plain "not yet in this
build" line instead of doing anything. Verified: clean rebuild (zero new
`/W4` warnings) and the user testing it live at the keyboard against a
real save. **User's verdict: "looks good."**

Phase 1 committed as `778a57c`.

**Phase 2 shipped this session too: zone interiors, in the same
`ansalon_sfml_phase1`/`sfml_phase1/main.cpp`** (extended in place, not a
new target -- this is the one growing prototype the migration
accumulates into). Added `world::Zone`/`ZoneTile`/`ZoneLoader`/
`ZoneCatalog` to the target; `state.mode` now genuinely switches between
Overworld and Zone rendering/movement, matching `game::GameState` exactly.
Real collision (`zoneTileFor`, POIs always passable), real entry/exit via
`Enter` (including `PORTAL`-nested sub-zones like Solace's Inn, pushing/
popping `GameState::zoneStack`), placeholder pixel-space tile colors (no
reference art exists for interiors, unlike the overworld's real map) with
a small name label at each POI marker. Talk/Shop/etc. stay the same
"not yet in this build" placeholders Phase 1 already had, even inside a
zone. Verified: clean rebuild (zero new `/W4` warnings), user tested live
-- walked into Solace, into its Inn via the portal, and back out through
both exits. **Confirmed working.**

Phase 2 committed as `4ff3bfb`.

**Zone-interior placeholder visuals reworked this session, same
`sfml_phase1/main.cpp`.** Phase 2's POI tiles fell through
`colorForZoneTile`'s unhandled-code branch (every POI sits on its own
letter code in the grid, not `.`) straight to a flat magenta square --
user's word for the result: "yellow and pink squares." Fixed two ways:
the fallback now renders as plain open-ground floor instead of magenta
(confirmed via `ZoneLoader.cpp`'s fail-fast grid validation that this
branch is otherwise unreachable for any zone that loaded at all), and
each POI now draws a small 2-primitive placeholder icon instead of a
flat colored dot -- shape *and* color now carry meaning: a doorway for
portals, a counter+awning for shops, a mattress+pillow for beds, a
head+robe figure for talkable NPCs, a small muted diamond for pure
scenery (so non-interactive landmarks recede behind things the player
can actually do something with). Classified purely from data already on
`world::PointOfInterest`/`world::Zone` (`isShop`/`isBed`/`dialogue`-
non-empty/`portalAt`) -- no data-file or `world::` changes needed.
Verified: clean rebuild (zero new `/W4` warnings); user tested live in
Solace and the Inn (all 5 icon kinds). **User's verdict: "much better, a
decent starting spot."**

Icon rework committed as `ef1ac77`.

**Phase 3 (combat) shipped this session, same `sfml_phase1/main.cpp`.**
User picked combat off the roadmap below. `GameLoop::runCombat` (the console
reference, ~1300 lines) is one big blocking `for(;;) { draw(); readKey(); }`
loop with nested blocking pickers; this build replaces that with a
non-blocking `CombatSession`/`CombatUiState` state machine (see main.cpp's
own doc comments) driven by the same per-KeyPressed-event loop Phase 1/2
already use, reusing every sourced combat primitive
(`combat::resolvePlayerAttack`/`resolveMonsterAttack`/`playerActsFirst`/
`rollSavingThrow`, `combat::isAdjacent`/`chebyshevDistance`/`stepToward`,
`combat::rollGroupSize`/`MonsterCatalog::randomMonster`,
`character::meleeAttacksThisRound`/`applyPendingLevelUps`) completely
unchanged.

**Scoped to the core melee loop, real spellcasting/item use deferred** (see
the approved plan and main.cpp's top-of-file comment for the full
reasoning): real random encounters (data/monsters.txt now loaded by this
target too), positional movement, target picking (one target per round --
this phase's own simplification of the console version's mid-round
retarget-on-kill, since that needs real resumable state to redo non-
blockingly), monster AI including each monster's *passive* on-turn/on-death
specials (Bozak Magic Missile, Aurak breath weapon, Sivak death-burst, Baaz
stone-death, Giant Spider poison -- all automatic, no chooser, so they cost
nothing extra), companion AI (present and fighting, no sweep), flee,
victory/XP/leveling/knockout-to-nearest-refuge, and the two free wins
(Frostreaver glacier bonus, Weapon Specialization). `M` (Cast) and `I` (Item)
print "not yet implemented in this build" during combat, matching Phase 1's
existing placeholder convention -- thief backstab and Fighter sweep are
deferred the same way (no key currently maps to them either).

Verified: clean rebuild of `ansalon_sfml_phase1` (zero new `/W4` warnings,
`CMakeLists.txt` gained `src/combat/Combat.cpp`/`Monster.cpp`/
`MonsterLoader.cpp`/`CombatGrid.cpp`), a launch smoke test (loads save2, all
catalogs including the new monster one load, window opens, no crash) --
this session had no desktop/GUI access to go further itself -- and then the
user testing it live at the keyboard: **won a fight and lost one (knockout),
both confirmed working.** This build still never writes back to the save
file, so nothing about the user's real save was ever at risk.

Phase 3 committed as `a3e30fc`.

**Not yet interactively confirmed** (moved to the Playtest backlog below,
same convention as every other partially-verified feature in this project):
Flee, a multi-instance group encounter's in-frame target picker, and a
recruited companion fighting alongside the player.

**Character sheet shipped this session, same `sfml_phase1/main.cpp`.** User
picked it off the full-migration roadmap below. A read-only, full-window
overlay bound to `C` (works from Overworld or Zone, dismissed by any key,
matching every console overlay's blocking-`readKey()` convention), toggled
via a new local `sheetOpen` bool -- same "transient UI state, not a third
`game::Mode` value" reasoning `CombatSession` itself already established.
Composited as a final draw-on-top layer rather than branching the existing
map/sidebar draw chains, so those needed no changes at all.

Content mirrors `render::MapRenderer::drawCharacterSheet`
(`MapRenderer.cpp:689-840`) field-for-field -- race/subrace, class, level,
XP, alignment, knight order, robe affiliation, all 6 ability scores
(exceptional-strength percentile included), HP/AC/THAC0, weapon/armor, all
5 saving throws, steel, carried items, a spells-memorized summary (casters
only), and recruited companions -- laid out as two real pixel-space columns
across the full window instead of one long list squeezed into the 320px
sidebar, plus a small HP bar (a genuine visual the console version
structurally can't do). **Deliberately dropped**: the console sheet's `'s'`
full-spellbook drill-down (`GameLoop::showSpellbook()`) -- spellbook is its
own separate roadmap item below, out of scope here; the memorized-spells
summary line alone still shows what's prepared.

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke test
against real `save2.txt` (Regan, level 20 Human Mage -- a caster, so the
robe/spells-summary paths get exercised) confirming no crash/exception on
startup -- this session again had no desktop/GUI access to press `C` and
see the panel itself. **Not yet interactively confirmed** -- moved to the
Playtest backlog below.

**Dialogue (core conversation) shipped this session, same
`sfml_phase1/main.cpp`.** User picked it off the full-migration roadmap
below. This is the biggest screen ported so far by underlying logic size --
`GameLoop::talkTo`/`offerOrTurnInQuest` (`GameLoop.cpp:971-1470+`) is the
single largest screen in the console build, well past combat's core melee
loop. Research surfaced that `ansalon_sfml_phase1` didn't link
`timeline::Timeline` at all before this session -- without it, Talk could
never find a canon Hero, and a zone POI's own `TALK_AFTER`/`TALK_BEFORE`
gating (which itself queries `Timeline::latestDayEnd`/`earliestDayStart`)
couldn't work either. Added `src/timeline/Timeline.cpp`/`TimelineLoader.cpp`
to the target and load `data/timeline.txt` alongside the existing
World/ZoneCatalog/MonsterCatalog loads.

**Scoped, by user decision, to the core conversation loop**: TALK/
TALK_AGAIN, TALK_AFTER (aftermath)/TALK_BEFORE (anticipation), conditional
greeting (SAY_IF), the curated TOPIC picker, and GRANTS_ITEM (needs no
chooser -- an inventory push + one log line, so it shipped for free, same
"cheap, no chooser" reasoning Phase 3 used for monsters' passive specials).
Free-text "Ask about something else..." (needs real `TextEntered` input
handling, unused anywhere in this codebase so far), quest offer/accept/
progress/turn-in, boat-voyage accept/decline, and companion-recruit accept/
decline are each deferred to a later phase -- a POI carrying any of those
now prints one placeholder log line instead of silently doing nothing.
**Scope correction found during implementation:** `ASK_LIMIT_LOCKED` was
initially discussed as in-scope, but it can only ever fire once free-text
asking has exhausted a daily question cap -- with that flow deferred, the
counter never advances and it can never trigger, so it was dropped from
this phase's local `Speech` shape entirely rather than carried as dead
code. It'll come back naturally once free-text ask ships. Also not
touched: canon-character "X is here" arrival announcements aren't ported in
this build at all yet (only POI arrivals log a line today) -- a player has
to think to press Talk, with no auto-nudge that a Hero is standing on their
tile; a reasonable follow-up, out of scope for "the Talk screen" itself.

`GameLoop.cpp`'s own `talkTo` machinery isn't linkable here (it's built on
`render::Console`/`render::MapRenderer`, which this target deliberately
excludes), so its logic was ported as new local code following this file's
existing precedent (`pluralMonsterName`/`describeToHit`/`describeDamage`
are already copied verbatim from `GameLoop.cpp` for the same reason):
trimmed local `DialogueSpeech`/`DialogueCandidate` structs, `conditionMatches`
copied verbatim, local `speechFromWindow`/`speechFromPoi`, a
`gatherTalkCandidates()` mirroring `GameLoop::handleTalk`, and a
`DialogueSession`/`DialogueUiState` state machine (`PickingCandidate`/
`Greeting`/`TopicText`/`TopicPicker`) following `CombatSession`/
`CombatUiState`'s own naming and non-blocking shape. One implementation-time
correction from the plan: Q/Escape already close the whole window
unconditionally in this build (checked before any per-screen dispatch), so
there was never room for them to also mean "cancel this picker" -- matching
combat's own `PickingTarget` precedent, `PickingCandidate` has no cancel
(must pick someone) and `TopicPicker`'s always-present "Nothing, thanks"
entry is its only way out. Rendering is a new full-window overlay
(`drawDialogueOverlay`), composited the same way `drawCharacterSheetOverlay`
already is, reusing that overlay's own color/size constants and the
existing `wrapToWidth` helper.

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke test
against real `save2.txt` confirming every catalog -- including the new
`timeline::Timeline` -- loads and the window opens with no crash/exception
-- this session again had no desktop/GUI access to press `T` and actually
talk to anyone. **Not yet interactively confirmed** -- moved to the
Playtest backlog below.

**Free-text "Ask about something else..." shipped this session, same
`sfml_phase1/main.cpp`.** User picked it off the full-migration roadmap --
the smallest genuinely new engine capability still open (every other
roadmap screen is a pure UI-state port; this one needed this codebase's
first real `sf::Event::TextEntered` handling) and it directly unblocks
something the dialogue phase left waiting on it: `DialogueSpeech`'s own
comment said `ASK_LIMIT_LOCKED` "can only ever fire once that free-text
flow has exhausted a daily question cap" -- restored along with the input
box itself, not just the typing UI.

`DialogueSpeech`/`speechFromWindow`/`speechFromPoi` regained full parity
with `game::Speech`/`game::speechFromWindow`/`game::speechFromPoi`
(`subjects`/`subjectUnknown`/`askLimit*`/`suppressAskHints`/
`askLimitLocked*`, all previously dropped as unreachable dead code -- see
those functions' own comments in `sfml_phase1/main.cpp`). `tokenizeAskInput`/
`matchSubject` are copied verbatim from `GameLoop.cpp`, same idiom as
`conditionMatches`/`pluralMonsterName` already established. Two new
`DialogueUiState` values (`AskInput` for typing, `AskResponse` for the
result -- 1 or 2 queued messages, covering the reached/extended-cap
follow-up) drive a non-blocking port of `GameLoop::talkTo`'s ask-anything
block (`GameLoop.cpp:1160-1298`) and its `askLimitLocked` greeting override
(`GameLoop.cpp:1010-1023`), including the homebrew Intelligence+Wisdom
hard-cap-extension check, unchanged.

**One real behavior fix discovered during implementation, beyond the
approved plan's scope:** this build's established rule is "Escape/Q always
closes the whole window, no per-screen cancel" (true for every other picker
here) -- but during free-text typing, 'q' is an ordinary letter a player may
need to type, not a quit key, so closing the app on it would have been a
real bug, not just a UX inconsistency. Fixed by deferring Q/Escape's
`window.close()` out of the raw key switch into a guarded check: while the
ask-input box is open, Q is swallowed (harmless -- the actual character
still reaches the buffer via `TextEntered`) and Escape cancels the box
(returns to the topic picker) instead of closing the window. Every other
screen's Escape/Q behavior is unchanged. Empty-buffer Enter also cancels
(mirrors `Console::readLine`'s own Esc-returns-empty behavior), so there
are now two equivalent ways out, matching the console exactly either way.

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke test
against real `save2.txt` confirming every catalog still loads and the
window opens with no crash/exception -- this session again had no desktop/
GUI access to actually type a question. **Not yet interactively
confirmed** -- moved to the Playtest backlog below.

**Generic picker overlay shipped this session, same `sfml_phase1/main.cpp`.**
User picked it off the full-migration roadmap. The console build's
`render::MapRenderer::drawPickerFrame` (`MapRenderer.cpp:1149`) is its
single most-reused screen primitive -- a "title + cursor list + footer"
frame shared by Talk-to-whom, topic menus, Look-at-whom, quest offer/
accept, boat departure, companion recruit, and spell-memorization
keep-loadout (see `GameLoop.cpp`'s own callers). The SFML port had no
equivalent shared component: dialogue's `PickingCandidate`/`TopicPicker`
states each hand-rolled their own near-identical title/cursor-list/
footer rendering block inside `drawDialogueOverlay`, real duplication
between just those two cases alone.

Added a new `drawPickerOverlay(title, items, selectedIndex, footer)`
lambda alongside the other overlay draws (`main.cpp`, right before
`drawDialogueOverlay`), mirroring `drawPickerFrame`'s signature and
reusing the character sheet's own `kSheet*` color/size constants for
visual consistency. `PickingCandidate`/`TopicPicker` now both delegate
to it instead of rendering their own list -- pure extraction, no new
visual or input behavior. **Deliberately not used by combat's
`PickingTarget`**: that picker's cursor is drawn embedded in the roster
panel, a structurally different shape from this full-window overlay: left
as-is, with a comment explaining why. Shop, Inventory, and Spellbook (the
roadmap's remaining picker-shaped screens) weren't built this session --
they can now call `drawPickerOverlay` directly instead of each
duplicating the loop a third, fourth, and fifth time.

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke test
against real `save2.txt` (run from the repo root -- `References/` isn't
copied next to the exe the way `data/` is, so the map texture only loads
when the CWD is the repo root, same as this project's documented
`ansalon_rpg.exe` invocation convention) confirming every catalog and the
map texture still load and the window opens with no crash/exception --
this session again had no desktop/GUI access to see the picker render.
Since the render path for dialogue's two pickers changed even though
behavior shouldn't have, that pair is flagged for a re-check alongside
dialogue's own still-open playtest item below, not treated as newly at
risk.

**Shop shipped this session, same `sfml_phase1/main.cpp`.** User picked it
off the full-migration roadmap -- confirmed as the cheapest of the three
picker-overlay-shaped screens (`drawPickerOverlay` was extracted from
dialogue specifically to unlock this), since all of the buy/sell mechanics
already live in `src/character/Equipment.h`/`.cpp`, which was *already*
compiled into this CMake target for the character sheet's steel/weapon/
armor lines. **Zero `CMakeLists.txt` changes needed** -- a real
simplification versus every prior phase, each of which added new source
files.

Ports `GameLoop::handleShop` (`GameLoop.cpp:1570-1636`) faithfully: browse
a shop's catalog (buy), toggle to the sell view (`I`), purchase/sell with
the same insufficient-steel/already-owned/cannot-use/cannot-sell checks
and feedback messages, all computed fresh each call via
`character::availableShopItems`/`sellableItems`/`purchaseItem`/`sellItem`
(unchanged) -- no caching, matching those functions' own "compute on
demand" comment. `drawPickerOverlay` gained one optional trailing
`message` parameter (default `""`, drawn between the item list and
footer) so Shop's post-transaction feedback could reuse it too; dialogue's
two existing call sites are unaffected. New local `shopCatalogFor` and
`ShopSession` (a flat struct, no `UiState` enum -- unlike Dialogue/Combat,
Shop has exactly one screen shape throughout, just a `sellMode` toggle,
matching `handleShop`'s own single loop).

**One real gap, flagged rather than faked:** `SHOP_LOCKED`
(`docs/ZONE_NOTES.md`) gates a shop behind a quest being Complete --
exactly one POI project-wide, Flint's Smithy in Solace
(`data/zones/solace.txt:41`). This build tracks no quest state at all yet
(same gap dialogue's own deferred quest/boat/recruit offers already
flag), so that one shop prints "(This shop is quest-locked -- that isn't
tracked in this build yet.)" instead of silently always-locking (reads as
"no shop here") or silently unlocking (lets the player buy before
earning it) it.

**One deliberate deviation from this build's own established convention,
called out explicitly:** every other overlay here (character sheet,
combat, dialogue) follows "Q/Escape always closes the whole window, no
per-screen cancel." But `GameLoop::handleShop` explicitly documents
`Key::Quit here exits the shop, not the whole game` (`GameLoop.cpp:1611`),
and backing in and out of a shop repeatedly is the normal case, not an
edge case -- closing the whole app on `Q` would be an immediate
regression a player would hit on their very first visit, not a style
nit. So Shop gets a local Quit override (same shape as ask-input's
existing Escape override), checked before the general "Q closes the
window" branch.

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke test
against real `save2.txt` confirming every catalog still loads and the
window opens with no crash/exception -- this session again had no
desktop/GUI access to press `P` live. **Not yet interactively
confirmed** -- moved to the Playtest backlog below.

**Inventory shipped this session, same `sfml_phase1/main.cpp`.** User
picked it off the full-migration roadmap -- confirmed as the cheapest
remaining screen, same "already have every mechanic, already have the
picker overlay" reasoning that made Shop cheap. **Zero
`CMakeLists.txt` changes needed** -- `src/character/Equipment.cpp` was
already linked in for the character sheet/Shop phases.

Ports `GameLoop::handleInventory` (`GameLoop.cpp:1638-1675`) faithfully:
North/South cycles the carried-item list (wrapping, no-op if empty);
Enter dispatches on `InventoryItem::kind` -- a Potion is drunk via
`character::drinkPotion` (its result message shown), a Webnet/Brooch of
Imog shows "That can only be used in combat.", a QuestItem shows "That's
meant for someone else -- you'll need to deliver it.", and everything
else (Armor/Shield/Weapon) is equipped via `character::
equipInventoryItem` -- after any action the cursor resets to 0, matching
the console's own "the list just changed shape" comment. Unlike Shop,
Inventory has no opening gate (works from Overworld or Zone, matching
Character Sheet's `sheetOpen` convention rather than Shop's POI-gated
one), and `Q` closes just the screen, not the whole window (same
deliberate deviation Shop's own `Key::Quit` precedent already
established, since `handleInventory`'s own Quit just returns from its
local loop).

New local `InventorySession` (flat struct, no `UiState` enum -- one
screen shape throughout, same reasoning as `ShopSession`). Reuses
`drawPickerOverlay` for the item list; since that lambda only takes a
flat row list with no separate header block, the HP/Weapon/Armor+Shield
summary lines (mirroring `drawInventoryFrame`'s own header,
`MapRenderer.cpp:1105-1147`) are prepended as non-selectable leading
rows and the cursor index is offset past them, with the cursor
suppressed entirely (index -1) when the inventory is empty so
"(nothing carried)" never draws a selection arrow. `I` now opens
Inventory in the ordinary case; while a shop is open it still means
"toggle buy/sell view" as before (Shop's own dispatch reads `key`
directly and is checked first, so the two never conflict).

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke
test against real `save2.txt` (run from the repo root) confirming every
catalog still loads and the window opens with no crash/exception --
this session again had no desktop/GUI access to press `I` live. **Not
yet interactively confirmed** -- moved to the Playtest backlog below.

**Spellbook shipped this session, same `sfml_phase1/main.cpp`.** User
picked it off the full-migration roadmap -- the cheapest of the 5
remaining screens, and the one already named and explicitly deferred when
the character sheet shipped (`main.cpp`'s own comment at the time: "minus
its 's' = full-spellbook drill-down"). **Zero `CMakeLists.txt` changes
needed** -- `src/character/Spellcasting.cpp` was already linked in for
the character sheet's own spells-memorized summary line.

Ports `GameLoop::showCharacterSheet`/`showSpellbook`
(`GameLoop.cpp:626-647`) and `render::MapRenderer::drawSpellbookFrame`
(`MapRenderer.cpp:842-885`) faithfully: while the sheet (`C`) is open,
`s`/Down -- offered only when `character::canCastSpells` -- opens a new
`spellbookOpen` overlay listing every known spell grouped by level
("Level N (X slots/day):" headers) with "(memorized)"/"(memorized xN)"
annotations when `spellsCastDay` matches today, or "Cannot cast arcane
magic." for the level-0 case; any key returns to the sheet, which stays
open (matching the console's own showCharacterSheet loop, not a full
dismiss). Reuses `drawPickerOverlay` with `selectedIndex` always `-1`
(pure information, no selection at all -- the same idiom
`drawInventoryOverlay`'s own non-selectable header rows already
established) rather than adding a new overlay-drawing shape. The sheet's
own footer now also matches the console's conditional wording
(`"(s=view spells known, any other key to continue)"` for casters,
`"(press any key to continue)"` otherwise), which the SFML port had
skipped entirely up to now.

**One real bug found and fixed while touching this exact code, beyond
the approved plan's narrow scope:** the key-dispatch chain's general
`wantsQuit && !askInputActive -> window.close()` branch was checked
*before* `sheetOpen`'s own dismiss branch, meaning Q/Escape while the
character sheet was open closed the whole window instead of just
dismissing the sheet -- contradicted the sheet's own "dismissed by any
key" doc comment and the console's behavior (its `showCharacterSheet`
loop has no special Quit handling, so Quit falls through to "any other
key -> dismiss" same as everything else). Fixed by moving the
sheet/spellbook dismiss guards ahead of the general quit branch, the
same placement Shop's and Inventory's own Quit overrides already use
just above it -- not a new pattern, just applying the existing one to a
third screen. This means the character sheet's own Q/Escape behavior is
also now correct, not just spellbook's.

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke
test against real `save2.txt` (Regan, level 20 Human Mage -- a caster,
so the new path is exercised) confirming every catalog still loads and
the window opens with no crash/exception -- this session again had no
desktop/GUI access to press `C` then `s` live. **Not yet interactively
confirmed** -- moved to the Playtest backlog below.

**Help, Full Log, World Map, and Journal all shipped this session, same
`sfml_phase1/main.cpp` -- the last 4 screens on the full-migration
roadmap, closing it out entirely.** User asked to knock out all 4 in one
pass rather than pick one at a time. All four follow the established
`bool xOpen` / "any key dismisses" overlay convention (`sheetOpen`,
`spellbookOpen`) or the flat-struct-session convention (`ShopSession`,
`InventorySession`), guarded ahead of the general `wantsQuit ->
window.close()` branch in the key-dispatch chain, composited as a final
draw-on-top layer -- no new pattern introduced. **Zero `CMakeLists.txt`
changes** across all four.

- **Help** (`/`, this build's bind for the console's `?` -- the physical
  key `sf::Keyboard::Key::Slash` maps to) -- `bool helpOpen`, ported
  verbatim from `render::MapRenderer::drawHelpFrame`
  (`MapRenderer.cpp:1474-1500`): static command reference text, no
  game-state dependency, rendered via the existing `drawPickerOverlay`
  with `selectedIndex -1` (same "pure information" idiom Spellbook
  already established). One wording change from the console text: "/ =
  this help screen" instead of "? = this help screen", matching the real
  key this build binds.
- **Full Log** (`V`, replacing its old placeholder) -- new
  `LogSession { active, scrollOffset }` (the `-1` `scrollOffset` sentinel
  matches `drawLogFrame`'s own "start at the bottom" convention,
  `MapRenderer.cpp:1220-1256`). North/South scroll by a fixed 10-line
  chunk (matches `GameLoop::handleLog`'s own `kLogScrollStep`,
  `GameLoop.cpp:1681`); `V` or `Q` closes it (same "the key that opened
  it closes it too" rule `handleLog`'s own comment states,
  `GameLoop.cpp:1687-1690`). `drawLogOverlay` wraps and scroll-windows
  the same `log` vector the sidebar already renders in full every frame
  unclipped, then delegates the actual title/list/status/footer
  rendering to `drawPickerOverlay` -- a "Lines X-Y of Z" status line
  matches the console's own wording.
- **World Map** (`O`, previously unbound) -- new `bool worldMapOpen`.
  Real design choice: rather than porting `drawWorldMapFrame`'s ~180-line
  ASCII box-majority-vote downsampling (`MapRenderer.cpp:1258-1435`),
  this screen draws the *real* `dragonlancemap2.png` scaled down to fit,
  since `mapTexture` and the real grid-to-pixel scale
  (`pxPerTileX`/`pxPerTileY`) the live overworld already uses were
  already loaded -- genuinely higher fidelity than the console version
  and far less code. Location markers (same green-town/red-other color
  convention the live overworld uses) and the player's own marker are
  plotted via that same scale; a side legend lists all 25 locations
  alphabetically (colored bullet + name -- fits one column at this
  window size, so the console's own "+N more" truncation wasn't needed).
- **Journal** (`G`, replacing its old placeholder) -- new `bool
  journalOpen`. Real judgment call, not a silent stub: `GameLoop::
  showJournal` iterates `game::GameState::quests`, which exists on
  `GameState` (`GameState.h:98`) but this build never populates it --
  quest offer/accept dialogue is still deferred, the same gap Shop's
  quest-lock and Dialogue's `hasQuest` placeholder already flag (grepped
  to confirm: no `state.quests` write site anywhere in this file). A
  byte-faithful port would always render the console's own empty-state
  line, "(no quests yet)" -- but that reads as "you truly have zero
  quests," not "this build doesn't track quests yet." To stay consistent
  with Shop's and Dialogue's own honest-placeholder wording, Journal's
  body says explicitly that quest tracking isn't wired up in this build
  yet, rather than silently implying a working-but-empty quest log. Needs
  no `quest::` sources linked into the CMake target.

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke
test against real `save2.txt` confirming every catalog still loads and
the window opens with no crash/exception -- this session again had no
desktop/GUI access to press `/`/`V`/`O`/`G` live. **Not yet interactively
confirmed** -- moved to the Playtest backlog below.

**This closes the full-migration roadmap out.** Every screen in
`ansalon_sfml_phase1` is now a real pixel-space port; nothing on that list
is ASCII/terminal-only anymore, aside from character creation and the
save-slot menu, which were always explicitly out of scope (plain
`std::cin`/`std::cout`, no window exists yet when they run). The open
question the roadmap's own last line named -- "when (if ever) this code
gets promoted to replace `ansalon_rpg` outright" -- is now the natural
next thing to decide, once enough of the Playtest backlog below is
cleared to trust it.

**Next step:** the user's call, not to be assumed -- clear the Playtest
backlog below (a lot has shipped without a live keyboard check this
session and prior ones), decide when/whether to promote this build to
replace `ansalon_rpg`, or something else.

**Standalone demo packaging added this session:** `tools/package_sfml_demo.ps1`
(sibling to `tools/package_release.ps1`, which packages the older console
target instead). Builds Release `ansalon_sfml_phase1` and stages a folder
under `dist/` with the exe, `data/`, `References/dragonlancemap2.png`, a
copy of `save2.txt` as `demo_save.txt` (Regan, level 20 Human Mage --
read-only, never written back), a `RunDemo.bat` launcher, and a
`README.txt` (WIP disclaimer, controls, map attribution) -- lets someone
outside this source tree run the SFML build without a terminal. Verified
via a launch smoke test of the staged folder in isolation (all catalogs +
save + map texture loaded, window opened, stayed running); no interactive
playtest of the packaged copy itself. Zips to
`dist/AnsalonSFMLDemo-v<N>.zip` (~76MB, dominated by the map PNG);
`dist/` stays gitignored as before.

**Loading screen and quit confirmation added this session, same
`sfml_phase1/main.cpp` -- SFML-only by user decision** (the console
build, `ansalon_rpg`, already has an established `promptYesNo("...(y/n)
")` convention for confirmations and a different, already-instant text
loading trace; touching it wasn't asked for and would have roughly
doubled this change for no requested benefit).

`runPhase1()` used to create the window *after* the grid/world/zones/
monsters/timeline/save loads, then decode `References/
dragonlancemap2.png` (the heaviest single load) and open the placeholder
font before the first `window.display()` -- a real risk of a blank or
"Not Responding" window on a slow disk. Window and font construction
moved to the top of the function, and a small local `drawLoadingScreen`
lambda now draws a real "Ansalon: Age of Despair" / status-line frame
(and pumps/handles the window-close event) before each of the existing
load steps -- every original `std::cout << "step N: ..."` trace line is
unchanged, this just interleaves a real drawn frame ahead of each one.

Separately, `Q`/Escape used to call `window.close()` unconditionally
from the outermost catch-all in the key-dispatch chain -- every other
overlay (dialogue/shop/inventory/spellbook/sheet/help/world map/journal/
log) already had its own `wantsQuit` override that closes just that
screen, but Combat did not, so `Q` mid-fight fell into the same
catch-all and force-quit with zero warning. That catch-all now opens a
new `quitConfirmOpen` overlay ("Are you sure you want to end your
adventure?", reusing the existing generic `drawPickerOverlay` the same
way Help/Journal/Shop/Inventory/Spellbook already do) instead of closing
outright -- up/down selects between "Yes, end my adventure" / "No, keep
playing" (defaults to "No"), Enter acts on the selection, Escape/Q
cancels the dialog itself rather than falling through to anything else.
This also fixes the Combat gap above for free, since it shares the same
catch-all. Deliberately unchanged: clicking the window's own OS close
button (`sf::Event::Closed`, the title-bar X / Alt+F4) still closes
immediately -- the user's ask was about an accidental `Q` keypress
specifically, and gating a deliberate OS-level gesture too would be
scope creep beyond that.

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke
test against real `save2.txt` (run from the repo root) confirming every
`step 0..4` trace line still prints in order and the window opens with
no crash/exception -- this session again had no desktop/GUI access to
actually watch the loading screen draw or press `Q` and see the confirm
dialog. **Not yet interactively confirmed** -- moved to the Playtest
backlog below.

**Combat spellcasting (`M`) shipped this session, same
`sfml_phase1/main.cpp`.** User picked it off Phase 3's own deferred list
(item use, thief backstab, and Fighter sweep all stay deferred,
untouched). Direct port of `GameLoop::playerCasts`'s full switch
(`GameLoop.cpp:2616-2722`) -- all 13 `SpellEffect` cases, so every
sourced spell in `character::spellListFor` actually works in this build
now, not a hand-picked subset -- as a new `combatApplySpellEffect`, plus
the rest of `GameLoop::runCombat`'s this-fight-only buff/debuff locals
(`GameLoop.cpp:1955-1978`) added to `CombatSession` alongside the two
(`playerThac0Bonus`/`playerDamageBonus`) Phase 3 already carried for the
Frostreaver/Weapon Specialization fight-start bonuses:
`playerAcBonus`, `hasteAttackMultiplier`, and four new per-instance
vectors (`monsterThac0Penalty`/`monsterDamagePenalty`/`monsterAcPenalty`/
`blockedAttacksRemaining`/`incapacitatedRestOfFight`), sized fresh in
`combatStartEncounter` same as the console's own per-fight vectors. All
threaded into the existing `resolvePlayerAttack`/`resolveMonsterAttack`
call sites (player attack, companion attack, monster attack, opportunity
attacks), and `combatMonstersAct` gained the
Web/Hold-style incapacitated/blocked skip-guard (with the same
attack-suppressed log lines) the console's `monstersAct` always had.
`globeActive`/Brooch of Imog stays out -- that's Item-use scope, not
touched.

**The real design problem, solved by sequencing carefully rather than
inventing new mechanics:** traced exactly in `GameLoop.cpp`, the
console's own "which spell?" chooser resolves entirely *before*
initiative is rolled (free to cancel), but `character::castSpell` itself
and "which target?" both live inside `playerCasts`, which only runs as
part of the shared `playerActsFirst()` dispatch every action goes
through -- so if the monsters act first and that knocks the player out,
the chosen spell is never actually cast or consumed. Reproduced in this
non-blocking build with a new `PickingSpell` `CombatUiState` (2+
distinct memorized spells -- a full list via the existing
`drawPickerOverlay`, unlike `PickingTarget`'s roster-embedded grid
cursor; exactly one distinct spell skips straight past it, same
"no picker needed for one candidate" rule every other combat chooser
here already follows) that resolves with **no initiative roll and no
round consumed**, Escape/Q cancelling it for free (a new guard ahead of
the general quit-confirm catch-all, same placement every other
screen's own Quit override uses). Only once a spell is truly chosen does
`combatCommitSpellChoice` roll initiative (`combatRollGoFirstAndMaybeActMonsters`,
already established by `combatBeginPlayerAttack`) and *then* call
`character::castSpell` -- so a monsters-act-first knockout correctly
prevents the cast, matching the console exactly. `PickingTarget` itself
is reused for a spell's target choice (originally a `pickingForSpell`
bool + `pendingSpellResult` on `CombatSession` disambiguating it from an
ordinary melee attack, since generalized to a 3-way `TargetPickReason`
enum -- see the item-use writeup directly below) rather than adding a
second grid-cursor state -- same single mechanism `GameLoop::pickTarget`
already is for all of attack/spell/item targeting. Unlike melee, spell
targeting isn't adjacency-restricted (every alive instance is eligible,
matching `GameLoop.cpp:1935`'s own `anyAlive` filter).

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke
test against real `save2.txt` (Regan, level 20 Human Mage -- a caster
with Fireball per Milestone 152's own playtest note) confirming every
catalog still loads and the window opens with no crash/exception -- this
session again had no desktop/GUI access to press `M` live. **Not yet
interactively confirmed** -- moved to the Playtest backlog below.

**Combat item use (`I`) shipped this session, same `sfml_phase1/main.cpp`.**
User picked it off Phase 3's own deferred list (thief backstab and Fighter
sweep stay deferred, untouched). Direct port of `GameLoop::runCombat`'s own
four item-use lambdas (`GameLoop.cpp:2724-2759`: `playerDrinksPotion`/
`playerUsesWebnet`/`playerActivatesBrooch`/`playerUsesStaffCure`) and its
`I`-key USE-menu chooser (`GameLoop.cpp:2931-2985`), reusing
`character::availableCombatItems`/`CombatItem`/`CombatItemKind` and the
four underlying `character::` functions unchanged -- all already linked
into this target via `src/character/Equipment.cpp` for Shop/Inventory/
Character Sheet. **Zero `CMakeLists.txt` changes needed.**

Same "pick before initiative, resolve after" shape spellcasting already
established: a new `combatBeginUseItem` (mirrors `combatBeginCast`) builds
the usable-item list fresh, auto-resolves the 0/1-item cases for free (no
round consumed), and opens a new `PickingItem` `CombatUiState` for 2+ (a
full list via `drawPickerOverlay`, new `drawCombatItemPickerOverlay`,
composited the same way `drawCombatSpellPickerOverlay` is). Only once an
item is chosen does a new `combatCommitItemChoice` roll initiative
(`combatRollGoFirstAndMaybeActMonsters`) and *then* actually apply it --
so, same as a monsters-act-first knockout preventing a chosen spell from
being cast, it also now prevents a chosen item from being consumed.
Escape/Q cancels `PickingItem` for free, extending the existing
`PickingSpell` guard rather than duplicating it.

Webnet reuses `PickingTarget` a third way (not adjacency-restricted, same
as spell targeting) -- `CombatSession::pickingForSpell` (a plain bool) was
generalized to `TargetPickReason { Attack, Spell, Webnet }` to disambiguate
`combatConfirmTarget`'s three real resolutions and the footer's three real
prompts ("Attack"/"Cast X"/"Tangle which enemy?"), rather than adding a
second ad-hoc bool alongside it.

**One real gap closed in the same pass, beyond a pure item-use port:** the
Brooch of Imog's `globeActive` flag was explicitly called out as
"Item-use scope, not touched" when spellcasting shipped -- it didn't exist
anywhere in `combatMonstersAct`/`combatTriggerOpportunityAttacks` yet, so
porting item use without it would have made the Brooch a real no-op in
this build. Added `CombatSession::globeActive` (resets for free each fight
via the existing `combatSession = CombatSession{}` reset) and wired it into
all four spots `GameLoop.cpp` gates on it: Bozak Magic Missile
(`GameLoop.cpp:2463`), Aurak breath weapon (`GameLoop.cpp:2482`), a melee
hit resolving against the player specifically -- never a companion, the
Brooch is player-only (`GameLoop.cpp:2549-2556`) -- and opportunity attacks
(`GameLoop.cpp:2182`).

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke test
against real `save2.txt` confirming every catalog still loads and the
window opens with no crash/exception -- this session again had no
desktop/GUI access to press `I` live. **Not yet interactively confirmed**
-- moved to the Playtest backlog below.

**Thief backstab and Fighter sweep shipped this session, same
`sfml_phase1/main.cpp`.** User picked them off Phase 3's own deferred
list -- the last two items on it (spellcasting and item use had already
closed out the rest). Direct port of `GameLoop::runCombat`'s shared
`firstAttackerId`/`positionOfAttacker`/`backstabBonus`/
`adjacentWeakInstances` lambdas (`GameLoop.cpp:2024-2085`), its player
sweep block (`GameLoop.cpp:2214-2245`), backstab wired into the player's
ordinary attack loop (`GameLoop.cpp:2275-2289`), and companion sweep +
backstab (`GameLoop.cpp:2347-2416`) -- reusing `character::classGroupFor`/
`ClassGroup::Warrior`/`ClassGroup::Rogue`, `character::canBackstab`,
`character::backstabDamageMultiplier`, `combat::isSweepEligible`, and
`combat::oppositeSide` completely unchanged (all already linked into this
target for other combat features). **Zero `CMakeLists.txt` changes
needed.** Unlike every other feature shipped this session, neither ability
gets a new key binding or `CombatUiState` -- both are automatic,
position/class/HP-dice-driven, exactly as in the console build.

`CombatSession` gained one new fight-long field, `firstAttackerId` (sized
fresh per encounter alongside the other per-instance vectors, NOT reset
per round -- same "the first character to attack it, full stop, for the
instance's whole lifetime" rule the console version documents). Two new
shared lambdas, `combatBackstabBonus` and `combatAdjacentWeakInstances`,
sit right before `combatCompanionActs` (so they're in scope for it, for
`combatResolveAttackAgainstTarget`, and for `combatBeginPlayerAttack`, all
of which come later in the file) and are called from all three: the
player's sweep check (inserted into `combatBeginPlayerAttack` right after
the existing ranged-weapon-adjacency early return, before candidate
gathering -- same placement the console uses), the player's ordinary
attack loop (`combatResolveAttackAgainstTarget`, which needed no signature
change since it already only ever resolves the player's own attacks), and
companion sweep/backstab (`combatCompanionActs`, both inserted right
before its existing single-target `attacks` loop). This build's own
established simplification carries over unchanged: **one target per
round, no mid-round retarget-on-kill** -- sweep doesn't interact with that
since it was never target-picker-driven to begin with. Log lines follow
this build's already-simplified style (no `describeToHit`/`describeDamage`
roll-math breakdown, dropped at commit `c657a44`) -- "You hit the Goblin
for 5.", "Backstab! You hit the Goblin A for 12."

Verified: clean rebuild (zero new `/W4` warnings) and a launch smoke test
against both real saves (`save1.txt`, Mike, level 1 Human Fighter; and
`save2.txt`, Regan, level 20 Human Mage) confirming every catalog still
loads and the window opens with no crash/exception -- this session again
had no desktop/GUI access to actually fight anything. **Not yet
interactively confirmed** -- moved to the Playtest backlog below. Neither
current save carries a recruited companion (checked directly, no
`COMPANION` line in either file) -- since backstab and companion sweep
both need a second party member, and this build's own Dialogue phase
still defers the recruit-accept/decline flow, a companion has to come
from a console-build (`ansalon_rpg`) playthrough saving into the slot
first, same prerequisite the Phase 3 combat backlog entry below already
notes for its own companion-fighting item.

This closes out every item Phase 3 originally deferred -- backstab,
sweep, spellcasting, and item use have all now shipped in this build (only
Phase 3's own deliberately-out-of-scope items, like player-directed party
control, remain out of scope project-wide).

## Full-migration roadmap (screens still ASCII/terminal-only)

Each needs its own real pixel-space design pass -- not a mechanical port,
same reasoning that ruled out the monospace-grid approach above. Rough
size/complexity noted from this session's research, not a commitment to
this order:

- ~~Zone interiors (`drawZoneFrame`)~~ -- done, see above.
- ~~Combat (`drawCombatFrame`)~~ -- core melee loop done, see above;
  spellcasting/item use/backstab/sweep still deferred to a later phase.
- ~~Character sheet~~ -- done, see above; not yet interactively confirmed
  (Playtest backlog below).
- ~~Dialogue~~ -- core conversation loop done, see above; quest/boat/recruit
  choosers still deferred to a later phase; not yet interactively confirmed
  (Playtest backlog below).
- ~~Ask-input (free-text "Ask about something else...")~~ -- done, see
  above, including the `askLimitLocked` greeting override this unblocked;
  quest/boat/recruit choosers stay deferred, unaffected by this; not yet
  interactively confirmed (Playtest backlog below).
- ~~Generic picker overlay~~ -- done, see above (`drawPickerOverlay`,
  used by dialogue's `PickingCandidate`/`TopicPicker`); not a screen of
  its own, but unlocks Shop/Inventory/Spellbook below to reuse it instead
  of each re-deriving the picker loop.
- ~~Shop~~ -- done, see above (buy/sell, reusing `drawPickerOverlay`); one
  quest-locked shop (Flint's Smithy) shows a placeholder instead of
  opening, no quest state tracked in this build yet; not yet
  interactively confirmed (Playtest backlog below).
- ~~Inventory~~ -- done, see above (equip/drink/no-op, reusing
  `drawPickerOverlay` the same way Shop did); not yet interactively
  confirmed (Playtest backlog below).
- ~~Spellbook~~ -- done, see above (the sheet's `s`-key drill-down,
  reusing `drawPickerOverlay` with no selection at all, same as
  Shop/Inventory's own reuse); also fixed a pre-existing Q/Escape bug on
  the character sheet itself while implementing it (see writeup above);
  not yet interactively confirmed (Playtest backlog below).
- ~~Help~~ -- done, see above (`/`, static content ported verbatim,
  reusing `drawPickerOverlay`); not yet interactively confirmed
  (Playtest backlog below).
- ~~Full Log~~ -- done, see above (`V`, scroll-windowed via a new
  `LogSession`, reusing `drawPickerOverlay` for the actual rendering);
  not yet interactively confirmed (Playtest backlog below).
- ~~World Map~~ -- done, see above (`O`, the real map image scaled down
  instead of an ASCII downsample port, plus a side legend); not yet
  interactively confirmed (Playtest backlog below).
- ~~Journal~~ -- done, see above (`G`, a real overlay stating quest
  tracking isn't wired up yet, since this build never populates
  `game::GameState::quests`); not yet interactively confirmed (Playtest
  backlog below).
- Character creation and the save-slot menu use plain `std::cin`/
  `std::cout` before any window exists -- can stay as-is indefinitely,
  not part of this migration.
- Deciding when (if ever) this code gets promoted to replace `ansalon_rpg`
  outright -- only once enough of the above is real, not before.

Milestones 146-152 are all implemented and documented on
`master` (146 terrain smoothing, 147 mountain glyph, 148 region-boundary
highlighting, 150 header/border scroll-drift fix, 151 two more Astinus
SUBJECT entries, 152 Fireball/Delayed Blast Fireball real area damage);
146-150 are also interactively confirmed. The `terrain-smoothing` branch is
fully merged into `master` -- no longer an open decision.

This session also researched (but did not implement, at the user's choice
of scope) two follow-on combat-visuals ideas raised alongside Milestone
152: per-cell color on the combat grid (currently impossible -- `writeBoxed`/
`BoxLine`, the "organic" screen family's rendering, only supports one color
per whole line, see `docs/ARCHITECTURE.md`'s Milestone 32 note) and an
animation/timing primitive (`<chrono>`/sleep, currently absent everywhere in
`src/`). Neither is started; either is a reasonable next step if the user
wants combat visuals to keep moving.

Two things carry over to the next session:

## Playtest backlog

Implemented, documented, and verified via piped-smoke-test, but not yet
interactively walked with a real save/keyboard -- worth clearing before
piling on more unverified content. Full sourcing/detail for each is in its
`docs/MILESTONES.md` entry.

- **SFML loading screen + quit confirmation** -- launch the game and
  confirm a real "Ansalon: Age of Despair" screen with an updating status
  line is visible (even briefly) instead of a blank window before the
  overworld appears; press `Q`/Escape from the Overworld, from inside a
  Zone, and mid-Combat and confirm each opens "Are you sure you want to
  end your adventure?" instead of quitting outright; confirm up/down
  moves the selection, Enter on "No, keep playing" (or Escape/Q) returns
  to the game unchanged, and Enter on "Yes, end my adventure" actually
  closes the window. Also confirm every other screen's own `Q`/Escape
  behavior is unaffected (dialogue/shop/inventory/spellbook/sheet/help/
  world map/journal/log all still close just that screen, not this new
  dialog). See `sfml_phase1/main.cpp` and this file's writeup above, not
  a numbered `docs/MILESTONES.md` entry -- this branch isn't merged to
  `master` yet.
- **SFML combat spellcasting** -- on save2 (Regan, level 20 Human Mage),
  memorize and cast: a single-target damage spell (Magic Missile) at a
  solo monster (should resolve immediately, no target picker, since
  there's only one alive candidate) and at a multi-instance group (picker
  opens, up/down + Enter picks correctly, and the footer reads "Cast
  Magic Missile at which enemy?" not "Attack which enemy?"); Fireball at
  a multi-instance group with 2+ instances within 2 cells of the chosen
  epicenter (both take the same damage, both named in one log line);
  Cure Light Wounds on yourself (no target picker at all, HP increases);
  a buff (Bless/Prayer-equivalent, if memorized) and confirm subsequent
  attack rolls actually reflect it; Web (or another `BlockMonsterAttacks`
  spell) on one instance and confirm its next turn logs "can't bring
  itself to attack!" and is actually skipped, with the counter running
  out on schedule. Also confirm: memorizing 2+ *different* spells opens
  "Cast which spell?" (a real overlay, not the roster-embedded picker);
  Escape/Q on that picker cancels back to Idle with no round consumed
  (HP/round number unchanged); pressing `M` with nothing memorized (or
  nothing left today) logs the right message and doesn't consume a
  round either. Hardest to force but worth a real attempt: get the
  monsters to act first (retry until initiative favors them) on a round
  where you press `M`, and confirm a knockout that round means the spell
  was never actually cast (still shows as memorized afterward). See
  `sfml_phase1/main.cpp` and this file's writeup above, not a numbered
  `docs/MILESTONES.md` entry -- this branch isn't merged to `master` yet.
- **SFML combat item use** -- **neither real save carries anything usable
  yet** (`save1.txt`'s Mike and `save2.txt`'s Regan both show
  `INVENTORY 0`), so this needs buying a Potion of Healing (any shop) and,
  for the Mage-only items, a Webnet and/or Brooch of Imog (Solace's Item
  Shop per `docs/CHARACTER_NOTES.md`'s "Magic items" section) via the SFML
  Shop screen first -- itself still on this same backlog, so confirming
  Shop purchases actually land in inventory doubles as a prerequisite check
  here. Once carrying at least a Potion: press `I` in combat with exactly
  one usable item and confirm it resolves immediately (no picker) --
  drinking a Potion heals HP and ends the round with no picker shown. With
  2+ usable items carried, confirm "Use which item?" opens (a real overlay,
  not the roster-embedded picker) and Escape/Q cancels it back to Idle with
  no round consumed (HP/round number unchanged), same as the spell picker
  above. For Webnet specifically: against a solo monster it should tangle
  immediately with no target picker; against a multi-instance group the
  picker should open and the footer should read "Tangle which enemy?" (not
  "Attack"/"Cast"), and the tangled instance's next turn should log "can't
  bring itself to attack!" and actually skip its turn. For the Brooch:
  activating it once should absorb the *next* monster attack against the
  player specifically (a log line naming "globe of invulnerability") but
  NOT protect a companion, if one is recruited, hit in the same round --
  and confirm it can't be activated twice in the same in-game day
  (`character::broochAvailableToday`'s own gate). If the save carries a
  Staff of Striking/Curing (a quest reward, likely absent on both current
  saves), confirm it heals once per day without being consumed. Hardest to
  force but worth a real attempt: get the monsters to act first (retry
  until initiative favors them) on a round where you press `I` and choose
  an item, and confirm a knockout that round means the item was never
  actually used (Potion still shows in inventory afterward, Brooch's globe
  never actually activated). See `sfml_phase1/main.cpp` and this file's
  writeup above, not a numbered `docs/MILESTONES.md` entry -- this branch
  isn't merged to `master` yet.
- **SFML Phase 3 (combat)** -- win and knockout are confirmed; still
  untested: Flee (`f` during an idle combat round), a multi-instance group
  encounter's in-frame target picker (up/down to cycle, Enter to confirm --
  needs a monster with a `GROUP` line in `data/monsters.txt` to roll more
  than one instance), and a recruited companion fighting alongside the
  player (check whether save1/save2 has one recruited first). See
  `sfml_phase1/main.cpp` and this file's Phase 3 writeup above, not a
  numbered `docs/MILESTONES.md` entry -- this branch isn't merged to
  `master` yet.
- **SFML thief backstab and Fighter sweep** -- **neither current save
  carries a recruited companion** (checked directly), so this needs a
  console-build (`ansalon_rpg`) playthrough that recruits one into
  save1/save2 first (this build's own Dialogue phase still defers the
  recruit flow, so it can't do this itself). For sweep: fight a Fighter
  (or a Fighter companion) against a weak group (Goblin/Kobold/Hobgoblin/
  Skeleton -- HD <=1, `combat::isSweepEligible`) with 2+ adjacent, and
  confirm the log reads "You sweep through the Goblins!" (or the
  companion's own name) followed by one hit/miss line per adjacent
  instance, no target picker opened, and no to-hit/damage bonus applied.
  For backstab: get a Thief-type party member (player or companion) in
  light-or-no armor positioned exactly opposite whichever party member
  first attacked a given instance, and confirm the log line is prefixed
  "Backstab! " with a visibly larger damage number (PHB Table 30's
  level-based multiplier) -- also confirm a Thief attacking from any other
  square, or in armor heavier than Leather, gets no bonus. Both apply
  symmetrically to the player and to companions, so worth checking at
  least one of each direction (player backstabbing around a companion's
  engagement, and vice versa) if the party composition allows. See
  `sfml_phase1/main.cpp` and this file's writeup above, not a numbered
  `docs/MILESTONES.md` entry -- this branch isn't merged to `master` yet.
- **SFML character sheet (+ spellbook)** -- press `C` from the overworld
  and again from inside a zone; confirm every field renders correctly
  (ability scores, saves, weapon/armor, steel/inventory, the
  spells-memorized summary against save2's caster, companions if
  save1/save2 has one recruited), the HP bar reflects current/max HP, and
  an ordinary key (e.g. Enter/W/A/D) dismisses back to the prior screen
  without also moving the character or opening combat. On a caster,
  confirm the footer reads "(s=view spells known, any other key to
  continue)" and pressing `s`/Down opens the spellbook overlay -- check
  its per-level grouping and memorized annotations match the sheet's own
  summary line, and that any key returns to the (still-open) sheet rather
  than closing it. On a non-caster (if one exists in a save slot), confirm
  the footer omits the `s` hint and the spellbook shows "Cannot cast
  arcane magic." if somehow reached. Also confirm the just-fixed bug:
  pressing `Q`/Escape while the sheet (or spellbook) is open dismisses
  just that screen, not the whole window. See `sfml_phase1/main.cpp` and
  this file's Character Sheet and Spellbook writeups above, not a numbered
  `docs/MILESTONES.md` entry -- this branch isn't merged to `master` yet.
- **SFML dialogue (core conversation)** -- press `T` at a zone-native NPC
  (greeting, then topics if any, `TALK_AGAIN` on a second visit) and at a
  canon Hero's scheduled tile (overworld or a zone's `TIMELINE_ANCHOR`);
  confirm a multi-candidate tile picks correctly (up/down, Enter); confirm
  the quest/boat/recruit placeholder log lines appear at a POI marked with
  each (e.g. Kalaman's Curiosities Cart for `QUEST`, Crossing/Port O'Call
  for `BOAT`, Haven or Solace for `RECRUIT`). Also covers the generic
  picker overlay below -- `PickingCandidate`/`TopicPicker` now render via
  `drawPickerOverlay`, a render-path change with no intended visual
  difference, so confirming these two still look right (title, cursor
  list, footer) doubles as that item's confirmation too. See
  `sfml_phase1/main.cpp` and this file's writeup above, not a numbered
  `docs/MILESTONES.md` entry -- this branch isn't merged to `master` yet.
- **SFML ask-input (free-text "Ask about something else...")** -- press `T`
  at Astinus in Palanthas's Great Library (zone `palanthas`, POI `L`) --
  he's the only POI with `ASK_LIMIT` configured (5, extendable to 10) plus
  `ASK_LIMIT_LOCKED`/`ASK_ANYTHING`, so a single NPC exercises the whole
  feature. Confirm: the "Ask about something else..." option appears in his
  topic picker (note `ASK_ANYTHING` means no hint list is shown -- expected,
  not a bug); typing a real keyword (or an unrelated word, to hit
  `SUBJECT_UNKNOWN`) gets a real response; Backspace edits the buffer and
  empty-Enter cancels back to the topic picker; asking 5 questions in one
  day shows the Int+Wis extension roll (`ASK_LIMIT_EXTENDED` text on a pass)
  or the reached-limit message (`ASK_LIMIT` text on a fail) and, on a fail,
  ends the conversation; talking to him again the same day after the limit
  is reached shows the Aesthetic's `ASK_LIMIT_LOCKED` line instead of
  Astinus's own greeting. Also worth a quick check against any ordinary
  zone-native NPC or canon Hero with a plain (unlimited) `SUBJECT` pool, to
  confirm the common case works without any of Astinus's limit machinery.
  See `sfml_phase1/main.cpp` and this file's writeup above, not a numbered
  `docs/MILESTONES.md` entry -- this branch isn't merged to `master` yet.
- **SFML shop** -- press `P` at a shop POI (e.g. Flint's Smithy in Solace
  once `ore_for_the_forge` is complete, or any other shop -- see
  `docs/CHARACTER_NOTES.md`'s "Six shops, six catalogs" for the full
  list); confirm the buy list shows real prices and correctly marks
  already-owned/cannot-use items; buy something and confirm steel is
  deducted and the item lands in inventory; press `I` to switch to the
  sell view, sell something back, confirm steel increases and the
  cannot-sell flag appears on a starting weapon; confirm `Q` returns to
  the zone (not the whole window); confirm Flint's Smithy shows the
  quest-locked placeholder line instead of opening. See
  `sfml_phase1/main.cpp` and this file's writeup above, not a numbered
  `docs/MILESTONES.md` entry -- this branch isn't merged to `master` yet.
- **SFML inventory** -- press `I` from the Overworld and again from
  inside a Zone; confirm the HP/Weapon/Armor(+Shield) header lines and
  the carried-item list render, with an empty inventory correctly
  showing "(nothing carried)" and no cursor; equip a weapon/armor/shield
  and confirm the header line updates to match; drink a potion and
  confirm current HP increases and a result message appears; confirm a
  Webnet/Brooch of Imog/quest item shows its no-op message instead of
  equipping; confirm `Q` returns to the map (not the whole window);
  confirm pressing `I` while a shop is open still toggles buy/sell
  instead of opening this screen. See `sfml_phase1/main.cpp` and this
  file's writeup above, not a numbered `docs/MILESTONES.md` entry --
  this branch isn't merged to `master` yet.
- **SFML help/full log/world map/journal** -- press `/` from the
  Overworld and again from inside a Zone, confirm the command reference
  renders and any key dismisses; press `V`, confirm the event log shows
  the most recent entries first, North/South scroll by a chunk and the
  "Lines X-Y of Z" status line updates correctly, and `V`/`Q` both close
  it; press `O`, confirm the real map image renders scaled down with a
  marker at every location (green for towns, red otherwise) plus the
  player's own marker in the right place, and the side legend lists every
  location alphabetically and stays readable; press `G`, confirm it shows
  the "quest tracking isn't wired up in this build yet" message rather
  than crashing or silently doing nothing. See `sfml_phase1/main.cpp` and
  this file's writeup above, not a numbered `docs/MILESTONES.md` entry --
  this branch isn't merged to `master` yet.
- **152** -- Fireball/Delayed Blast Fireball are now real area attacks
  (radius 2 grid cells, Chebyshev distance). Fight a multi-instance group
  (e.g. Goblins), memorize Fireball, cast it at one instance while a second
  is within 2 cells, and confirm both take the same damage with both named
  in the log; separately confirm a solo/isolated target still reads as a
  clean single-target hit. Save slot 2 (`Regan`, level 20 Human Mage) should
  already have Fireball available.
- **137** -- day-gated Astinus dialogue, fixing 12 shipped spoilers. Talk to
  Astinus in Palanthas before day 2/3/12/17 and again after day 160; confirm
  both halves read correctly.
- **138/139** -- 6 Astinus SUBJECT topics (Kagonesti, gnomes, gully dwarves,
  minotaurs, ogres/Irda, Reorx). Ask Astinus about each.
- **142** -- Harpy/Griffon/Stirge (Monster Manual). Trigger wilderness
  encounters on hills/mountains (Griffon), grassland/hills (Harpy), forest
  (Stirge).
- **143** -- `a_widows_due` DELIVER quest at Kalaman. Talk to the Curiosities
  Cart, find the Furtive Trader POI, deliver the wedding band.
- **145** -- 3 Astinus SUBJECT topics (Fizban, Silvara, Berem/the Everman),
  each day-gated. Ask before/after day 192/69/193 respectively.
- **149** -- 3 Astinus SUBJECT topics (Gilthanas, Lord Soth, Ariakas), each
  day-gated. Ask before/after day 69/190/193 respectively.
- **151** -- 2 more Astinus SUBJECT topics (Alhana Starbreeze, Porthios),
  each day-gated. Ask before/after day 51/70 respectively.

## Parked: SFML rendering + variant tile art (round 2, `sfml-trial-2`)

**Superseded in direction, not necessarily obsolete.** This is round 2's
tile-atlas approach (hand-drawn 32px terrain tiles); the active migration
above instead renders the real map image directly, pixel-space. Still
worth keeping this note around in case tile-based art becomes relevant
again for a screen the real map can't represent (a zone interior, say) --
just don't assume it's the plan for the active migration above.

Not abandoned, not committed to -- purely the user's call whenever (or if)
they revisit it. Don't propose pushing this further unprompted; this project
has a real history of visual-change attempts that didn't land (SFML with no
art, twice-rejected ANSI truecolor shading), and this round's two checkpoints
("looks a bit better", then "good enough, stop here for now") were engaged
with productively but still didn't convert into a commit-to-it decision.

What exists: an SFML window rendering real overworld data with actual
pixel-art sprite tiles (scrolling camera, per-tile animation) hit and fixed a
real bug (a dangling `std::string&` in a hand-rolled JSON parser). On top of
that, Python/Pillow mockups added 6 art variants per terrain code (forest,
grassland, hills, mountains, savannah, bog, salt flat, glacier), picked
per-tile via a stable position hash so large same-terrain regions -- bigger
now after Milestone 146's smoothing -- stop reading as repeating wallpaper.

To resume:

- SFML trial code: `git checkout sfml-trial-2 && git stash pop` (uncommitted
  `CMakeLists.txt` target `ansalon_sfml_trial` + `sfml_trial/main.cpp`).
- Variant art already exists and is proven, just not wired in:
  `References/variant_tiles6.png` (forest/grassland/hills) and
  `References/variant_tiles_extra.png` (mountains/savannah/bog/salt-flat/
  glacier), each a 6-column strip; the position-hash selection logic
  (`(x*73856093) ^ (y*19349663)`) is proven in the mockup script. Porting
  both into `terrain_tileset.json`'s schema (single rect per code -> list of
  rects) and `sfml_trial/main.cpp`'s tile lookup is the concrete next step,
  not a re-derivation.
