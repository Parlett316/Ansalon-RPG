# Claude Code Workflow Cheat Sheet

How to work on this project without burning tokens or dragging around
giant conversation history. The short version: **the chat box is
disposable, the `.md` files are permanent.** Memory lives in docs, not
in hours-long conversations.

---

## Doc Map

| File | Purpose |
|---|---|
| `CLAUDE.md` | Entry point. Routes Claude to the right doc for the task at hand. |
| `docs/CURRENT_WORK.md` | What's in flight right now. Session-to-session handoff. |
| `docs/MILESTONES.md` | Shipped history (numbered) + "Next Up" backlog. |
| `docs/ARCHITECTURE.md` | Technical structure / how the project is built. |
| `docs/GOTCHAS.md` | Known pitfalls, past mistakes, things not to repeat. |
| `docs/COMBAT_NOTES.md` | Combat system specifics. |
| `docs/CHARACTER_NOTES.md` | Character/NPC design. |
| `docs/TIMELINE_NOTES.md` | Canon timeline, chance-encounter schedule. |
| `docs/ZONE_NOTES.md` | Zone/region design. |
| `docs/MAP_NOTES.md` | Map/overworld specifics. |

---

## Daily Workflow

**1. Start of session — cheap rehydration**

```
Read CLAUDE.md and docs/CURRENT_WORK.md. What's next?
```

or, for milestone-specific work:

```

```

**2. Work the task**

Let Claude route itself to the relevant notes file via `CLAUDE.md`.
Point it at specific files when you can:

```
Review CombatSystem.cpp and docs/COMBAT_NOTES.md. Fix X.
```

instead of:

```
Look into the combat system and fix X.
```

**3. Before you stop or switch topics — save state**

```
Update docs/CURRENT_WORK.md with what we did and what's next.
```

If a milestone finished:

```
Mark Milestone N complete in docs/MILESTONES.md, move the next item
up from "Next Up," and update CURRENT_WORK.md.
```

**4. Clear the slate**

```
/compact
```
Use when you might still want to reference *this* conversation later
(e.g., double-checking a decision you just made).

```
/clear
```
Use when you're done with the topic entirely, or ending for the day.

---

## Golden Rules

- **No 8+ hour sessions.** Long sessions are what caused the original
  150k+ token context problem. Break work into chunks, save state,
  clear, repeat.
- **Files are memory. Chat is scratch space.** If it matters, it goes
  in a `.md` file before you clear.
- **Point Claude at specific files** instead of "the whole project"
  whenever possible. Scans of every doc/notes file are expensive —
  only worth it for one-time consolidation tasks (like building
  `MILESTONES.md` the first time).
- **Verify against source material, don't assume.** Before treating a
  lore/design assumption as fact, have Claude check the actual source
  text (book PDF, existing code, existing docs) rather than reasoning
  from summary or memory.
- **Sanity-check expensive outputs while context is fresh.** If Claude
  just did a big multi-file reconstruction (like the milestone
  history), review it *before* compacting/clearing — fixing it later
  means paying for the same expensive scan twice.
- **Update docs immediately after finishing work**, not "later." A doc
  that's out of date is worse than no doc, because future sessions
  will trust it.

---

## The Loop

```
Read → Work → Update docs → Clear
```

That's the whole system. Everything else is detail.
