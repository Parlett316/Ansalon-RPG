# Timeline notes

## What the timeline is

The "chance encounter" engine the whole project was originally pitched
around (see `README.md`): a hand-authored schedule of where canon
Dragonlance characters are on which in-game day. `game::GameState` already
tracks `hoursElapsed` and the player's overworld position for its own
reasons; the timeline just reads both and answers "who's here right now?"
whenever `render::MapRenderer::drawOverworldFrame` draws a location the
player is standing on.

Like `world::World`/`world::OverworldGrid`, a `timeline::Timeline` is
**static content** — loaded fresh from `data/timeline.txt` every run by
`timeline::TimelineLoader`, never mutated during play, and not part of
`game::GameState` or the save format. Only the player's *position* and
*hoursElapsed* (both already saved) determine what a given frame shows.

## File grammar (`data/timeline.txt`)

```
CHARACTER <id>            starts a block; <id> is a stable key -- used as
                          the "have I met this character" tracking key
                          (see GameState::metCharacters, "Talking to a
                          canon character" below)
NAME <text>                display name
PRESENCE <location-id> <day-start> <day-end> <text>   one or more; day
                          range is inclusive, <text> is free-form flavor
                          shown when the player is standing at that
                          location on a day in range
SAY <text>                 optional, follows a PRESENCE line -- the
                          default/first-meeting spoken line shown when
                          the player presses 't' (talk) during that
                          window; attaches to whichever PRESENCE line
                          came immediately before it, not to the
                          character as a whole
SAY_IF <condition> <text>  optional, zero or more, follows SAY -- a
                          reactive variant, checked in the order written
                          the first time this window is talked to; the
                          first condition matching the player's character
                          wins over the plain SAY. Condition vocabulary:
                          good, evil (Alignment's ethical axis); human,
                          dwarf, elf, gnome, halfelf, halfling, kender
                          (race); fighter, mage, cleric, thief, tinker
                          (class). An unrecognized condition just never
                          matches -- checked at talk-time against runtime
                          character data, so TimelineLoader can't validate
                          it at load time the way a typo'd location-id
                          would be caught elsewhere
SAY_AGAIN <text>           optional -- shown on every talk after the
                          first, instead of the generic recognition
                          fallback (see "Talking to a canon character")
TOPIC "<label>" <text>     optional, zero or more -- if this window has
                          any, a topic picker follows the greeting;
                          label is quoted (same convention as ZoneLoader's
                          POI name) since it may contain spaces
END                        closes the block
```

`PRESENCE`'s trailing text is unquoted, taken as "the rest of the line" —
same convention as `WorldLoader`'s `DESC` (it's positionally last, so no
quoting is needed, unlike `ZoneLoader`'s `POI` which has a name *and* a
description on one line and needs the quotes to tell them apart).

**`location-id` is not validated against `data/locations.txt` at load
time** — `TimelineLoader` has no knowledge that `World` exists (same
separation as `WorldLoader` not knowing about `OverworldGrid`). A typo'd
location id doesn't fail to load; that character's window just never
matches anything at query time. Deliberate for a small, hand-authored file
one person edits directly — revisit if this file grows enough that a silent
typo becomes a real risk.

## How presence is shown

`MapRenderer::drawOverworldFrame` calls `Timeline::presentAt(locationId,
day)` (where `day = state.hoursElapsed / 24`, integer division, matching
how the status line already displays "Day N") only when the player is
standing exactly on a `Location`. Every character present gets one line,
`<Name>: <flavor text>`, appended right after that location's own
description.

This is a **persistent** line, not a one-shot toast message — it's part of
the ordinary per-frame render, so it reappears every time the player is at
that location while the window is active, including after leaving and
coming back. Simpler than tracking "has this already been shown," and
better for a "wander around and stumble into people" game than a
blink-and-miss-it notification would be.

## Talking to a canon character

The passive flavor line above is always-on scenery; pressing `t` (talk,
`game::GameLoop::handleTalk`) is a deliberate action that shows the
current `PresenceWindow`'s dialogue in its own dedicated frame
(`render::MapRenderer::drawDialogueFrame`) instead. **If more than one
character is present at once** (all 8 Heroes of the Lance share a
schedule as of Milestone 17), `t` first asks which one via
`render::MapRenderer::drawPickerFrame` — a cursor picker, same
nested-loop shape `handleShop` already established — rather than dumping
every present character's line at once (the original design, when at
most Tanis and Raistlin ever shared a window; stopped scaling once all 8
did). Picking one (or the only one, if just one is present) hands off to
`GameLoop::talkTo`, the single place "have I met them"/reactive-dialogue/
topic logic lives (see below) — every talk path funnels through it, so
there's exactly one implementation to keep correct.

