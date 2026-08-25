# Milestone 72 spec — character-level subject pools and day-gated knowledge

Hand this to Claude Code as the task brief. It assumes the repo state as of
Milestone 71 ("Ask about anything", `SUBJECT`/`SUBJECT_UNKNOWN` shipped on
exactly one window: Raistlin's `PRESENCE solace 0 1`).

---

## 1. Goal

Let a canon character answer many free-text subjects, with answers that are
correct **for the day the player meets them**.

Two things stand in the way today:

1. **Duplication.** `SUBJECT` attaches to a `PRESENCE` window. Raistlin has
   eight talkable windows (`solace 0 1`, `haven 2 3`, `darken_wood 2 3`,
   `xak_tsaroth 4 6`, `qualinesti 7 9`, `pax_tharkas 10 12`, `tarsis 20 22`,
   `silvanesti 25 30`; `palanthas 83 83` is deliberately mute). His hourglass
   eyes are true at all eight. 25 subjects × 8 windows is 200 lines to keep in
   sync, and the day-0 copy will drift from the day-25 copy within two passes.
2. **No way to say "he learns this on day N."** Cyan Bloodbane must be
   unanswerable at Solace and answerable at Silvanesti, without hand-copying
   the subject into every window past day 25.

**What this milestone does not change:** the free-text prompt, tokenizer,
matcher, picker row, `Console::readLine`, or any zone-file grammar. This is a
loader + one pure Timeline query + a content pass.

---

## 2. Grammar change (`data/timeline.txt`)

### 2.1 Character-level subjects

`SUBJECT` and `SUBJECT_UNKNOWN` become legal **before the first `PRESENCE`
line in a `CHARACTER` block**, where they attach to the character rather than
to a window — same positional convention `NAME` already uses. Their existing
meaning inside a window is unchanged.

### 2.2 One new keyword

```
SUBJECT_WHEN <day-start> <day-end> <keywords> <text>
```

- Character-level only. **Error if it appears after a `PRESENCE` line** — a
  window is already day-scoped, so a day range inside one is ambiguous by
  construction. Fail fast with the usual `file:line` message.
- `<day-start>` is an integer >= 0. `<day-end>` is an integer >= `<day-start>`,
  **or `-1` meaning open-ended** — the same `-1` sentinel `latestDayEnd`/
  `earliestDayStart` already use for "no such day."
- Range is inclusive, matching `PRESENCE`'s own `<day-start> <day-end>`.
- `<keywords>` and `<text>` are parsed exactly as the existing `SUBJECT` line
  parses them (one whitespace-free comma-separated token, then rest-of-line).
- Plain character-level `SUBJECT` is exactly `SUBJECT_WHEN 0 -1`. Keep both;
  `SUBJECT` is the common case and shouldn't carry ceremony.

Updated grammar block for `docs/TIMELINE_NOTES.md`:

```
SUBJECT <keywords> <text>       optional, zero or more -- a free-text-askable
                                subject. Inside a PRESENCE window: scoped to
                                that window. Before the first PRESENCE:
                                character-level, available at every window
                                (equivalent to SUBJECT_WHEN 0 -1)
SUBJECT_WHEN <d0> <d1>          optional, zero or more, character-level only
  <keywords> <text>             (before the first PRESENCE; an error after
                                one) -- a character-level subject available
                                only on days d0..d1 inclusive. d1 of -1 means
                                open-ended. Two entries sharing a keyword list
                                with adjacent ranges is how an answer changes
                                over time
SUBJECT_UNKNOWN <text>          optional -- shown when a typed subject matches
                                nothing. Legal inside a window (applies to
                                that window) or before the first PRESENCE
                                (applies to every window that lacks its own)
```

### 2.3 Worked example

```
CHARACTER raistlin
NAME Raistlin Majere

SUBJECT eyes,hourglass,pupils    <what the Test did to his sight>
SUBJECT skin,gold,golden         <the same, and being stared at>
SUBJECT staff,magius,shirak      <Par-Salian's gift>

SUBJECT_WHEN 0 3    gods,mishakal,faith   <the gods are gone; a fool's hope>
SUBJECT_WHEN 4 -1   gods,mishakal,faith   <after the Disks: grudging, unsettled>
SUBJECT_WHEN 0 24   cyan,bloodbane        <in-voice ignorance>
SUBJECT_WHEN 25 -1  cyan,bloodbane        <the real answer>

SUBJECT_UNKNOWN <his catch-all dismissal>

PRESENCE solace 0 1
SAY ...
SUBJECT kitiara,kit,sister       <the letter-scene answer, Solace-specific>
END
```

---

## 3. Resolution semantics

When the player asks a window a free-text question on day `D`, the candidate
list is, in order:

1. That window's own `SUBJECT` entries, in authored order.
2. The character's `SUBJECT`/`SUBJECT_WHEN` entries whose day range contains
   `D`, in authored order.

`matchSubject` runs over that concatenated list unchanged — **first match
wins**, same convention as `SAY_IF`. Two consequences worth writing down in
the docs because they're load-bearing:

- A window `SUBJECT` sharing a keyword with a pool entry **silently wins**.
  That is the override mechanism, not a bug. Use it where a stop deserves a
  specific voice (Raistlin on Kitiara at the Inn) and let the pool cover the
  rest.
- Two `SUBJECT_WHEN` rows sharing a keyword list with adjacent, non-overlapping
  ranges give the evolving-answer behavior for free. No new concept needed.

`SUBJECT_UNKNOWN` resolution: window's own, else the character's, else the
existing hardcoded engine line.

**`D` is the current in-game day** (`state.hoursElapsed / 24`) — the same value
`presentAt` is already called with, not the window's `dayStart`. A player who
meets Raistlin on day 25 of a 25–30 window gets the day-25 answer; one who
arrives day 28 gets the day-28 answer. No extra plumbing, and it's the more
truthful reading of "when did they meet him."

---

## 4. Code changes

### 4.1 `src/timeline/` — data model

Add to `timeline::CanonCharacter`:

- an ordered list of character-level subjects, each `{ keywords, text,
  dayStart, dayEnd }` (reuse whatever struct the existing window-level subject
  already uses, plus the two day fields);
- an optional character-level `subjectUnknown` string.

`PresenceWindow` is untouched.

### 4.2 `src/timeline/Timeline` — two pure queries

Same shape and idiom as the existing `latestDayEnd`/`earliestDayStart` pure
queries:

```cpp
// Window subjects first (authored order), then character-level subjects
// whose day range contains `day` (authored order).
std::vector<Subject> subjectsFor(const CanonCharacter&,
                                 const PresenceWindow&,
                                 int day) const;

// Window's own, else the character's, else empty.
std::string subjectUnknownFor(const CanonCharacter&,
                              const PresenceWindow&) const;
```

Returning by value is fine — these lists are tiny and this runs once per talk,
not per frame. Keep the return type a `timeline::` type: **`timeline/` must not
learn about `game::Speech`**, same module independence `docs/ARCHITECTURE.md`'s
module map already documents.

### 4.3 `src/game/GameLoop` — adapter only

Wherever `handleTalk` builds a `game::Speech` from a `Timeline::Presence`
today (there are two timeline paths — the overworld tile and the
`TIMELINE_ANCHOR` tile; if they don't already share one helper, make them),
populate `Speech::subjects` from `subjectsFor(...)` and `Speech::subjectUnknown`
from `subjectUnknownFor(...)` instead of reading the window directly.

**`GameLoop::talkTo`, `TalkCandidate`, `pickAndTalk`, `tokenizeAskInput`,
`matchSubject`, `drawAskInputFrame`, and `Console::readLine` do not change at
all.** `talkTo` stays fully source-agnostic — same "the executor was always
source-agnostic; what was missing was purely loader parsing" precedent
Milestones 26 and 71 both followed.

### 4.4 `TimelineLoader` — parsing and validation

- Parse `SUBJECT`/`SUBJECT_UNKNOWN` before the first `PRESENCE` into the
  character-level lists; after a `PRESENCE`, behavior is unchanged.
- Parse `SUBJECT_WHEN`; **error** (fail fast, `file:line`) if it appears after
  a `PRESENCE`, if either day fails to parse as an integer, if `dayStart < 0`,
  or if `dayEnd != -1 && dayEnd < dayStart`.
- Error on a second character-level `SUBJECT_UNKNOWN` in one block (at most one,
  same rule the window-level line already has).

### 4.5 Keyword-collision reporting (new, and worth the small effort)

At three subjects, ordering is invisible. At twenty-five it is load-bearing and
fragile: a player typing "dragon orb" produces the tokens `dragon` and `orb`,
and `dragon` alone will be reachable from dragon orbs, Khisanth, Cyan
Bloodbane, and draconians.

After loading, walk each character's merged pool and report any keyword
reachable from two entries whose day ranges overlap (counting a window subject
as overlapping its window's own days). Report it as a **warning to stderr, not
a load failure** — a deliberate override is exactly this pattern, so failing
hard would forbid the intended use. Keep the message in the existing
`file:line`-style format so it reads like the rest of the loader's output.

Authoring rule to document alongside it: **order specific before general.**
`orb`/`orbs` before any entry claiming `dragon`.

### 4.6 One tokenizer question to settle first

The shipped Solace keyword list contains `half-sister`. Before authoring
another twenty-five lines on top of that assumption, confirm in the throwaway
self-test what `tokenizeAskInput` does with a hyphen:

- if punctuation is stripped to nothing, `half-sister` becomes `halfsister` and
  a player typing "half sister" never matches;
- if it splits, a bare `half` token exists and can collide later.

Either behavior is fine. Pick one, assert it in the self-test, and write it
down in `docs/GOTCHAS.md` — don't leave it undetermined.

---

## 5. Content pass — Raistlin only

Scope this milestone's content to Raistlin across all eight of his talkable
windows. Every other Hero, and every zone NPC, stays as-is — same "prove it out
narrow, widen later" restraint `SAY_IF`/`TOPIC` (M19), `TALK_AFTER` (M66),
`TALK_BEFORE` (M68), and `SUBJECT` itself (M71) each followed. Widening to the
other seven Heroes is Milestone 73.

### 5.1 Migrate what's already shipped

The three Solace `SUBJECT` entries (`kitiara,kit,sister,half-sister`;
`caramon,brother`; `magic,test,towers,sorcery,tower`) and the Solace
`SUBJECT_UNKNOWN`:

- Promote `caramon,brother`, `magic,...`, and `SUBJECT_UNKNOWN` to
  character-level unchanged.
- **Keep the Kitiara entry at the Solace window**, where its Inn-letter-scene
  voice belongs, and add a character-level Kitiara entry for the other seven
  windows. Whether the later one should itself split into two
  `SUBJECT_WHEN` ranges (before/after the party learns what she's become) is a
  sourcing question — see below, don't guess it.

### 5.2 Always-true set — safe to author

These predate day 0 in every source and need no gate. Plain character-level
`SUBJECT`:

- his hourglass eyes / what the Test did to his sight
- his golden skin
- the Test itself, Wayreth, the Conclave
- the Staff of Magius
- Par-Salian
- the three Orders and the Robes; his own neutrality
- the three moons (Solinari, Lunitari, Nuitari) and which is his
- his health, the cough, the blood
- his mother and father (Legends is in the source library)
- one entry each for the seven companions — Tanis, Sturm, Flint, Tasslehoff,
  Caramon, Goldmoon, Riverwind. His opinions of them are cheap, characterful,
  and always available. This is the single highest-value block in the roster.

### 5.3 Gated set — every day-gate needs verification before it's authored

Candidates: Fistandantilus, Khisanth, Bupu, the Disks of Mishakal, draconians,
Verminaard, Laurana, Lorac, Cyan Bloodbane, Alhana Starbreeze, dragon orbs,
Astinus, Takhisis, the true gods (the two-stage pair above), Fizban / the old
man.

**Do not fill these in from memory.** For each one, run the project's usual
`pdftotext`/`.research/*_full.txt` pass and answer one question:

> On what day does *this game's already-modeled timeline* first put Raistlin in
> contact with this subject?

That is not always the same day canon does, and where they differ, **this
file's own established facts win**. The sharp case is Fistandantilus: Raistlin
canonically knows the name from the Test, but Milestone 20's Xak Tsaroth
content establishes the vault as the in-game first naming, so gate at day 4
even though it's arguably late — and say so in the docs. Expect the same
tension on Takhisis and on Kitiara's Highlord rank.

**Drop any candidate you can't ground.** Same call Riverwind's missing Solace
`SAY_IF` and the Haven scope cut each made.

### 5.4 Three kinds of "no answer"

Milestone 71's note that `SUBJECT_UNKNOWN` doubles as the forward-reference
answer is true, but it gets weak at this breadth. Author deliberately:

- **Not his subject at all** → falls through to `SUBJECT_UNKNOWN`. Unchanged.
- **His subject, but he doesn't know it yet** → an authored early-range
  `SUBJECT_WHEN`. "Cyan Bloodbane" getting a generic brush-off wastes the beat;
  him snapping that you invented that name, or read it somewhere you had no
  business reading, *is* the scene. Reserve these for names he'd visibly react
  to — six or eight, not every gated subject.
- **Knows and won't say** → an ordinary always-on `SUBJECT` that refuses.
  Fistandantilus after day 4 is the obvious one.

All text freshly written, grounded in voice and tone, never transcribed —
standing precedent, unchanged.

---

## 6. Verification

- **Throwaway self-test** covering: day-range filtering at both edges and
  outside; `-1` open-ended end; window-subject-beats-pool ordering; the
  two-`SUBJECT_WHEN`-same-keyword evolving-answer case across its boundary day;
  `subjectUnknownFor` falling window → character → empty; the hyphen
  tokenization decision from §4.6; and the loader rejecting `SUBJECT_WHEN`
  after a `PRESENCE`, a backwards day range, and a negative `dayStart`.
- **Clean `/W4` rebuild**, zero new warnings.
- **Piped smoke test** — confirms `data/timeline.txt`'s new grammar parses.
- **Interactive verification still needs the user's keyboard** and must be
  flagged as such in the milestone entry: `_getch()` can't be piped. Specifically
  untestable here — typing a gated subject at Solace vs. at Silvanesti and
  confirming the answers differ; confirming the Solace Kitiara override still
  wins over the pool entry; confirming the curated `TOPIC` menu and the "Ask
  about something else..." row both still behave.

---

## 7. Docs to update

- **`docs/TIMELINE_NOTES.md`** — the grammar block in "File grammar"; a new
  subsection under "Ask about anything" covering character-level pools,
  `SUBJECT_WHEN`, the resolution order, why `D` is the current day rather than
  `dayStart`, and the specific-before-general ordering rule. Also record the
  Fistandantilus gate decision from §5.3 explicitly — it's the precedent the
  next character's pass will reach for.
- **`docs/ARCHITECTURE.md`** — the Timeline data-model paragraph (`CanonCharacter`
  gains the pool) and the talk-path paragraph (the adapter now calls
  `subjectsFor`; `talkTo` unchanged).
- **`docs/GOTCHAS.md`** — the hyphen/tokenizer decision, and keyword-ordering
  fragility with the collision-warning behavior.
- **`docs/MILESTONES.md`** — a Milestone 72 entry in the established style, and
  update NEXT UP item 6 (widening `SUBJECT`) to reflect that the pool mechanism
  now exists and the remaining work is the other seven Heroes.
- **`docs/CURRENT_WORK.md`** — replace with this milestone's state, including
  the interactive-verification note from §6.

---

## 8. Explicitly out of scope

- Zone-file (`data/zones/*.txt`) subject pools. `TALK_BEFORE`/`TALK_AFTER`
  already cover zone-NPC time-awareness; revisit only if a zone NPC actually
  needs a day-gated *subject*.
- Fuzzy matching, synonyms, or stemming. Widening a keyword list stays a
  content-only change.
- Any change to `Console::readLine` or `drawAskInputFrame`.
- The other seven Heroes' subject content — Milestone 73.
