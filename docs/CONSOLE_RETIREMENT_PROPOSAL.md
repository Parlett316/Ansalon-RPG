# Proposal: Retirement trigger for the legacy console build (`ansalon_rpg`)

> Decision brief for the user (Michael). **This is a proposal, not an action.**
> Nothing is retired until a trigger below is explicitly approved. Recorded
> here so future sessions have a clear decision point instead of maintaining
> two live targets indefinitely by default.

---

## The problem

`ansalon_rpg` (ASCII console) is kept as a non-retired legacy/reference build
alongside the primary `ansalon_sfml_phase1` (SFML) build. Both compile side by
side today. That's a deliberate, reasonable choice for the migration period —
but "keep it around indefinitely" is a decision by default, not on purpose, and
it carries an ongoing cost:

- **Doubled maintenance.** Every systems change risks divergence between the
  two targets, and parity has to be tracked by hand (see the parity matrix).
- **Doubled verification.** Console changes verify via piped character
  creation; SFML changes verify via launch smoke test. Every cross-cutting
  change means two verification paths.
- **`/W4`-clean across both.** The zero-new-warnings bar applies to both
  targets, so even a change that only matters to one still has to be proven
  clean on the other.
- **Explicit CMake source lists** must be maintained for both targets.

The console build still has real value **right now** (it's the only build with
full quest tracking, and it's the fallback if an SFML save goes wrong), so
retiring it today would be premature. The question is *when* it stops earning
its keep.

---

## Recommended trigger (primary recommendation)

**Retire `ansalon_rpg` once `ansalon_sfml_phase1` reaches full parity on the
two systems where the console build is currently ahead — quests and saves —
and that parity has been confirmed live at the keyboard, not just smoke-tested.**

Concretely, all of the following must be true:

1. **Quest parity (P1 done):** SFML tracks quest state fully — offer, accept/
   decline, progress, "ready to turn in" detection, turn-in + reward, and the
   quest-locked shop unlock — with every quest path confirmed off the Playtest
   backlog, not flagged "not yet interactively confirmed."
2. **Save parity + hardening (P2 done):** SFML saves are atomic
   (temp-write-then-rename), backed up on write, versioned, and covered by a
   passing round-trip self-test — and the user has run at least one multi-
   session playthrough without a save issue.
3. **No remaining SFML `Placeholder` or `Not yet` rows** in the parity matrix
   that represent lost functionality. (The `Look` command is the one open
   `Not yet`; either implement it or the user explicitly agrees it's droppable.)
4. **The user has played a full SFML session and is satisfied** it's the build
   they'd hand to someone else without reaching for the console fallback.

Rationale: this ties retirement to the exact two things the console build is
still *better* at, so it can't be retired while it's still doing something the
primary build can't.

---

## What "retire" should mean (staged, reversible)

Retirement doesn't have to be deletion. Recommended stages, each independently
approvable:

- **Stage 0 — today:** Both targets live and fully maintained. (Current state.)
- **Stage 1 — Deprecate:** Mark `ansalon_rpg` deprecated in the README and
  `CLAUDE.md`. New features land on SFML only; console gets bug-fix parity only
  if trivial. Still builds, still `/W4`-clean. **Reversible.**
- **Stage 2 — Freeze:** Stop holding console to feature/verification parity.
  It still builds but is explicitly "reference, unmaintained." Drop it from the
  routine per-change verification loop. **Reversible.**
- **Stage 3 — Remove from default build:** Take `ansalon_rpg` out of the
  default CMake target set (still buildable via an opt-in flag / tag). Cuts the
  doubled-warning and doubled-verification cost. **Reversible via git.**
- **Stage 4 — Archive:** Tag the last console-inclusive commit
  (e.g. `console-final`), then remove the target and its console-only sources
  from the tree. `docs/ARCHITECTURE.md` keeps a short "why it existed / how to
  get it back" note. **Reversible via the tag.**

The recommended trigger above gates **Stage 1 (Deprecate)**. Each later stage
should get its own explicit go-ahead rather than cascading automatically.

---

## Alternatives considered

- **Retire now:** Rejected — console is still the only build with full quests
  and the save fallback. Premature.
- **Keep both forever:** Rejected as a *default* — it's fine as an explicit
  choice, but it should be chosen on purpose, not backed into. The doubled
  maintenance/verification cost compounds every milestone.
- **Trigger on a date instead of on parity:** Rejected — a calendar date
  doesn't know whether the SFML build is actually ready. Parity-based is safer.
- **Trigger on a version number (e.g. "at v8"):** Weaker than parity-based for
  the same reason, but acceptable as a coarse proxy if the user prefers a
  simple milestone marker over a checklist.

---

## Decision requested

Please pick one (AskUserQuestion-style):

- **A — Adopt the recommended parity trigger** gating Stage 1 (Deprecate), with
  each later stage requiring its own approval. *(Recommended.)*
- **B — Adopt a simpler version trigger** (e.g. "deprecate console at Playable
  v8") instead of the parity checklist.
- **C — Keep both builds fully live for now**, revisit after v7 ships.
- **D — Something else** (specify the trigger you'd prefer).

Once chosen, record the decision in `docs/ARCHITECTURE.md` and note the current
stage in `docs/CURRENT_WORK.md` so no future session re-litigates it.