**Real "have I met them" tracking, as of Milestone 18.**
`GameState::metCharacters` (a set of ids, saved via a `MET` line in
`SaveGame`, same shape/treatment as `visitedLocations`) records every
character the player has ever talked to. The *first* time, `talkTo`
shows the character's greeting (their plain `SAY`, or the first matching
`SAY_IF` — see below). *Every time after*, it shows `SAY_AGAIN` if one is
authored, else falls back to a short generic recognition line. The
passive flavor line is untouched by any of this — it stays
persistent/always-on scenery regardless of whether the player has ever
talked to that character.

**Reactive dialogue and branching topics, as of Milestone 19.** Going
"all in" on conversations per direct user request, using the newly-added
Chronicles trilogy (`Dragons_of_Autumn_Twilight`/`_Winter_Night`/
`_Spring_Dawning`) and Legends trilogy (`Time_of_the_Twins`/
`War_of_the_Twins`/`Test_of_the_Twins`) as additional source material
alongside DL1-3. `SAY_IF <condition> <text>` (see grammar above) lets a
window react differently depending on the player's own race/class/
alignment — checked only the first time a window is talked to (a
`SAY_AGAIN`, if any, always wins on repeat visits regardless of
condition). `TOPIC "<label>" <text>` offers a branching menu after the
greeting: `talkTo` shows each topic's label in a `drawPickerFrame` with a
trailing "Nothing, thanks" entry, loops back to the menu after showing a
topic's text (so a player can ask about more than one thing per visit),
and exits on "Nothing, thanks" or `Key::Quit`.

**Scoped to Solace, and to the 8 Heroes — as of Milestone 19.** This pass
went deep on the single richest scene already in the game (the Inn of the
Last Home reunion, day 0-1) rather than wide across every stop — all 8
Heroes have a real `SAY_AGAIN` at all 4 of their stops (replacing the
generic fallback everywhere), but `SAY_IF`/`TOPIC` content only existed at
Solace at first, and only 7 of the 8 Heroes got a `SAY_IF` there
(Riverwind didn't get one forced in without real grounding for it — see
"not force-fit onto everyone" below). Zone NPCs (Otik, Tika, etc.) got
`TALK_AGAIN` content but no reactive dialogue or topics.

