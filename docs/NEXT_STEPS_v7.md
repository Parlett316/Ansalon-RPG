# Next Steps — Ansalon: Age of Despair (toward Playable v7)

> Hand-off brief for a Claude Code session. Read `docs/CURRENT_WORK.md`,
> `docs/ARCHITECTURE.md`, and `docs/GOTCHAS.md` before touching anything
> outside a single well-contained file. Follow the established loop:
> **Research → Plan (EnterPlanMode) → Implement → throwaway self-test →
> clean /W4 rebuild → verify → update docs → update CURRENT_WORK.md → hand off.**
> Don't start the next milestone without being asked — offer a short
> AskUserQuestion menu instead.

---

## Guiding principle for v7

The SFML build (`ansalon_sfml_phase1`) is now the **primary** build, but it
is not yet at parity with the legacy console build. **v7's job is to close
the biggest capability gaps on the primary build and harden the newest,
highest-risk system (saves) — not to add new content.** Hold the line on
"restraint over completeness": no new zones, monsters, or timeline content
this milestone unless a task below explicitly needs it.

---

## ~~Priority 1 — Port quest tracking to the SFML build~~ *(done — Milestone 183)*

**Closed 2026-09-11.** The Look command (a separate real gap this doc didn't
originally list, found by the same audit) was ported too — see Milestone 182.
Both are **not yet interactively confirmed live**; see
`docs/CURRENT_WORK.md`'s Playtest backlog. Kept below for the historical scope
record.

## Priority 1 — Port quest tracking to the SFML build  *(biggest parity gap)*

**Why:** The primary build doesn't track quest state at all. The `G` journal
says so plainly, quest-giver dialogue logs a placeholder instead of offering
a quest, and one quest-locked shop shows a placeholder instead of resolving
the lock. Quests work fully in the console build — this is the single largest
thing between "primary build" and "feature-complete."

**Scope:**
- Wire the existing console quest state machine (offer → accept/decline →
  progress tracking → "ready to turn in" detection → turn-in + reward) into
  the SFML `T` dialogue and `G` journal flow.
- Resolve the quest-locked shop placeholder (Flint's Smithy `ore_for_the_forge`
  lock) so it unlocks on quest completion in SFML, same as console.
- Reuse the console quest grammar and loaders as-is — **do not** fork a second
  quest format. If the state lives in `GameState`, confirm it serializes
  through `SaveGame` (see Priority 2).

**Research first:** re-read `docs/QUEST_NOTES.md` (grammar, objectives,
journal, turn-in flow) and confirm what the console path already computes so
the SFML path calls the same logic rather than reimplementing it.

**Verify:** launch smoke test that catalogs load; then flag every
quest-interaction path (offer render, accept, progress line, turn-in,
shop-unlock) as **"not yet interactively confirmed"** in
`docs/CURRENT_WORK.md`'s Playtest backlog for a live keyboard check — never
claim a screen works because it compiled and smoke-tested clean.

