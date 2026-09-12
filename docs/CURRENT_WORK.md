# Current work

**Nothing in-flight code-wise.** Playable v7 is done: P1 (quest system +
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

- **Look command** (Milestone 182) -- not yet interactively confirmed at
  all: `'l'` on the overworld (with an NPC present, and the nearest-
  location/compass-direction fallback with none present) and inside a
  zone (a POI's own description, and a TIMELINE_ANCHOR tile with a canon
  character present).
- **Quest system** (Milestone 183) -- not yet interactively confirmed at
  all: offering/accepting/declining a quest, the progress-text revisit,
  turning one in, all six reward flags (especially the Wayreth Test of
  High Sorcery's ethical-choice scene and its three outcome passages),
  the journal (`'g'`) rendering real quest state, and a `SHOP_LOCKED`
  shop (e.g. Flint's Smithy) actually gating on quest completion. Easiest
  real quest to walk end-to-end first: `road_wolves` (Solace's Notice
  Board, a single `SLAY wolf 3` objective, no `REQUIRE`).
- **Native character creation** (Milestone 180) -- partially confirmed
  2026-09-11: reached the Knight-of-Crown offer screen on a fresh slot.
  Still open: the Knight Offer screen itself, the final summary screen
  (does "Yes" land you in Solace with correct stats, does "No" restart
  from Name), an ineligible race/class/alignment pick's inline error and
  re-prompt, the Elf/Dwarf subrace step, the Gnome-forced-Tinker path,
  and the save-slot menu's own Continue/overwrite/delete branches.
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
- **Ask-input** (Milestone 162) -- not yet confirmed: matching a real
  keyword (only gibberish tried so far), Backspace editing the buffer
  and empty-Enter cancelling back to the topic picker,
  `ASK_LIMIT_LOCKED`'s greeting override on a same-day return visit to
  Astinus, the extension-roll *fail* path (pure chance which branch
  fires live), and a plain unlimited-`SUBJECT` NPC/Hero (only Astinus's
  limited pool has been tried).
- **Inventory** (Milestone 165) -- not yet confirmed: equipping a
  weapon/armor/shield (header line updating to match) and the
  Webnet/Brooch of Imog/quest-item no-op message.
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
