# Current work

**2026-09-15 (second session): empty-name bug fixed in both builds.**
The finding from the session below (`CreationStep::Name` accepting an
empty name with no validation) is fixed: `sfml_phase1/main.cpp`'s Name
step now refuses to advance on `wantsEnter` while `nameBuffer` is empty,
showing an inline "Please enter a name before continuing." line (reusing
the existing `stepMessage` mechanism the race/class ineligibility errors
already use) instead of silently accepting it; `promptName()` in
`src/character/CharacterCreator.cpp` gained the same check ("Please enter
a name." + reprompt, same shape as its existing 20-char-limit reprompt).
Both builds rebuilt clean, zero new `/W4` warnings. Verified for
`ansalon_rpg` via the piped path (all three real save slots were
occupied, including slot 3 -- the actual disposable character carrying
this exact bug's saved-blank name -- so the three real saves were moved
aside to the scratchpad, piped `1\n\nTestname\n` confirmed "Please enter
a name." fires on the blank line and a real name is then accepted and
proceeds to ability scores, then the three real saves were moved back
and re-confirmed unchanged). `ansalon_sfml_phase1` only got a launch
smoke test (catalogs load, no crash) -- the interactive Name-step
behavior itself (typing nothing, pressing Enter, seeing the message,
then typing a name and it working) is **not yet interactively
confirmed**, added to the Playtest backlog below. Note: this does not
retroactively fix slot 3's own save, which still carries the blank name
baked in from before the fix -- unaffected either way since that slot is
disposable and headed for `d3` deletion whenever it's no longer needed
(see the quest turn-in note below).

**2026-09-15: console-retirement question raised, deliberately not acted on
yet — session redirected into closing the parity gap instead.** The user
asked to "sunset the ascii build." `docs/CONSOLE_RETIREMENT_PROPOSAL.md`'s
Option A (adopted 2026-09-11) gates even Stage 1 (Deprecate) on a parity
checklist that wasn't met — quest system and native character creation were
both still substantially unconfirmed live. Asked the user which stage they
meant and how to handle the unmet checklist; **chose to hold off on any
retirement action and spend the session live-testing instead**, via the
same `SendKeys`+screenshot automation prior sessions used (confirmed working
again this session — see the updated `feedback_no_desktop_gui_access`
memory). Used a fresh character in the previously-empty save slot 3 (Human
Fighter, Lawful Good, Knight of the Crown — chosen deliberately so the
Knight-offer screen would also get exercised) rather than touching either of
the user's real characters in slots 1/2, both confirmed untouched afterward
(`save1.txt`/`save2.txt` positions unchanged). The app was closed at the end
of the session; slot 3 still holds this disposable test character (Day 6,
Haven area) — harmless to leave, or tell a future session to `d3` it from
the native slot menu if the user wants the slot back to `(empty)`.

**Newly confirmed live, closing real gaps in the Playtest backlog below**
(full detail in each backlog entry; not repeated here): the save-slot
menu's display; native character creation's ability-score assign flow, an
ineligible race pick's inline error + re-prompt (tested against Kender),
the Knight of the Crown offer screen, the Weapon Specialization prompt, and
the final "Begin your journey?" summary screen landing correctly in Solace
with the right stats; the quest system's offer/accept/decline picker, the
journal rendering a live objective counter that actually increments on a
real kill (a `SLAY` objective going 0/3 → 1/3, confirmed twice, and
confirmed to survive a knockout); a `SHOP_LOCKED` shop (Flint's Smithy,
gated on the separate `ore_for_the_forge` quest) correctly refusing to open
("There's nothing to buy here yet."); the Flee command; and, as reconfirmed
bonus coverage rather than new findings, the knocked-out/full-heal/
teleport-to-nearest-town mechanic (now also confirmed for **poison**
damage specifically — "The poison overwhelms you!" still routes through the
same safe path, not a separate save-or-die), battlemap wall-blocking, and
monster-acts-first initiative.

**Genuinely new finding, not just a confirmation**: `sfml_phase1/main.cpp`'s
native character-creation `CreationStep::Name` step accepts an empty name
with no validation — `wantsEnter` on an empty `nameBuffer` just does
`character.name = nameBuffer;` and advances (`sfml_phase1/main.cpp` around
line 1467). Triggered by accident this session (an empty name reached the
overworld HUD as a bare `", level 1 Human Fighter"` and a log line reading
`"Loaded ."`), not by deliberately testing for it. Likely not a new
SFML-only regression — the console's own `promptName()`
(`src/character/CharacterCreator.cpp`) only checks `size() <= 20`, not
non-empty, so this gap probably predates the SFML port. Not fixed this
session (verification-only, not a code-change session); flagging for the
user to decide whether it's worth a minimum-length check. **Fixed in a
follow-up session the same day** -- see the top of this file.

**`road_wolves` reached 3/3 and `ReadyToTurnIn` before this session ended**
(continued in a second pass after the "keep going" ask) — the objective
itself is now fully proven, including the specific one-time log line
`game::GameLoop::checkQuestReadiness` pushes: *"The Wolves on the Solace
Road is ready to turn in -- return to the Notice Board to collect your
reward."*, and the journal's `[x] Kill three timber wolves...` checked
state. Confirmed in the save file too: `QUEST road_wolves 2` (=
`QuestStatus::ReadyToTurnIn`) and `KILL wolf 3`. The path there was almost
entirely encounter RNG, not a code problem — Timber Wolves are one
random-encounter species among ~15+ on the relevant terrain codes
(`world::Terrain.cpp`'s `encounterChancePercent`, 2-12% per step depending
on terrain), so the two sessions combined logged well over 100 non-wolf
encounters (rats, beetles, ghouls, gnolls, hobgoblins, kobolds, ghasts,
goblins, bugbears, giant spiders, zombies, black bears, giant toads,
wraiths, weretigers, gorgons, ettins, owlbears, a troll, draconians of all
three types, worgs [a different monster id, doesn't count], lizard men)
for every wolf pack encountered, and a knockout happened five separate
times along the way before the third kill finally landed clean. **Not a
bug**: the knocked-out-not-killed design (see `docs/COMBAT_NOTES.md`)
means every one of those was a free retry, just an expensive one in real
turns — and each knockout teleports to whichever refuge town is actually
nearest in a straight line (confirmed genuinely computed, not hardcoded:
landed in Haven, Solace, and Ice Wall Castle on different occasions).

**Still not closed: the actual turn-in dialogue, `COMPLETE` text, and
reward.** The third kill landed while the character was deep in the
Balifor region chasing wolf encounters — a random-encounter knockout
teleported it to **Port Balifor**, roughly 166 overworld tiles
(Chebyshev) from Solace's Notice Board, confirmed via `POS 357 182` vs.
Solace's `POS 191 203`. Walking that back was judged not worth the
remaining session budget — the objective-complete mechanics this was
actually meant to verify (live counter increment, knockout-survival,
`ReadyToTurnIn` state, and its log line) are now all proven regardless of
whether the steel/XP reward text itself has been seen fire. **For a
future session finishing this off**: the disposable slot-3 character
(Human Fighter, `ZONE port_balifor`) is sitting inside Port Balifor's
interior zone, ready to turn in `road_wolves` the moment it reaches
Solace again — a real, long overworld walk (or `ansalon_rpg`'s faster
piped/no-render path could in principle reach the same state on a fresh
character much more directly, if the goal is only to see the `COMPLETE`
text/reward/journal-clear fire, rather than to reuse this specific
character). Also still open from the quest checklist: any `DELIVER`-type
quest (`ore_for_the_forge` was found and confirmed locked, but its own
offer/accept/turn-in was never walked), and the six reward flags /
Wayreth Test of High Sorcery scene.

**Two more confirmations from this second pass**: the save-slot menu's
**Continue** branch (selecting an occupied slot, confirming "Continue this
character?", resuming at the exact saved position/HP/day) — including
surviving an actual unexpected process exit, not just a graceful quit (see
below); and `r` = Rest (heals a small amount, once per day — "You've
already rested today" on a second attempt same day).

**Minor operational note, not a confirmed game bug**: the app's window
closed cleanly and unexpectedly twice during this session's extended
automated `SendKeys` play (no crash dump/Application-Error event logged
either time — a clean exit, not a crash). Both times the disposable
character's autosave was fully intact on relaunch (continuous
per-keypress autosave doing exactly its job). Reads as an automation
artifact (a very high volume of synthetic input over a long session)
rather than a game-logic bug; flagging only so a future session isn't
surprised if it recurs, not as something to chase down.

**Console-retirement trigger: still not met, and this session shouldn't be
read as having met it** — quest parity is now very close (offer/accept/
track/lock/ready-to-turn-in all live-confirmed with the `road_wolves`
SLAY path fully proven end to end) but not done (the actual turn-in
exchange for `road_wolves` was never seen fire — the character is 166
tiles from Solace, see above — `DELIVER` quests untested, reward flags
untested). Native character creation is also closer (Continue confirmed
alongside pick-empty-slot) but still has open sub-items (the final
summary's "No" restart path, Elf/Dwarf subrace step, Gnome-forced-Tinker
path, and the save-slot menu's overwrite/delete branches). See `docs/
CONSOLE_RETIREMENT_PROPOSAL.md`'s four-point checklist and the Playtest
backlog below for exactly what's left. Don't resume the retirement
conversation unprompted — wait for the user.

**Playable v7 packaged 2026-09-13** (`dist/AnsalonRPG-Playable-v7.zip`,
`tools/playable_release_version.txt` bumped 6->7 via `package_playable_release.ps1
-Major`): bundles everything shipped since v6 (Sep 11) -- Milestone 190
(multi-square creatures + Blue Dragon), the target-picker cancel fix,
Milestones 191/192 (combat sprite art for the player and Bren Alder), and
this session's monster-sprite-art batch (loader wiring for `-wide`/
`-tall`/`-four` art, the neighbor-aware overlap sizing fix, chroma-key
transparency across all 32 sprite files, the divider-bar/border crop
fixes, and 30 of 44 monsters now with real art). Classified as a major
increment per CLAUDE.md's rule: real sprite art (replacing placeholder
circle+letter combat markers) and multi-square creatures are new
player-visible capability, not just a bug-fix/cleanup pass. Verified via
a Release-config build (clean, via the packaging script itself) and a
launch smoke test of the packaged exe against a disposable copy of
`save1.txt` (4s, no crash). Committed and pushed to `origin/master`.

**In flight, 2026-09-12: user downloading/generating monster sprite art,
in progress across multiple passes.** First batch of 30 dropped into
`assets/sprites/`; `loadCombatSprite` now wired to find them (see just
below). Still no art at all for 14 of 44 monsters: **spider, bugbear,
ogre, bozak, thanoi, ettin, harpy, griffon, stirge, giantrat, wereboar,
weretiger, brownpudding, shamblingmound** -- `brownpudding` confirmed
still genuinely needing art: a `blackpudding-wide.png` turned up in this
batch but matched no roster id and didn't visually resemble a pudding at
all (a pointed blue/teal creature, not a blob like
`whitepudding-wide.png`) -- flagged to the user, who confirmed it was a
bad/mismatched download; deleted 2026-09-12, not renamed to brownpudding.

**File-naming convention (user's own), now wired into
`sfml_phase1/CombatSprite.cpp`'s `loadCombatSprite`**: plain `<id>.png`
still works unchanged; `loadCombatSprite` also now tries `<id>-wide.png`
(2x1) / `<id>-tall.png` (1x2) / `<id>-four.png` (2x2) in that order,
recording which one matched as the new `CombatSpriteFrames::renderWidth/
Height`. `dragon_blue-four.jpg` was also re-saved as a real
`dragon_blue-four.png` this session (the loader only ever tries `.png`,
and a `.jpg`-content file merely renamed wouldn't reliably round-trip) --
old `.jpg` deleted.

**Rendering size, corrected once live-played (two rounds so far)**:
first pass (this same session) assumed the suffix was purely an
art-canvas-proportion choice and left `drawCombatSpriteToken`'s bounding
box keyed only to the monster's real gameplay footprint
(`Monster::footprintWidth/Height`) -- reasoned that this kept `-wide` art
on a still-1x1-footprint pack monster (wolf, gnoll, icebear, worg,
giantoad, centipede, boringbeetle, blackbear all got suffixed art despite
staying `GROUP` 2-4 pack monsters) safely scaled down to a normal
single-cell box, untouched by `MonsterLoader`'s existing
footprint-area-must-be-solo rule. **The user then actually fought a Black
Bear live and found it rendered as one square** -- confirmed the intent
is for the art itself to visibly occupy 2+ squares regardless of gameplay
footprint. First fix: bound box became `max(real footprint,
frames.renderWidth/Height)` per axis, always centered on the token's
existing position.

**That centered version then hit a second live problem**: two adjacent
wolves in the same pack (each still 1x1 gameplay-footprint, standing in
neighboring cells) each drew their own "-wide" box centered on their own
cell, so the boxes overlapped in the shared middle cell -- screenshotted
by the user, visibly two wolf sprites (and separately the player token)
colliding with hard rectangular edges (compounded by the sprites' known
non-transparent mint-green background, same cosmetic issue already on
`player.png`/`bren_alder.png` -- see Milestone 191/192's own notes; NOT
yet addressed, see below). **Fixed**: `drawCombatSpriteToken` now takes a
`neighborFree(dx, dy)` checker; the two real call sites (monster tokens in
both `combatAnimateAiStep` and the main render branch) build one from
`combatCellOccupied` (moved earlier in the file, right after
`combatCompanionAlive`, so both call sites can reach it -- pure relocation,
same logic) plus a player-position check. A "-wide"/"-tall"/"-four"
sprite now only expands into a neighbor cell confirmed empty *this
frame*; if both sides on an axis are occupied it falls back to the plain
footprint-sized box instead of overlapping either neighbor; if only one
side is free it biases fully toward that side (`shiftX`/`shiftY`) instead
of always centering. Companions/player pass an always-true checker since
neither has suffixed art yet (dead code today, cheap to keep uniform).
`troll-tall.png`/`dragon_blue-four.png` are unaffected either way since
their art already matches their real multi-cell `SIZE` (Milestone 190) --
extra is always 0 for them, so the neighbor check never even triggers.
`ogre` and `griffon` have that same real `SIZE` but no art yet (see the
missing-14 list above).

**Known limitation, accepted rather than solved**: this is a per-token
heuristic, not a layout solver -- two "-wide" pack members could still
independently decide the *same* single gap between them is free and each
grow halfway into it (a smaller, softer overlap than before, not a full
one). Good enough for a cosmetic nicety; revisit only if it actually looks
bad in practice.

**Chroma-key transparency fix, done this session (asked first, approved)**:
the sprites' shared solid mint-green background had no alpha channel, so
overlapping/adjacent sprites showed hard-edged color blocks instead of
blending naturally. Confirmed the exact fill (`RGB 103,247,159`, one file
-- `spectre.png` -- off by a few units at `100,244,156`, likely
generation-pipeline variance) is used as pure background across all 31
sprite files that existed before this pass (`player.png`/`bren_alder.png`
included, the same "known cosmetic issue" flagged back at Milestones
191/192), confirmed via full per-file color histograms that no
creature's own palette comes remotely close to that hue, and that every
file has a separate, deliberate opaque-black canvas border/outline
(unrelated -- pixel-art linework, left untouched). A one-off script
(`dechroma.py`, not checked into the repo -- this was a data cleanup
pass, not new tooling this project keeps around) set alpha=0 for every
pixel within Euclidean distance 30 of the background RGB, across all 32
files (the 31 plus `bren_alder.png`), leaving black linework and every
creature's own colors untouched. Spot-verified pixel-level (background
alpha goes to 0, black border pixel stays 255) and by compositing several
results against the actual floor-tile color (`RGB 70,65,55`) -- clean
blend, no green fringe, thin black outline remains (expected, matches the
existing pixel-art style). This was flagged as a bulk irreversible-
looking file rewrite by the session's own safety classifier even though
every file is git-tracked (so fully revertible); the user approved
running it after seeing the scratchpad-verified result. Both this and the
neighbor-aware overlap fix above were committed together (`3968cb8` code,
`f2e45b1` chroma-key).

**Confirmed live 2026-09-12, then two more findings from that same
screenshot**: the user fought a wolf pack and screenshotted it -- the
neighbor-aware overlap fix worked (no more overlapping wolves) and the
chroma-key worked (no green background), but surfaced two more real
issues, both fixed this session and NOT yet the ones re-confirmed live:

1. **A solid black bar down the right edge of every single sprite --
   then, after fixing that, a second live finding: a black line along the
   top and bottom of every sprite too.** Root cause: every sprite sheet
   (all 32, confirmed via a full pixel scan) has a real 1px opaque-black
   border around the *entire* canvas (all four edges, not just left/
   right) plus a much thicker 8px opaque-black divider between the
   idle|attack halves -- `computeSpriteFrameRects`' original 50/50-
   midpoint split baked half that divider into each cropped frame's
   right/left edge, and the first fix (below) only trimmed the left/right
   border, missing the identical top/bottom one. Fixed in two passes:
   `computeSpriteFrameRects` now takes an `sf::Image` (not just width/
   height) and detects opaque-black, fully-covering border rows (top/
   bottom, trimmed first, uniformly across both frames since there's no
   idle|attack split on that axis) and border/divider columns (left/
   right, searched only within the row-trimmed content height) by
   scanning for fully-opaque-black runs; falls back to the old plain
   half-width split if zero or more than one interior divider column run
   is found (art without this convention keeps working). `loadCombatSprite`
   now loads an `sf::Image` first (to inspect pixels) before building the
   GPU texture from it, instead of loading straight to `sf::Texture`.
   Verified via a throwaway self-test (`CombatSpriteSelfTest.cpp` + a
   temporary CMake target, both deleted after, per this project's
   self-test convention, run once after each of the two passes) -- 21
   checks: synthetic full-border+divider, synthetic no-divider fallback,
   ambiguous-divider fallback, odd-width/empty-image rejection, and a real
   regression check loading the actual `assets/sprites/goblin.png` and
   confirming its final computed rects (`(1,1)`-`24x24` idle,
   `(33,1)`-`24x24` attack) match the file's real, by-hand-confirmed pixel
   layout on all four edges.
2. **A "-wide" wolf boxed in between two packmates rendered as a visibly
   tiny wolf** ("leaves a small image of itself lol" -- the user's own
   words) rather than falling back to a normal-looking single-cell
   sprite. Root cause: the boxed-in fallback correctly stopped expanding
   the *bounding box*, but still displayed the *entire* oversized (~2:1
   aspect) frame contain-fit into a square 1-cell box -- since contain-
   fit preserves aspect ratio, a 2:1-wide frame squeezed into a square box
   ends up far shorter than a normal, roughly-square creature frame gets
   in that same box. Fixed: when an axis can't expand (`widthBoxedIn`/
   `heightBoxedIn`), `drawCombatSpriteToken` now crops the *source*
   texture rect itself down to a centered square-ish sub-region (sized to
   the frame's own shorter dimension) before the contain-fit math, so a
   boxed-in oversized sprite renders at a normal, comparable size
   (cropped, losing some left/right or top/bottom art) instead of shrunk
   whole. A "-four" sprite boxed in on both axes at once crops both
   correctly (computed from the original frame's dimensions, not
   compounded). No occupancy/footprint/GROUP changes -- purely which
   pixels get drawn and how big.

**Verified this session**: clean `/W4` rebuild of all three targets (zero
new warnings) and an `ansalon_sfml_phase1` launch smoke test against a
disposable copy of `save1.txt` (ran 4s, no crash) after every change this
session (loader wiring, first render-size fix, neighbor-aware overlap
fix, chroma-key, divider-crop fix pass 1 [left/right only], boxed-in-crop
fix, divider-crop fix pass 2 [top/bottom, after the second live finding]).
**Confirmed live 2026-09-12**: kobold's sprite rendered correctly in a
real fight (plain 1x1 art, no suffix); the neighbor-aware overlap fix and
chroma-key both confirmed live via the wolf-pack screenshot; the
left/right divider-bar fix was live-tested immediately (screenshotted by
the user) and while it removed the *side* bar as intended, it surfaced
the top/bottom border as a separate, previously-hidden issue -- now also
fixed (pass 2 above). **Not yet interactively confirmed**: the top/bottom
border trim (pass 2) and the boxed-in-crop fix (a fully-surrounded
"-wide"/"-tall"/"-four" sprite rendering at a normal size, cropped,
instead of tiny) -- the former just needs any real fight with sprite art
(should be visible on every single token), the latter needs a real fight
with 3+ same-suffix pack members adjacent (a wolf/gnoll/icebear/worg/
giantoad/centipede/boringbeetle/blackbear pack of 3-4 is the easiest way
to force the boxed-in case specifically). Add to the Playtest backlog
below once the blackpudding question above is resolved and this batch is
considered closed.

**Milestone 192 shipped 2026-09-12** (same session as 191, immediately
after): companion sprite art for Bren Alder. The user supplied
`References/Sprites/BrenAlder.png`; moved to `assets/sprites/bren_alder.png`
(`bren_alder` is his existing companion id) -- Milestone 191's id-keyed
mechanism meant **zero code changes were needed**, only two stale "only
player has art" comments updated. Verified via clean `/W4` rebuild (zero
new warnings) and a launch smoke test against a copy of `save1.txt`.
**Confirmed live 2026-09-12** (same session, against a fresh disposable
copy of `save1.txt`, driven via `SendKeys` + screenshot -- see this
day's own desktop/GUI access note elsewhere in this file): walked out
from Haven until a 4 Giant Centipede encounter fired, and both the
player's Knight sprite and Bren Alder's real sprite (red shirt/green
pants, sword raised) rendered side by side on the grid, in place of the
plain circle+letter markers. Same known cosmetic issue as `player.png`
(no alpha channel, solid mint-green background block shows behind the
character) -- confirmed present on Bren Alder's sprite too, left as-is,
same call as Milestone 191. Note: companions are never passed a facing
target (`drawCombatSpriteToken`'s `facingTarget` argument is
`std::nullopt` for companion tokens, only the player gets
`combatNearestLivingEnemyPos()`), so Bren Alder's sprite doesn't mirror
left/right -- by design, not a bug; only the player's facing was ever in
scope for Milestone 191's cosmetic flip.

**Milestone 191 shipped 2026-09-12** (a fresh session, after the
character-wounded playtest below was already paused): combat sprite art
for `ansalon_sfml_phase1` -- the first real art in the SFML target,
replacing the plain circle+letter marker with a real idle/attack sprite
for the player's own combat-grid token, prompted by the user sharing a
side-view fighter sprite pair (`References/Knight.png`, moved to
`assets/sprites/player.png`). Built as a general, id-keyed lookup
(`combat::Monster::id`/`game::RecruitedCompanion::id` are the natural
future keys) rather than a player-only special case, per the user's own
explicit scoping choice -- companions and monsters keep their existing
markers until art exists for them, with zero further code changes needed
when it does. The sprite mirrors left/right to face the nearest living
enemy (purely cosmetic -- confirmed via `docs/COMBAT_NOTES.md` that
backstab eligibility is already a pure position check,
`combat::oppositeSide`, with no facing concept, so this doesn't touch it)
and briefly flashes an attack-pose frame on every one of the player's own
swings before reverting to idle. See `docs/ARCHITECTURE.md`'s SFML
section ("Combat sprite art") for the full design and
`docs/MILESTONES.md` entry 191 for the shipped writeup. Verified via a
throwaway self-test (16 checks, deleted after), a clean `/W4` rebuild of
all three targets (zero new warnings), and an `ansalon_sfml_phase1`
launch smoke test against real `save1.txt` (every catalog still loads;
sprite loading is lazy, first attempted only once combat starts).
**Confirmed live 2026-09-12** (same session, against a copy of
`save1.txt`): the sprite renders in place of the marker, mirrors to face
the nearest enemy correctly, and the attack-pose flash plays and reverts
-- see this file's own Playtest backlog entry for the one known cosmetic
follow-up (a non-transparent background in `Knight.png` itself, left as
is for now per the user). Asked the user at hand-off whether to bump
`tools/playable_release_version.txt` as a minor increment for this --
**held off for now**, pending the cosmetic cleanup or bundling with
other pending work before the next release is packaged.

**Right now: character wounded (HP 1/26), rest before wandering off
again -- but read the correction below before treating this as an
emergency.** `save1.txt`'s Mike is at `MODE OVERWORLD`, `POS 197 249`
(open ground a few tiles north of Tarsis, near Tower of Tears), Day 16,
02:00 -- the result of fleeing a 4-Giant-Toad encounter this session by
quitting the app rather than risking another attack roll (see
"Live-testing session, 2026-09-12 (part 2)" below for the blow-by-blow).

**Correction, caught only after quitting**: `docs/COMBAT_NOTES.md`'s own
"Death: knocked out, not killed" section (a deliberate, pre-existing
design decision, not something from this session) means the player
*cannot actually die* -- hitting 0 HP fully heals to max and teleports to
the nearest refuge (`isTown`/`seaLocked`), no run-ending, no save
deletion. Tarsis itself is one of the five refuge towns and was only
~25 tiles away. So the in-the-moment framing below ("one more hit kills
him") **overstated the actual risk** -- worst case really was a free
full heal plus a free trip to the exact town this session was trying to
reach anyway. Quitting to preserve HP 1 was still a reasonable
conservative call *given what was known at the time* (this section
hadn't been checked yet), and it did avoid burning the still-untested
"attack-picker has no cancel" bug's worst case for real, but a future
session hitting the same spot should feel free to just keep fighting --
0 HP is an inconvenience (teleport + lost position), not a threat. Resting
next session is still good practice before wandering back out, just not
urgent in the way this was first written up.

**Live-testing session, 2026-09-12 (part 2 -- this session, via the
VSCode extension host, not the earlier terminal session below).**
Confirmed this session has real desktop/GUI access (`SendKeys` +
`GetClientRect`/`CopyFromScreen` via a DPI-aware PowerShell helper --
see updated memory `feedback_no_desktop_gui_access`: this is
session-launch-dependent, not a universal restriction, so retest cheaply
each session rather than assuming either way). Used it to finally reach
a zone this marathon-session's character had never entered (see part 1's
own note below) and clear a good slice of the Playtest backlog:

- **Reached Tarsis** (first zone entry all session) and confirmed zone
  walls block movement the same as overworld walls ("Blocked: cannot
  walk onto a wall.").
- **Look command inside a zone** (Milestone 182) -- confirmed both
  halves: a POI *with* dialogue (`R`, A Knight's Runner) shows its full
  description via `l`; a POI *without* dialogue (`D`, The Old Dock, also
  the zone's `TIMELINE_ANCHOR`) correctly falls back to "Nothing else
  catches your eye here." with no Hero scheduled there today -- this is
  by design (`gatherLookCandidates` only surfaces dialogue-bearing POIs
  plus anchor-tile Hero presence; a plain POI's description is already
  shown once via the "Here: X." arrival line), not a bug. An actual
  Hero-present-at-a-`TIMELINE_ANCHOR` Look still couldn't be forced --
  every canon Hero was 50+ tiles away at Pax Tharkas today, an
  overworld-travel-time away no single session can close.
- **Ask-input** (Milestone 162) -- confirmed real keyword matching
  (typed "dragons" at A Knight's Runner, an unlimited-`SUBJECT` NPC,
  got the correct `SUBJECT` text), Backspace editing the buffer, and
  empty-Enter cancelling back to the topic picker. Only the
  Astinus-specific pieces (`ASK_LIMIT_LOCKED` greeting override,
  extension-roll fail path) remain open, needing Astinus specifically.
- **Inventory equip** (Milestone 165) -- confirmed: bought Leather Armor
  and a Shield at Tarsis's forge, equipped both via `i`, watched the
  header line go "Armor: none" -> "Armor: Leather Armor" -> "Armor:
  Leather Armor + Shield", and confirmed the character sheet's AC
  updated to match. Quest-item no-op message still unconfirmed (no quest
  item carried).
- **Movement: Space genuinely holding/ending a turn** (Milestone 185,
  last open piece) -- confirmed: holding against 4 Skeletons logged "You
  hold your action." + each monster "closes in.", sidebar distances
  updated. Also reconfirmed Fighter Sweep (explicit "You sweep through
  the Skeletons!" line, one roll per adjacent 1-HD target) and the
  manual "Attack which enemy?" picker for tougher multi-HD monsters
  (Giant Toads, which don't qualify for Sweep).
- **Two findings from that session, triaged 2026-09-12:**
  1. **FIXED: the "Attack which enemy?" target picker had no working
     cancel.** Root cause: the existing Escape/Q cancel guard (checked
     ahead of the general quit-confirm branch, right where
     `wantsQuit && !askInputActive` is handled) only listed
     `PickingSpell`/`PickingItem`/`ViewPicking` -- `PickingTarget`
     (the Attack/Spell/Webnet target picker) was simply missing from
     that condition, so Escape/Q fell through to the top-level
     quit-confirmation instead (itself safe to back out of via "No,
     keep playing", but that returns to the *same* picker, not Idle).
     Fix: added `CombatUiState::PickingTarget` to that same guard.
     Escape/Q now returns straight to Idle, from where `i`/`f`/Space all
     work normally -- this closes the actual gap ("no way to change
     your mind and drink a potion or flee instead"); `i`/`f`/Backspace
     were never meant to work *from inside* the picker itself (no other
     sub-picker in this file supports that either), so nothing further
     was needed there. One nuance documented inline at the fix site: for
     `TargetPickReason::Attack` this is a genuinely free cancel (nothing
     committed yet -- the swing only happens once a target is
     confirmed), but for `Spell`/`Webnet` the spell/charge was already
     consumed by `combatCommitSpellChoice`/`combatCommitItemChoice`
     *before* the picker opened, so cancelling those doesn't refund it
     -- it just leaves the round unfinished until the next action closes
     it out, an accepted "you wasted it" consequence, not a bug. Found
     live, the hard way, at 1 HP against 4 Giant Toads. Verified via a
     clean `/W4` rebuild and a launch smoke test; **not yet
     interactively confirmed** -- needs a real combat with 2+ eligible
     attack targets, Escape/Q pressed mid-picker, confirming it lands on
     Idle and that Flee/Item/Space all work immediately after. Added to
     the Playtest backlog below.
  2. **Investigated, no code bug found: the combat sidebar's `dist N`
     readout appeared to lag one action behind.** Traced the full
     render path: the main loop drains all queued key events (which
     includes `combatMonstersAct`/`combatCompanionActs` mutating
     `combatSession.instancePositions` synchronously), *then* calls
     `window.clear()` and redraws the whole frame -- map, tokens, and
     the sidebar's `dist N` text -- reading those same live positions,
     all within the same loop iteration, before `window.display()`.
     There's no cached/stale copy anywhere in that path the sidebar
     could be reading instead. Best explanation given the code is clean:
     a screenshot-capture timing artifact of that session's automation
     (SendKeys + a separate `CopyFromScreen` grab, an external process
     not synced to SFML's own `window.display()` calls) catching the
     previous frame before the compositor shows the just-submitted one
     -- consistent with "didn't affect the actual combat math, just the
     displayed number" and a second action "catching up". No code
     change made. If this comes up again on a future live session,
     confirm by adding a short (~150-250ms) settle delay between sending
     the keypress and capturing the screenshot before assuming it's a
     real rendering bug.
- **Confirmed, via `data/overworld.grid`, why the Milestone 190 Blue
  Dragon / salt-flat wall backlog items can never be forced through
  ordinary play**: salt flat's glyph (`_`) has zero occurrences anywhere
  in the actual 480x320 grid -- already documented as an honest,
  deliberate gap in `docs/MAP_NOTES.md` ("`salt_flat` still doesn't
  appear anywhere"), not something this session broke. The Dragon and
  that specific wall-terrain confirmation stay permanently backlogged
  behind either a hand-edited grid tile or a debug save, not "walk
  around enough and it'll come up."
- **The HP 1 sequence itself** (see the correction above for why this
  wasn't actually life-threatening): a 4x Giant Toad encounter (hills terrain)
  chipped Mike from full to 1 HP over two rounds (8+2, then 5+4 damage
  against AC 5) faster than expected. With the target-picker cancel bug
  above blocking a mid-picker retreat to the potion, and `save1.txt`
  already having HP 1 written to disk (autosave is continuous, even
  mid-combat), the only safe option left was quitting the app outright --
  combat state itself isn't part of the save format (only
  `MODE`/`POS`/character stats are), so quitting mid-fight is equivalent
  to fleeing it entirely. Confirmed this actually works: the reload
  target is `MODE OVERWORLD POS 197 249`, no toads, HP 1/26 intact. User
  explicitly chose this over gambling on another attack roll.

**Milestone 190 shipped 2026-09-12** (sixth and final part of the Gold
Box-style battlefield chain): `ansalon_sfml_phase1`'s combat grid gained
real multi-square creatures. The chain's own motivating case (the DQoK
screenshot's 2x2 dragons) had no roster monster to exercise it, so this
milestone both built the mechanic and added a real monster for it, per
the user's own explicit direction: a new `Monster::footprintWidth/Height`
(`SIZE <w> <h>` grammar line) applied to Ogre/Troll (1x2, tall), Griffon
(2x1, wide), and a new sourced **Blue Dragon** (`dragon_blue`, 2x2) --
the user's own stated Gold Box footprint convention, not a 2e stat. Five
new `combat::` helpers (`footprintCells`/`isAdjacentToFootprint`/
`footprintFits`/`nearestFootprintCell`/`stepFootprintToward`) back every
footprint-aware call site (melee/ranged eligibility, opportunity attacks,
companion/monster adjacency, occupancy, Milestone 189's `hasLineOfSight`
endpoint, rendering), each degenerating to the exact pre-existing 1x1
behavior for every ordinary monster. The Aurak's previously-hardcoded
breath weapon was generalized to data-driven fields (byte-identical
behavior, verified) so the Dragon's lightning breath could share the same
skeleton. See `docs/COMBAT_NOTES.md`'s "Multi-square creatures + a real
Dragon" section and `docs/MILESTONES.md` entry 190 for the full writeup,
including sourcing (Monster Manual p.66's rendered page image + Champions
of Krynn's own bestiary) and what's deliberately out of scope (no
multi-cell BFS pathing, no Pegasus, Ettin left at 1x1). `ansalon_rpg`
stays completely untouched. Verified via a throwaway self-test (19
checks, deleted after), a clean `/W4` rebuild of all three targets, an
`ansalon_sfml_phase1` launch smoke test (44 monsters load, up from 43),
and a piped `ansalon_rpg` character-creation run. **Still not
interactively confirmed as of 2026-09-12** (see the live-testing session
note just below): specifically an Ogre/Troll/Griffon/Dragon's 1x2/2x1/2x2
footprint actually rendering/behaving right -- despite ~50 real random
encounters fought/fled this same session hunting for one, none of the
four came up (the roster's other ~40 monsters kept winning the roll).
See the Playtest backlog below.

**This closes the six-part Gold Box battlefield chain (185-190) in
full.** Nothing further in this chain is queued -- the next session
should offer a fresh menu of backlog options rather than assume a
follow-on.

**Live-testing session, 2026-09-12 (part 1 -- an earlier session that day
gained real keyboard/
screenshot control of `ansalon_sfml_phase1` mid-conversation -- a
capability prior sessions didn't have, see the user's own
`.claude/settings.local.json` `PowerShell(*SendKeys*)`-family permission
rules that unlocked it).** Confirmed live this session (folded into
their own Playtest backlog entries below, not repeated here): the
`Movement: N/Max` readout counting down correctly (Milestone 185), real
wall-blocking + visually distinct wall tiles across 4 of 9 terrains
(Milestone 188), and the Look command's overworld compass-direction
fallback (Milestone 182). **Character is currently far from any town**
(wandering combat-testing pushed `save1.txt`'s live character from
Solace all the way to the Tarsis/Kharolis Mountains region, day 5 ->
day 14 in-game) with the companion Bren Alder knocked out (0 HP, not
dead -- a real consequence of extended combat exposure this session, not
a bug) -- expect this state on resume, don't treat it as data corruption.
Still open from this session's own attempt list: reaching any zone/town
at all (blocked by the ~50-encounter detour above), so quest system/
dialogue/ask-input/inventory-equip/native-character-creation backlog
items below remain exactly as untested as before this session. Also
learned the hard way and worth recording: the overworld camera's pixel
scale is small enough (~17px/tile at `world pixel size 8192x5461` over a
480x320 grid) that a handful of steps produces no visible movement on a
screenshot -- don't judge "did that step work" by eyeballing a screenshot
crop; either send a large batch (15-20+ presses) toward a `Look`-reported
compass direction and re-check, or don't try to pixel-navigate to a
precise tile at all.

**Milestones 185-187 shipped 2026-09-11** (the first three of a now-six-part
Gold Box-style battlefield chain, requested by the user against a real Dark
Queen of Krynn screenshot): `ansalon_sfml_phase1`'s combat grid grew from
15x9 to 50x25 with a scrolling camera, and movement is now a real
per-round budget (`character::movementSquares`/`Monster::moveSquares`,
sourced from DQoK.pdf p.51 and three SSI Gold Box games' own bestiaries)
instead of one step ending the round outright. `ansalon_rpg` deliberately
keeps its own 15x9 grid unchanged — see `docs/COMBAT_NOTES.md`'s "Bigger
battlefield and real per-round movement" and `docs/PARITY_MATRIX.md`'s
Combat row for the full writeup. Live playtesting the same day surfaced
and fixed three follow-on issues (monster/companion moves teleporting
instead of animating step-by-step, stray trail-line artifacts during that
animation, and the new `dist N` sidebar text clipping off-screen for a
long monster name) — see COMBAT_NOTES.md's "Live playtest follow-ups"
subsection. Milestone 186 then added a `v` = VIEW command (a read-only
stat card for any unit, no round cost) and status-effect tags surfaced
both on that card and passively in the sidebar roster, prompted by the
user sharing `References/BattleFrames.zip` (gameplay frames from the same
SSI trilogy) — see COMBAT_NOTES.md's "VIEW command and status-effect
visibility" section. Milestone 187 then added keypad diagonal movement
(numpad, or Home/PageUp/End/PageDown with NumLock off) across the
overworld, zone interiors, and combat, with corner-cutting blocked for
overworld/zone (not combat, which has no walls yet) — see
`docs/MAP_NOTES.md`/`docs/ZONE_NOTES.md`/`docs/COMBAT_NOTES.md`'s own
"Keypad diagonal movement" sections; this deliberately does NOT reverse
`docs/MILESTONES.md` entry 63's earlier removal of diagonal movement,
which was about the console build's letter-key scheme overlap
specifically, not diagonals themselves — `ansalon_rpg` is untouched, still
`wasd`-only. Walls (188) then shipped 2026-09-12 — see the note at the
top of this file. Each remaining chain entry is its own milestone,
SFML-only, sequenced this way at the user's own request.

Otherwise nothing else in-flight code-wise. Playable v7 is done: P1 (quest system +
Look command ported to SFML, Milestones 182-183), P2 (save hardening,
Milestone 184), and P3 (`docs/PARITY_MATRIX.md`) all closed 2026-09-11 —
`docs/NEXT_STEPS_v7.md` and `docs/PARITY_MATRIX.md` have both been
reconciled against Milestones 182-184 (no more stale rows/priorities;
this replaces an earlier version of this note that flagged them as
drafts needing reconciliation). `tools/playable_release_version.txt`
bumped 5→6 as a placeholder ahead of actually packaging (confirmed with
the user 2026-09-11: this work is "Playable v7" — the file's own
auto-increment will land there once `package_playable_release.ps1
-Major` actually runs, since `-Major` always adds one to whatever's
currently stored; don't read the "6" sitting in the file right now as
the final number for this milestone), matching the major-increment rule
(new player-visible systems: quests, Look, plus a save-format-relevant
hardening pass). `ansalon_sfml_phase1` (the
primary/main build) now has real quest/mechanic parity with
`ansalon_rpg`. Every subsystem a three-way Haiku-agent audit checked
2026-09-11 (character creation, save/load, companion recruitment,
overworld/timeline encounters, TALK/SHOP/PORTAL zone grammar, dialogue,
combat, inventory, leveling, and now quests/Look) is genuinely complete
in code — see `docs/MILESTONES.md` entries 157-184 for the full shipped
history; this file no longer repeats that narrative.

Console-retirement trigger is **decided but not met**: Option A from
`docs/CONSOLE_RETIREMENT_PROPOSAL.md` (parity-based, gating
Stage 1/Deprecate), recorded in `docs/ARCHITECTURE.md` — Stage 0 (both
targets fully live) still applies today. `ansalon_rpg` stays in the tree
as a legacy/reference build, unchanged.

**Right now: mid live playtest session of `ansalon_sfml_phase1`, paused,
resume here.** Most of the migration is confirmed working live at the
user's own keyboard (2026-09-09 through 2026-09-11) -- see each
milestone entry above for its own confirmation status. What's left is
the Playtest backlog below: continuing today's native-character-creation
test (Milestone 180), confirming the newly-ported Look/quest system
(Milestones 182-183), a handful of narrower unconfirmed sub-items on
already-shipped features, plus older sourced-content backlog items
unrelated to the SFML work.

## Playtest backlog

Implemented and verified via clean rebuild + launch smoke test, but not
yet fully walked live with a real keyboard. Full sourcing/detail for
each is in its `docs/MILESTONES.md` entry (linked below).

- **Empty-name validation fix** (fixed 2026-09-15, second session, see
  this file's own top section) -- `ansalon_rpg` is already confirmed via
  the piped path (blank Enter is rejected with a message, a real name is
  then accepted). `ansalon_sfml_phase1` only got a launch smoke test:
  still needs a live keyboard to confirm the Name step's inline "Please
  enter a name before continuing." message actually renders and blocks
  advancing on a blank Enter, and that typing a name afterward still
  works normally.
- ~~**Companion sprite art: Bren Alder**~~ (Milestone 192) -- **confirmed
  live 2026-09-12**: his real sprite renders in place of the marker+letter
  during a real fight (4 Giant Centipedes). Confirmed by design, not a
  bug: no left/right mirroring for him (only the player gets the cosmetic
  facing flip) and no attack-pose flash (Milestone 191 wired that to the
  player's own swings only).
- ~~**Combat sprite art: idle/attack pose swap**~~ (Milestone 191) --
  **fully confirmed live 2026-09-12**: fought a Boring Beetle group
  (forest terrain) against a copy of `save1.txt`. The player's token
  renders the real idle sprite in place of the gold circle; companion
  Bren Alder still correctly falls back to the plain marker+letter (no
  art for him yet, no regression); moving to keep an enemy on the
  player's right vs. left correctly mirrored the sprite to face that
  side; and attacking visibly swapped to the attack-pose frame before
  reverting to idle. **One known cosmetic issue, left as-is for now per
  the user**: `Knight.png` has no alpha channel, so a solid mint-green
  rectangle from the source art shows behind the character instead of
  the floor tile underneath -- an art-asset fix (needs real
  transparency), not a rendering bug. Revisit whenever the user wants
  the look cleaned up.
- **Attack/spell/webnet target-picker cancel fix** (fixed 2026-09-12,
  triaged from the live-testing session's own finding, see this file's
  top section) -- still not confirmed: attempted 2026-09-15 (approached
  a Giant Spider pack specifically to test this) but got knocked out
  before ever reaching the picker cancel itself, twice. Open the "Attack
  which enemy?" picker against 2+ eligible targets, press Escape or `q`,
  confirm it lands on Idle (not the quit-confirmation dialog), and that
  `f`/`i`/Space all work immediately afterward.
- **Bigger battlefield + real movement** (Milestone 185) -- **confirmed
  live 2026-09-11**: the scrolling camera following whichever token is
  actually moving, monster/companion AI walking one square at a time
  (animated, not teleporting) with no trail artifacts, and the sidebar's
  `dist N` readout displaying correctly (not clipped). **Also confirmed
  live 2026-09-12** (this session, input-driven -- see that day's
  session note above): taking several `wasd` steps in one round before
  attacking, and the `Movement: N/Max` readout counting down correctly
  (`12/12` -> `9/12` after 3 steps, unchanged by further blocked
  attempts). **Also confirmed live 2026-09-12 (part 2)**: Space
  genuinely holds/ends a turn with no attack -- "You hold your action."
  plus each monster "closes in.", sidebar distances update (with a
  one-action display lag, see this file's own top section). **Still not
  confirmed**: movement actually running out mid-round (refusing a further step with
  "You have no movement left this round"); and monster/companion AI
  closing distance at their own differing rates specifically (a fast
  Wraith/Spectre vs. a slow Zombie/Mummy/Boring Beetle, easiest to force
  by fighting each with a fresh character on open terrain).
- **VIEW command + status tags** (Milestone 186) -- **confirmed live
  2026-09-11**: `v` while Idle opens the "View who?" picker and shows a
  stat card correctly. **Still not separately confirmed**: a Status line
  actually appearing on the card (needs a debuff/buff active -- easiest to
  force by memorizing/casting Slow or Hold Monster and checking both the
  card and the sidebar roster line for the same tag), and Escape/Q
  cancelling the picker itself without consuming a round.
- **Keypad diagonal movement** (Milestone 187) -- **confirmed live
  2026-09-11**: numpad and Home/PageUp/End/PageDown diagonals work, and
  corner-cutting is blocked as expected, across overworld/zone/combat.
- **Battlemap walls + wall-aware pathing** (Milestone 188) -- **partially
  confirmed live 2026-09-12** (this session, input-driven): walking into
  a wall logs "Blocked: cannot walk onto a wall." exactly as documented,
  and wall tiles render in a visibly distinct darker color from open
  floor -- seen across real fights on forest, hills, mountains, and
  grassland battlemaps (4 of the 9 terrains). **Still not confirmed**: a
  diagonal corner-cut against a wall specifically logging "Blocked: can't
  cut across the wall." (no corner-shaped opportunity happened to come up
  -- the wall clusters encountered were all solid rectangles);
  monster/companion AI visibly detouring around a wall cluster (attempted
  via holding several rounds in place, but the monsters' distance didn't
  visibly change -- inconclusive, worth a cleaner retry with a monster
  that has to route around, not just toward, a wall); and the remaining
  5 of 9 terrains (bog, salt flat, savannah, glacier, road).
- **Line of sight** (Milestone 189) -- not yet interactively confirmed
  at all (no desktop/GUI access the session it shipped in): firing the
  Light Crossbow at a monster instance with a wall between it and the
  player logs "Nothing in your line of sight." and is refused; the same
  shot with a clear sightline still works as before; casting a targeted
  spell (e.g. Magic Missile) at a walled-off instance logs "Your
  `<spellName>` finds no target in sight." and still consumes the round/
  spell slot; and Fireball's epicenter picker still only offers
  in-sight instances while splash damage still reaches everyone within
  radius of a confirmed-visible epicenter regardless of walls between
  them and it. Easiest to force on a densely-walled terrain (forest/
  hills/mountains) with the crossbow equipped or a damage spell
  memorized.
- **Multi-square creatures + Blue Dragon** (Milestone 190) -- not yet
  interactively confirmed at all (no desktop/GUI access the session it
  shipped in): an Ogre/Troll renders as a 1-wide/2-tall marker, a Griffon
  as 2-wide/1-tall, and the new Blue Dragon (salt flat terrain, or use a
  debug/dev save near one) as a full 2x2 marker -- not just a single cell;
  the player/companions can walk around a big creature's full footprint
  (not just its anchor cell) without overlapping it; melee/ranged
  targeting and opportunity attacks trigger correctly from any side of the
  footprint, not just its anchor; a wall-blocked shot/spell at a big
  creature still refuses via the nearest visible edge of its footprint,
  not just its anchor cell; the Dragon's 30%-chance lightning breath logs
  "The Blue Dragon breathes a bolt of lightning!" with real damage, and
  its ordinary bite still lands the rest of the time; and an Ogre/Troll/
  Griffon/Dragon's own AI still closes distance and routes around a wall
  cluster reasonably (no wall-aware BFS for these, just the simpler
  greedy heuristic -- an occasional dead-end against a wall corner is
  accepted, see `docs/COMBAT_NOTES.md`).
- **Look command** (Milestone 182) -- **partially confirmed live
  2026-09-12** (this session, input-driven): `'l'` on the open overworld
  with no NPC present correctly falls back to the nearest-location
  compass direction ("You reckon Tarsis lies to the east."). **Also
  confirmed live 2026-09-12 (part 2)**, both halves of the inside-a-zone
  case -- see this file's own top section. **Still not confirmed**:
  `'l'` with an overworld NPC/canon Hero actually present, or a
  `TIMELINE_ANCHOR` tile with one -- every canon Hero was at Pax Tharkas
  today, 50+ overworld tiles from anywhere reachable this session.
- **Quest system** (Milestone 183) -- **further confirmed live
  2026-09-15**: `road_wolves` (Solace's Notice Board) end-to-end through
  offer/accept (real dialogue text, `Accept`/`Decline` picker), the
  journal (`'g'`) rendering real quest state including a live-incrementing
  `SLAY` objective counter (`(0/3)` -> `(1/3)` -> `(2/3)` -> `(3/3)` on
  real kills, persisting across three separate knockouts and one
  unexpected app close), and the objective flipping to checked
  (`[x] Kill three timber wolves...`) with the exact `ReadyToTurnIn`
  one-time log line firing ("...is ready to turn in -- return to the
  Notice Board..."). Also confirmed: `SHOP_LOCKED` actually gates a shop
  (Flint's Smithy refused "There's nothing to buy here yet." while
  `ore_for_the_forge` was unaccepted). **Still not confirmed**: the actual
  turn-in exchange (`COMPLETE` text + reward + shop unlock afterward) --
  see this file's own top section for why (the character ended up ~166
  tiles from Solace when the third kill landed), the progress-text
  revisit, any `DELIVER`-type quest (`ore_for_the_forge` itself, found and
  confirmed locked but never offered/accepted/turned in), and all six
  reward flags (especially the Wayreth Test of High Sorcery's
  ethical-choice scene and its three outcome passages). The disposable
  slot-3 character (Human Fighter, `road_wolves` ready to turn in, sitting
  in Port Balifor) is a ready-made starting point for whoever picks up the
  walk back to Solace.
- **Native character creation** (Milestone 180) -- **further confirmed
  live 2026-09-15**: the full ability-score flow (roll pool, keep/reroll,
  per-ability assignment from the pool); an ineligible race pick's inline
  error and re-prompt (tested against Kender with disqualifying scores --
  greyed out with "(your ability scores don't qualify)" in the list,
  selecting it anyway re-prompts with "Your rolled ability scores don't
  meet Kender's requirements. Choose a different race." rather than
  proceeding); the Knight of the Crown offer screen itself ("You meet the
  qualifications... Swear the oath and join?"); the Weapon Specialization
  prompt; and the final "Begin your journey as this character?" summary
  screen -- confirmed "Yes" lands correctly in Solace with the exact
  stats/HP/AC/THAC0 shown on the summary. **Also found** (not a
  confirmation -- a real gap): the `CreationStep::Name` text-entry step
  silently accepts an empty name with no validation, see this file's own
  top section. **Also confirmed** (second pass, same day): the save-slot
  menu's **Continue** branch on an occupied slot ("Continue this
  character?" -> resumes at the exact saved position/HP/day, twice,
  including once after an unexpected app close -- see this file's own top
  section). **Still open**: the final summary's "No" restart path, the
  Elf/Dwarf subrace step, the Gnome-forced-Tinker path, and the save-slot
  menu's overwrite/delete branches.
- **Stale startup banner fix** (Milestone 181) -- not yet interactively
  confirmed; no desktop/GUI access this session to watch the corrected
  banner render.
- **Combat spellcasting** (Milestone 169) -- still open, hardest to
  force: get monsters to act first (retry until initiative favors them)
  on a round where `M` is pressed, and confirm a knockout that round
  means the spell was never actually cast (still shows as memorized
  afterward).
- **Combat item use** (Milestone 170) -- not separately confirmed: the
  2+-item picker and its Escape/Q cancel, Staff of Curing (no current
  save carries one), the Brooch's once-per-day gate and companion
  exclusion, and a monsters-act-first knockout preventing an item from
  being consumed.
- **Thief backstab** (Milestone 171) -- still not interactively
  confirmed. Fighter sweep half is now confirmed (2026-09-11: Bren Alder
  swept a Giant Rat group, one hit/miss line per instance, no picker, no
  bonus -- see `docs/MILESTONES.md` entry 171). Backstab needs a
  Thief-type party member in light-or-no armor (e.g. Dessa Corrin at
  Haven, once recruited) positioned opposite whoever first attacked an
  instance; confirm the log reads "Backstab! " with a visibly larger
  damage number, and that there's no bonus from any other square or in
  heavier armor.
- **Same-cell combat-movement collision fix** (Milestone 172, both
  builds) -- not yet re-confirmed live. Worth deliberately re-triggering:
  retreat from an adjacent monster on a round where it can close the
  distance, confirm no overlap.
- **Dialogue** (Milestone 161) -- the quest placeholder log line at a POI
  marked `QUEST` (e.g. Kalaman's Curiosities Cart) not yet separately
  confirmed.
- **Ask-input** (Milestone 162) -- **confirmed live 2026-09-12 (part
  2)**: real keyword matching, Backspace editing, and empty-Enter cancel
  -- see this file's own top section. **Still not confirmed**:
  `ASK_LIMIT_LOCKED`'s greeting override on a same-day return visit to
  Astinus, and the extension-roll *fail* path (pure chance which branch
  fires live) -- both need Astinus specifically.
- **Inventory** (Milestone 165) -- **confirmed live 2026-09-12 (part
  2)**: equipping armor and a shield both updated the header line
  correctly (see this file's own top section). **Still not confirmed**:
  the Webnet/Brooch of Imog/quest-item no-op message (no quest item
  carried).
- ~~**152**~~ -- Fireball/Delayed Blast Fireball's multi-target and
  solo/isolated-target cases are confirmed 2026-09-09 via the SFML
  build. Still open: walking this through the *console* (`ansalon_rpg`)
  build's own `pickTarget` epicenter-picking UI specifically, which
  hasn't been exercised at all (same standing `_getch()` limitation) --
  low priority given the underlying area-damage math itself is now
  proven.
- **137** -- day-gated Astinus dialogue, fixing 12 shipped spoilers. Talk
  to Astinus in Palanthas before day 2/3/12/17 and again after day 160;
  confirm both halves read correctly.
- **138/139** -- 6 Astinus SUBJECT topics (Kagonesti, gnomes, gully
  dwarves, minotaurs, ogres/Irda, Reorx). Ask Astinus about each.
- **142** -- Harpy/Griffon/Stirge (Monster Manual). Trigger wilderness
  encounters on hills/mountains (Griffon), grassland/hills (Harpy),
  forest (Stirge).
- **143** -- `a_widows_due` DELIVER quest at Kalaman. Talk to the
  Curiosities Cart, find the Furtive Trader POI, deliver the wedding
  band. Actually testable for the first time as of Milestone 183 -- it
  depended on the quest system existing at all.
- **145** -- 3 Astinus SUBJECT topics (Fizban, Silvara, Berem/the
  Everman), each day-gated. Ask before/after day 192/69/193
  respectively.
- **149** -- 3 Astinus SUBJECT topics (Gilthanas, Lord Soth, Ariakas),
  each day-gated. Ask before/after day 69/190/193 respectively.
- **151** -- 2 more Astinus SUBJECT topics (Alhana Starbreeze,
  Porthios), each day-gated. Ask before/after day 51/70 respectively.
- **False "ocean" pockets inside Qualinesti/Silvanesti forest** -- fixed
  (see `docs/MAP_NOTES.md`'s "Fixing false 'ocean' pockets inside
  forest"), not yet walked live. Worth deliberately walking north and
  west from Regan's saved position (`save2.txt`, `POS 176 214`), and
  generally through Qualinesti near Bianost/Dark Tower, to confirm no
  more spurious "Blocked: cannot walk onto the ocean." Data-only change
  -- applies to `ansalon_rpg` too, not just the SFML build.

## Parked: Dragonlance Adventure modules as full quests

**Raised 2026-09-11, backburnered before any planning/code.** The user
asked about incorporating the classic DL adventure modules (`References/`
already has DL1 *Dragons of Despair*, DL2 *Dragons of Flame*, DL3
*Dragons of Hope*) as full in-game adventures/quests, not just sourcing
flavor text.

Research done before parking, so a future session doesn't have to
re-derive it:

- **Real design tension**: `docs/QUEST_NOTES.md` records that canon
  Heroes are deliberately never quest-givers -- they're "weather," and
  the player character isn't one of them. The DL modules' text is
  written to literally *be* played as the Heroes' own party (built-in
  pregens, Goldmoon/Riverwind joining mid-module) -- a faithful port
  would mean either breaking that rule or having the player reenact
  Tanis's party's own documented plot beats. Not resolved -- the user
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
- **DL1's actual dungeon is much bigger than that stub** -- "Lost City of
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

Don't resume this unprompted -- ask which framing (echo vs. literal) and
which location to start with, same as any other new milestone.

## Parked: SFML rendering + variant tile art (round 2, `sfml-trial-2`)

**Superseded in direction, not necessarily obsolete.** This is round 2's
tile-atlas approach (hand-drawn 32px terrain tiles); the active
migration above instead renders the real map image directly,
pixel-space. Still worth keeping this note around in case tile-based art
becomes relevant again for a screen the real map can't represent (a zone
interior, say) -- just don't assume it's the plan for the active
migration above.

Not abandoned, not committed to -- purely the user's call whenever (or
if) they revisit it. Don't propose pushing this further unprompted; this
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
same-terrain regions -- bigger now after Milestone 146's smoothing --
stop reading as repeating wallpaper.

To resume:

- SFML trial code: `git checkout sfml-trial-2 && git stash pop`
  (uncommitted `CMakeLists.txt` target `ansalon_sfml_trial` +
  `sfml_trial/main.cpp`).
- Variant art already exists and is proven, just not wired in:
  `References/variant_tiles6.png` (forest/grassland/hills) and
  `References/variant_tiles_extra.png`
  (mountains/savannah/bog/salt-flat/glacier), each a 6-column strip; the
  position-hash selection logic (`(x*73856093) ^ (y*19349663)`) is
  proven in the mockup script. Porting both into
  `terrain_tileset.json`'s schema (single rect per code -> list of
  rects) and `sfml_trial/main.cpp`'s tile lookup is the concrete next
  step, not a re-derivation.