**Docs to update:** `docs/QUEST_NOTES.md`, README Status (drop the "SFML
doesn't track quest state yet" caveat once confirmed), `docs/CURRENT_WORK.md`.

---

## ~~Priority 2 — Harden the new SFML save system~~ *(done — Milestone 184)*

**Closed 2026-09-11**, done deliberately *before* P1 per this doc's own
suggested order below. Atomic write-then-rename, rotated backups, and a
`VERSION` header all shipped, verified by a 58-check throwaway self-test.
Deliberately not attempted: platform-specific `fsync`/`FlushFileBuffers`
hard-power-loss durability (out of scope, see `docs/GOTCHAS.md`). Kept below
for the historical scope record.

## Priority 2 — Harden the new SFML save system  *(highest-severity risk)*

**Why:** SFML autosave-after-every-action is newly added and explicitly "not
yet extensively playtested." Save corruption is the worst bug class for a
persistent RPG — a bad autosave can clobber an in-progress character with no
recourse.

**Scope:**
- **Atomic writes:** write autosave to a temp file, `fsync`/flush, then
  rename over the real slot, so an interrupted write can never leave a
  half-written save.
- **Backup-on-write:** before overwriting, copy the previous good save to a
  timestamped `.bak` (keep the last N). Cheap insurance for the first few
  sessions the user is watching their save.
- **Versioned header:** add/confirm a save-format version field so future
  format changes can detect-and-migrate (or refuse cleanly) rather than
  misparsing.
- **Checked-in round-trip self-test:** the one permanent exception worth
  making to the "no test suite" rule, or at minimum a documented throwaway:
  create a character → mutate state (win a fight, level, equip, buy) →
  serialize → reload → assert deep equality. Pure logic, no window/keypress
  needed, so it sidesteps the headless-verification blind spot entirely.

**Guardrail (from CLAUDE.md):** if you test against real `save1/2/3.txt`,
move them aside first, run, then move them back — never leave the user's
real save clobbered or missing.

**Docs to update:** `docs/ARCHITECTURE.md` (SaveGame section), `docs/GOTCHAS.md`
(record the atomic-write/backup trap), README Status (soften the "keep an eye
on your save" caveat once hardened), `docs/CURRENT_WORK.md`.

---

## ~~Priority 3 — Add a build/feature parity matrix to the README~~ *(done — `docs/PARITY_MATRIX.md`)*

**Closed 2026-09-11.** Built as a standalone doc rather than inline in the
README (kept the README shorter); its Quests/Look/Saves rows have been
refreshed to match Milestones 182-184. Kept below for the historical scope
record.

## Priority 3 — Add a build/feature parity matrix to the README

**Why:** Parity state is currently scattered through prose ("sea travel now
works in both," "SFML doesn't track quests yet," "companion recruitment
confirmed in SFML"). A single table is the source of truth and prevents
silent divergence between the two live targets.

**Scope:** one table, rows = systems, columns = Console vs SFML, cells =
`Done` / `Partial` / `Placeholder` / `N/A`. Suggested rows: character
creation, saves, overworld/movement, zone interiors, talk/dialogue, chance
encounters, sea travel, companions, combat, magic/items, shops/equipment,
leveling, quests, world map, look. Replace the scattered inline parity notes
with a pointer to this table.

**Docs to update:** README, `docs/CURRENT_WORK.md`.

---

## Priority 4 — Extract combat/dice math into checked-in unit tests

**Why:** These are pure functions (dice distributions, THAC0/AC resolution,
damage-die breakdown, initiative) with no window or keypress dependency, so
they dodge the `_getch()`/SFML headless-verification problem completely — and
they're your most rules-sensitive, regression-prone code.

**Scope:**
- Small deterministic tests: seeded RNG → assert 4d6-drop-lowest bounds and
  distribution sanity; assert THAC0 math against known PHB values; assert
  damage-die ranges; assert weapon-specialization and backstab/sweep
  modifiers apply exactly once.
- Decide with the user whether these are **checked-in** (a deliberate,
  documented exception to "no permanent suite") or the throwaway pattern run
  each milestone. Recommend checked-in for math specifically.

**Docs to update:** `docs/COMBAT_NOTES.md`, `docs/ARCHITECTURE.md` (note the
testing exception if adopted), `docs/CURRENT_WORK.md`.

---

## Priority 5 — Validate the portability claim (lower urgency)

**Why:** The code claims "no source changes needed" for a future GCC/Clang
build, but only the Windows `<conio.h>` keypress path exists and nothing has
compiled off-MSVC. The claim is aspirational until proven.

**Scope (spike only — get explicit approval before doing real work here):**
- Attempt a GCC/Clang compile of the **game logic** with the Windows input
  layer stubbed, to validate `CMAKE_CXX_EXTENSIONS OFF` discipline holds.
- Document what breaks (if anything) in `docs/GOTCHAS.md`. Do **not** attempt
  a full cross-platform input abstraction this milestone — that's premature
  abstraction until a real non-Windows build is actually wanted.

---

## Explicitly out of scope for v7

- New zones, monsters, timeline stops, or dialogue content.
- Retiring the legacy console build. (Instead: **propose a retirement
  trigger** to the user — e.g., "retire `ansalon_rpg` once SFML reaches quest
  + save parity" — and let them decide. Don't act on it.)
- Companion combat orders, the `Look` command in SFML, or any new subsystem
  not required by the tasks above.

---

## Suggested session order

1. **P2 (save hardening) first** — do it *before* P1, so the quest state you
   add in P1 serializes onto an already-atomic, already-backed-up save path
   rather than a fragile one.
2. **P1 (SFML quests)** — the headline parity win, now landing on safe saves.
3. **P3 (parity matrix)** — quick, and P1 will have changed a row anyway.
4. **P4 (combat/dice tests)** — protects the rules math going forward.
5. **P5 (portability spike)** — only if the user green-lights it.

Offer this as an AskUserQuestion menu rather than assuming — the user may want
to reorder, and P5 in particular needs explicit approval.