**Widened to Xak Tsaroth and Qualinesti — as of Milestone 20.** A
follow-up research pass specifically targeting *Dragons of Autumn
Twilight*'s Xak Tsaroth chapters (the pot-lift ambush, Bupu the gully
dwarf, the confrontation with Khisanth/the Onyx dragon) and its Qualinesti
chapters (the Speaker's feast, the elves' wary homecoming welcome) turned
up strong, specific material for 6 and 5 of the 8 Heroes respectively:
Xak Tsaroth gained topics for Tasslehoff ("The Dragon's Lair" — the one
place in the book he's described as genuinely frightened), Raistlin ("The
Spellbook in the Vault" — his secret bargain with Bupu, naming the
archmage Fistandantilus as an in-world fact/proper noun, not a plot
transcription), Sturm ("Dragonfear" — mastering fear through the Knight's
Code), and Riverwind ("The Chieftain's Daughter" — deeper, non-redundant
material than his existing Solace "Goldmoon" topic), plus `SAY_IF kender`
for Flint (privately terrified for Tasslehoff despite his gruffness) and
`SAY_IF fighter` for Caramon. Qualinesti gained topics for Sturm ("Coming
Home" — a direct parallel between his own Solamnic homelessness and
Tanis's homecoming tension, callbacks to his Solace topic) and Goldmoon
("Tearsong" — the feast triggering a memory of her mother's funeral
banquet), plus `SAY_IF elf` for Flint, `SAY_IF evil` for Raistlin (unmoved
by the Tower of the Sun's beauty), and `SAY_IF thief` for Tasslehoff
(kender-to-thief "professional courtesy" about pocketed silverware).

**Haven deliberately stayed out of scope in Milestone 20 — resolved in
Milestone 26, see below.** Research at the time found it's genuinely thin
ground: the party never actually enters Haven in *Dragons of Autumn
Twilight* (they divert through Darken Wood instead), and DL1's own Haven
material is generic module/refugee-crisis text, not tied to any specific
Hero. Rather than force-fit content the way Riverwind was deliberately
left without a Solace `SAY_IF`, Haven's Heroes kept their Milestone 19
`SAY`/`SAY_AGAIN`-only content through Milestone 25.

**Widened to Darken Wood and Pax Tharkas — as of Milestone 25.** A
follow-up research pass re-read *Dragons of Autumn Twilight*'s Darken Wood
chapters (the spectre army testing the party, Raistlin briefly speaking
with the ancient dead commander's own voice; the Forestmaster/unicorn
scene, recognizing Sturm's blade and naming Huma, and calling Goldmoon's
staff a healer's weapon that will also be turned toward evil) and its Pax
Tharkas approach/interior (Flint's grief that the fortress was built by
elves and dwarves together "in a spirit of friendship since lost on
Krynn"; the Chain Room's defense mechanism, which Tasslehoff finds
irresistible). This turned up strong, non-redundant material — beyond what
the Milestone 24 `SAY` lines already used — for 5 of the 8 Heroes at
Darken Wood (Raistlin's "The Spectre's Commander", Tasslehoff's "Talking
to the Dead", Sturm's "The Forestmaster's Judgment", plus `SAY_IF cleric`
for Goldmoon and `SAY_IF fighter` for Caramon) and 2 of 8 at Pax Tharkas
(Flint's "The Fortress's Making" — a direct counterpoint to his own
Qualinesti `SAY_IF elf` — and Tasslehoff's "The Chain Room").

**Goldmoon's Pax Tharkas content deliberately stayed as-is in Milestone
25.** The book actually has the party split up before the fortress —
Tanis sends Goldmoon, Riverwind, Caramon, Raistlin, and Tika away with the
elves to protect the Disks of Mishakal rather than risk them in the raid.
That directly conflicts with this game's existing simultaneous-presence
abstraction (all 8 Heroes shown "at once" at each stop as a generous
chance-encounter window, not a literal single-path simulation — see
Milestone 24). Rather than invent a workaround, no second Goldmoon beat
was added here; she already gained new content this pass, just at Darken
Wood instead. Same restraint call as Riverwind/Haven above.

**Widened to Haven, and to zone-native NPCs — as of Milestone 26.** A
`pdftotext -layout` re-search of the actual *Dragons of Autumn Twilight*
PDF confirmed the Milestone 20 finding stands: the party plans a trip to
Haven, argues about it, is diverted into Darken Wood before arriving, and
later explicitly abandons the plan for Qualinesti ("Should we still go to
Haven? ... How do we know the Highseeker Council is even in existence?").
Haven is never entered on-page. Rather than invent an "and then the party
did X inside Haven" scene-beat that doesn't exist, the new content reacts
to material that *is* real and pre-dates the diversion: Tanis and Tika's
actual hope that the Highseekers there were still trustworthy; Sturm's
real, unanswered challenge ("And if the Highseekers there are as bad as
the one in Solace?"); Flint's real belief, stated in Solace, that "the
Highseekers in Haven are still wise and virtuous men... just this one
rotten apple in Solace"; Raistlin's real "Seekers! Hah! ... idiot
Seekers" dismissal; and Caramon's real "I've got a notion to go to Haven
and bash—" anger. All 8 Heroes gained a `TOPIC` (Haven previously had
zero, the only stop with that gap); Caramon, Flint, Goldmoon, and Sturm
also gained a `SAY_IF`, all reusing conditions each already carries
elsewhere (`fighter`, `dwarf`, `cleric`, `evil`) rather than introducing
new ones. Riverwind again deliberately has no `SAY_IF`, consistent with
every prior stop. Zone-native NPCs (Otik, Tika, the Seeker Guard, the
Forestmaster, the Fortress Guard) gained their first-ever `SAY_IF`/`TOPIC`
content in the same pass — see `docs/ZONE_NOTES.md`'s "NPCs: POIs you can
talk to" for the grammar and code-side changes that made this possible
(`ZoneLoader`/`PointOfInterest`/`GameLoop::handleTalk` previously had no
path for it at all), and each zone's own section there for what its new
content is grounded in. Otik and Tika's specifically extend real DAT
dialogue (`pdftotext`-verified) rather than the setting-reaction pattern
used everywhere else.

`data/timeline.txt`'s existing Tanis/Raistlin content each got one `SAY`
line per stop, grounded in `TSR 9130 DL1 Dragons of Despair.pdf` (the
original adventure module, added to the project folder specifically for
this) for tone and characterization — Otik and Tika's opening-scene
description on p.7 was the main reference point, carried across to how
Tanis and Raistlin are voiced here too. Per this project's standing
non-infringing precedent (see the intro above), these are freshly written
lines, not transcriptions — DL1 uses a fixed cast of pregenerated PCs and
a specific plot (a stolen Blue Crystal Staff) this game doesn't model, so
only the *voice*, not the *plot*, carried over.

**Zone-interior encounters, as of Milestone 23.** Standing on a location's
overworld tile shows who's there (unchanged); a zone can now also declare
one **`TIMELINE_ANCHOR <char>`** — an already-declared POI where present
canon characters are found and talkable, checked and rendered by
`drawZoneFrame`/`GameLoop::handleTalk` the same way the overworld checks
its own tile. A zone's *effective* timeline location defaults to its own
catalog id (true for every top-level zone: `haven`, `xak_tsaroth`,
`qualinesti` all match their Location id exactly), or can be overridden
with an optional **`TIMELINE_LOCATION <location-id>`** line for a
portal-only zone whose catalog id doesn't match any Location — this is
what lets `solace_inn.txt` (reached only via `solace.txt`'s `I` portal,
no `LOCATION solace_inn` exists) declare `TIMELINE_LOCATION solace` and
check the same schedule the Solace overworld tile does.

Anchor tiles authored so far: the Inn's Great Fireplace (`solace_inn.txt`,
`TIMELINE_LOCATION solace` — the actual novel reunion scene), Haven's
Market Stalls, Xak Tsaroth's Old Well Shaft, and Qualinost's Hall of the
Sky — each picked because it already matched the Heroes' existing
overworld flavor text for that stop. `solace.txt` (the town square, not
the Inn) deliberately has **no** anchor: the real scene is inside the Inn,
so showing the Heroes on the town square too would dilute the "go inside
to actually find them" escalation this feature is meant to create.

**Met-tracking id exception.** Every other zone-based `talkTo` call uses
a synthesized `"<zoneId>:<char>"` id (zone-native NPCs like Otik have no
stable id of their own — see "Real 'have I met them' tracking" above). A
canon character found via a `TIMELINE_ANCHOR`, though, uses their own
stable timeline id (e.g. `"tanis"`) — the *same* id the overworld
encounter uses — so `GameState::metCharacters` treats "met Tanis on the
overworld tile" and "met Tanis at the Inn's fireplace" as the same fact:
whichever happens first, the other shows his `SAY_AGAIN` line, not a
fresh greeting.

`GameLoop::handleTalk` was generalized around a small `TalkCandidate`
shape (`{id, name, Speech}`) so all three talk paths (zone POI,
zone-anchor timeline character, overworld timeline character) share one
`pickAndTalk` helper for the "0/1/2+ candidates" picker logic, rather
than a third near-duplicate loop.

## The current timeline specifically

`data/timeline.txt` now has **all eight** Heroes of the Lance — Tanis,
Raistlin, Caramon, Flint, Goldmoon, Riverwind, Sturm, and Tasslehoff —
sharing the same six-stop schedule (Solace → Haven *or* Darken Wood →
Xak Tsaroth → Qualinesti → Pax Tharkas, days 0–1/2–3/2–3/4–6/7–9/10–12),
since DL1/DL2/DAT's actual text keeps the whole party moving together
through this stretch of the story (DL1 is literally designed to be played
by this cast, via pregenerated character cards; DL2's "Elvenhome" chapter
has the group as a whole travel to Qualinost; DAT's climax is the whole
company together at Pax Tharkas). Windows are deliberately generous (2–3
days each) so a player has a real chance of crossing paths without
needing frame-perfect timing.

**Darken Wood shares Haven's day window (2–3), deliberately.** Milestone
24's research confirmed directly in *Dragons of Autumn Twilight*: the
party never actually visits Haven in this book — they debate it, then
divert through Darken Wood instead. Rather than invent a "what really
happened" answer, both get the same window as two versions of the same
leg of the journey; this project's chance-encounter engine was already a
generous abstraction (all 8 Heroes "simultaneously" at one stop, not a
literal single-path simulation), so showing them at both alternatives on
the same days is consistent with that, not a new liberty.
All flavor/dialogue text is original prose, not quoted from the books —
each character's voice is grounded in their own DL1 pregenerated
character card (STR/background paragraph, read-only `pdftotext` search,
no rendering needed since this was locating character-voice prose, not a
numeric table): Sturm's fixation on his father's fallen Knighthood,
Flint's grudge against the Mountain Dwarves of Thorbardin, Goldmoon and
Riverwind's shared history with the Blue Crystal Staff (Riverwind's Xak
Tsaroth line specifically callbacks his own card's "a well, a shining
lady, a leather-winged creature of evil" description of finding it),
Caramon's protectiveness toward Raistlin (cross-referenced with
Raistlin's own existing Haven line, where he brushes off Caramon's
worry), and Tasslehoff's undimmed kender chaos. A few lines deliberately
tie into zone content added in earlier milestones — Tasslehoff at the
Solace notice board, Sturm/Goldmoon/Flint all reacting to Haven's Seeker
theocracy the zone already establishes.

**Darken Wood and Pax Tharkas (Milestone 24).** Grounded directly in
*Dragons of Autumn Twilight*'s own text (`pdftotext` search, same method
as every prior content pass): Darken Wood's entry sequence (Sturm
following a stag in first "as did Huma," Raistlin's warning to "shoot
nothing, eat nothing, drink nothing," Flint's ghost-story scoffing that
"lacks conviction," Tasslehoff cheerfully mapping a forest that keeps
rearranging itself, Goldmoon's pale but unhesitating resolve, Riverwind
unsettled by ground he can't read) and Pax Tharkas's climax (the
fortress's stone-gate defense mechanism, Raistlin and Tasslehoff finding
an enchanted sword, Flint searching the walls for a secret dwarven-cut
door, Goldmoon's plan to offer herself and the Blue Crystal Staff/Disks
to Verminaard as bait, Riverwind tracking fresh passage through the
tunnels, Sturm and Tanis watching the courtyard as the gates prepare to
seal).

**Plains of Dust and Tarsis stay out of scope, and why.** Plains of Dust
has zero presence anywhere in *Dragons of Autumn Twilight* — it simply
isn't on this book's route (and Milestone 14 already found DL3
establishes Que-Shu, Goldmoon/Riverwind's home tribe, as destroyed by
that point, so inventing a stop there would need to sidestep that
directly). Tarsis is *named* — the book's own closing pages have the
party plan to "travel south in search of the legendary seaport city of
Tarsis the Beautiful" — but that's DAT's setup for its sequel, not a
scene of the party actually being there; writing arrival-scene flavor
text for a place they haven't reached yet would mean inventing what the
source doesn't provide. Both remain real, obvious future work if the
project ever extends into *Dragons of Winter Night*'s territory.
Zone-interior encounters (finding a Hero inside a specific room, not just
standing on the overworld tile) landed in Milestone 23 and were extended
to `darken_wood.txt`/`pax_tharkas.txt` in this same pass — see
"Zone-interior encounters" above.

**Source library addition (Milestone 19).** The user added the full
Chronicles trilogy (`Dragons_of_Autumn_Twilight_-_Margaret_Weis.pdf`,
`_Winter_Night`, `_Spring_Dawning`) and Legends trilogy
(`Time_of_the_Twins`, `War_of_the_Twins`, `Test_of_the_Twins`) to the
project folder. *Dragons of Autumn Twilight* specifically covers the
exact ground already modeled (the Inn reunion, Haven, Xak Tsaroth,
Qualinesti), and was the research target for all of this milestone's
`SAY_IF`/`SAY_AGAIN`/`TOPIC` content (read-only `pdftotext` search —
voice/tone grounding, not numeric tables, so no rendering needed): Tanis
and Raistlin's actual reunion-scene dialogue and description, Sturm's
formality and the moment he registers Tanis's mixed heritage, the "true
gods" theme running through every companion's five-year search (already
echoed in Haven's Seeker content), Raistlin's transformation at the
Tower. Same standing precedent as always — freshly written, inspired by
voice/tone, never transcribed. The other five novels cover story
territory well past what this game currently models (Qualinesti is as
far as the map goes) — noted as available for whenever the game's scope
extends that far, not mined this pass.

## Adding a new character or event

1. Add a `CHARACTER <id> ... END` block to `data/timeline.txt` (or a new
   `PRESENCE` line to an existing character).
2. `location-id` must match a real `LOCATION <id>` in `data/locations.txt`
   for it to ever be seen (see the validation note above — a typo won't
   error, it'll just silently never trigger).
3. Add a `SAY <text>` line right after the `PRESENCE` line if this
   character should be talkable during that window (see "Talking to a
   canon character" above) — optional, but every window added so far has
   one. Optionally follow it with `SAY_IF`/`SAY_AGAIN`/`TOPIC` lines for
   reactive dialogue, a real repeat-visit line, and branching topics —
   none of these are required (they all fall back gracefully if omitted).
4. Build and check the load succeeds — a malformed timeline file fails
   fast with a clear `file:line` error at startup, not partway through
   play.
5. Playtest by walking to the location during the day range (or start a
   fresh character — `hoursElapsed` starts at 0, so day-0 windows are
   immediately visible at the starting location).
