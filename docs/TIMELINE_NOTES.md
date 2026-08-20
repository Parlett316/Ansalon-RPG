# Timeline notes

## What the timeline is

The "chance encounter" engine the whole project was originally pitched
around (see `README.md`): a hand-authored schedule of where canon
Dragonlance characters are on which in-game day. `game::GameState` already
tracks `hoursElapsed` and the player's overworld position for its own
reasons; the timeline just reads both and answers "who's here right now?"
whenever the player arrives at a location. As of Milestone 30, that
answer is pushed to `GameLoop`'s scrolling event log once on arrival
(`GameLoop::announceOverworldTile`) rather than redrawn by
`render::MapRenderer::drawOverworldFrame` every frame — see
`docs/ARCHITECTURE.md`.

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

`GameLoop::announceOverworldTile` calls `Timeline::presentAt(locationId,
day)` (where `day = state.hoursElapsed / 24`, integer division, matching
how the HUD already displays "Day N") only when the player has just
arrived exactly on a `Location`. Every character present gets one line
pushed to the scrolling event log panel right after that location's own
heading/description.

As of Milestone 30 this is a **logged event, not a redrawn status
line** — pushed once per arrival (see `docs/ARCHITECTURE.md`), not
recomputed on every frame the player happens to be standing there. It
still reappears on leaving and coming back (arriving again re-triggers
the push), so "wander around and stumble into people" still works the
same way — it just no longer stays pinned on screen indefinitely while
stationary, and can scroll off the log panel if enough other events
happen first.

**As of Milestone 43, that one line is just `"<Name> is here."`, not
`<Name>: <flavor text>`.** The character's flavor text (the current
`PresenceWindow::flavorText`) no longer auto-prints — it now shows on
demand via Look (`;`, `GameLoop::lookOverworld`/`pickAndLook`), the same
way a zone NPC's `description` does (see `docs/ZONE_NOTES.md`). If more
than one character is present at once, Look asks which one first, via
the identical picker `t` uses below for "Talk to whom?".

## Talking to a canon character

The flavor text (as of Milestone 43, shown via Look rather than
auto-printed — see above) is one deliberate action; pressing `t` (talk,
`game::GameLoop::handleTalk`) is a second, separate one that shows the
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
authored, else falls back to a short generic recognition line. Look's
flavor text is untouched by any of this — it's available on demand
regardless of whether the player has ever talked to that character.

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
canon characters are found and talkable, checked by
`GameLoop::announceZoneTile`/`handleTalk` (Milestone 30 moved the
announce side from `drawZoneFrame` into `GameLoop` — see
`docs/ARCHITECTURE.md`) the same way the overworld checks its own tile.
A zone's *effective* timeline location defaults to its own
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
sharing the same seven-stop schedule through *Dragons of Autumn Twilight*
and the opening of *Dragons of Winter Night* (Solace → Haven *or* Darken
Wood → Xak Tsaroth → Qualinesti → Pax Tharkas → Tarsis, days
0–1/2–3/2–3/4–6/7–9/10–12/20–22), since DL1/DL2/DAT's actual text keeps
the whole party moving together through this stretch of the story (DL1 is
literally designed to be played by this cast, via pregenerated character
cards; DL2's "Elvenhome" chapter has the group as a whole travel to
Qualinost; DAT's climax is the whole company together at Pax Tharkas; DWN
opens with everyone still together in Tarsis). Windows are deliberately
generous (2–3 days each) so a player has a real chance of crossing paths
without needing frame-perfect timing. **As of Milestone 35, the shared
schedule ends at Tarsis** — Sturm, Flint, and Tasslehoff continue on,
first to an `ice_wall 38 42` window (Milestone 36) and then to a
`high_clerist_tower 76 80` window, while Tanis, Raistlin, Caramon,
Goldmoon, and Riverwind instead gain a `silvanesti 25 30` window
(Milestone 37); see "Dragons of Winter Night: the party splits" below for
the mechanics of the split itself, and "Ice Wall"/"Silvanesti" below for
each group's own destination.

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

**Plains of Dust stays out of scope, and why.** Plains of Dust has zero
presence anywhere in *Dragons of Autumn Twilight* — it simply isn't on
this book's route (and Milestone 14 already found DL3 establishes
Que-Shu, Goldmoon/Riverwind's home tribe, as destroyed by that point, so
inventing a stop there would need to sidestep that directly), and *Dragons
of Winter Night* doesn't send the party there either (see below). Remains
real, obvious future work if a later milestone extends into that
territory. **Tarsis was the same story through Milestone 34** — DAT's own
closing pages only have the party *planning* to travel there, never
arriving on-page — but Milestone 35 resolved this properly: *Dragons of
Winter Night* opens with the party already in Tarsis, so it finally has
real timeline content — see "Dragons of Winter Night" below. Zone-interior
encounters (finding a Hero inside a specific room, not just standing on
the overworld tile) landed in Milestone 23 and were extended to
`darken_wood.txt`/`pax_tharkas.txt` in this same pass — see
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

## Dragons of Winter Night: the party splits (Milestone 35)

The first timeline content sourced from a novel beyond *Dragons of Autumn
Twilight*, and the first time the 8 Heroes' schedules genuinely diverge —
the data model already supported this natively (each `CHARACTER` block has
always had its own independent `PRESENCE` list; the 8 sharing identical
windows through Milestone 34 was a fact about the source material, not an
engine constraint), so no code changed, only content.

**Sourcing.** Full text of *Dragons of Winter Night* and *Dragons of
Spring Dawning* extracted via `pdftotext -layout` (same method as every
prior research pass) and searched directly, line-cited below (line numbers
refer to that extraction, not any published page numbering).

**The split, verified against the actual text.** *Dragons of Winter
Night* opens with the whole party together in Tarsis. A dragon attack on
the city (lines 2861-2990) scatters them: Tanis, Raistlin, Caramon,
Goldmoon, and Riverwind are carried by griffon-riders to Silvanesti (lines
3774-3985) — at the time of Milestone 35, not yet a modeled location in
this game, so per this project's standing "don't invent to fill a gap"
rule (the same restraint already applied to Riverwind never getting a
forced `SAY_IF`, or Haven staying out of scope until real material
existed), their tracked schedules simply ended at their new `tarsis`
window, with a real `silvanesti 25 30` stop added two milestones later
once the location itself was properly researched and built — see
"Silvanesti" below. Sturm, Flint, and Tasslehoff, separated from that
group in the same chaos, sail on to Ice Wall Castle for a dragon-orb
quest (Milestone 36), then to Sancrist Isle (line 5697-5744, 5920-5996) —
this is where Sturm's Knights' Trial actually happens (Lord Gunthar Uth
Wistan's Castle Uth Wistan, lines 10237-10786 — Gunthar's own name
matches "Castle Uth Wistan," confirmed by name on the reference map, a
nice unplanned cross-check) — and finally to the High Clerist's Tower for
the siege, the Knighting, and Sturm's death (lines 13500-15000+).

**Sancrist Isle is deliberately not modeled, even after Milestone 36 added
sea travel.** It's a real, richly documented scene — Derek Crownguard's
accusation, Gunthar's procedural rescue of Sturm via a gap in the Measure,
the Order splitting into pro-Gunthar/pro-Derek factions on the spot — but
adding a boat mechanic for Ice Wall doesn't retroactively make every other
ocean-locked location worth building; Sancrist's trial content is already
covered as folded-in retrospective `TOPIC` dialogue at the Tower (see
Milestone 35 above) and duplicating it as an actual walkable stop would be
scope creep, not a gap this project failed to notice. Its content stays
folded into the Tower zone's Knight NPC as retrospective `TOPIC` dialogue
— he can talk about a trial he witnessed, the same "describe, don't model"
treatment the game already gives Sla-Mori or the Inn's upper floor.
Vingaard Keep, similarly, is only ever mentioned in the text (a cut-off
supply source, Sturm's ancestral homeland) and never visited on-page —
confirmed by checking every mention in the extracted text before writing
anything, not assumed.

**Day-range placement.** The existing 8-Hero schedule ends at
`pax_tharkas 10 12`. The new `tarsis 20 22` window (all 8) and
`high_clerist_tower 76 80` window (Sturm/Flint/Tasslehoff only) are soft
placements — *Dragons of Winter Night* gives only vague timing cues, not
exact day counts: the shared prophetic dream from the end of DAT is
referenced as "well over a month" before the Ice Wall voyage (line 5709),
the Sancrist Trial happens "at the beginning of the Yuletide season" (line
10259), and the Tower's garrison had "moved out from Palanthas only a few
weeks ago" (line 13576) by the time of the siege — consistent with roughly
2-3 months total elapsing between Pax Tharkas and Sturm's death. The tight
siege climax itself (blizzard night → Knighting → Derek's fatal sortie →
full assault → Sturm's death → funeral) spans roughly 4-5 in-game days per
the text, reflected in the Tower window's width.

**Why the Tower's zone flavor and Sturm's death don't need a new
mechanic.** There's no "character has died" flag anywhere in this engine,
and this milestone doesn't add one. Sturm's `high_clerist_tower 76 80`
window is simply the last `PRESENCE` line in his `CHARACTER` block — once
day 80 passes, `Timeline::presentAt` stops returning him anywhere, which
already reads correctly as "he's gone" without any special-casing. The
zone's own static flavor text (the Chapel, the Muster Yard) is written to
be evergreen — true whether a player visits before, during, or after his
window — rather than presupposing the Knighting has already happened,
since a zone has no day-gating on its own POI descriptions.

## Ice Wall (Milestone 36)

The second Winter Night stop for Sturm/Flint/Tasslehoff, and the first
milestone to add a real engine feature (sea travel, `GameState::hasBoat`
— see `docs/ARCHITECTURE.md`) rather than pure content, because the
location itself is genuinely unreachable any other way — confirmed by
cropping the reference map at full resolution: Ice Wall Castle sits on a
separate, sea-locked landmass south of Tarsis/Kharolis, no land bridge.
This was presented to the user as an explicit choice (fold in as dialogue,
like Sancrist / build sea travel / do Silvanesti instead), and the user
chose to build sea travel.

**Sourcing.** `dwn_full.txt` lines 5690-5734 (a retrospective recap
chapter): Sturm, Flint, and Tasslehoff, joined by Derek Crownguard and two
other young knights who signed on at Tarsis, search Ice Wall Castle for a
dragon orb, repeatedly fighting off Thanoi ("walrus-men"), winter wolves,
and bears, and losing two of Derek's knights in the process. They find an
ice-encased silver dragon with a mysterious rider (deliberate foreshadowing
the novel itself doesn't resolve yet, so this game doesn't invent a payoff
either), then defeat the dark elf Dragonlord Feal-thas and recover the
orb. `TSR 2143 Player's Guide to the Dragonlance Campaign` (`pg1_full.txt`
lines 2289, 2304-2306, 2340-2342, 5896) independently confirms
"Icewall"/"Ice Mountain Bay" as a real named region south of
Kharolis/Tarsis, home to the Thanoi and Ice Barbarians — matching the
reference map's own "Black Ice Valley"/"Ice Mountain Bay" labels in that
exact spot, and cross-checked by direct pixel inspection (see
`docs/MAP_NOTES.md`).

**Southern Ergoth is deliberately not modeled, even though the original
backlog line named it alongside Ice Wall.** Checking the actual text
before building anything found the ship only sails *past* it on the way
from Ice Wall to Sancrist (`dwn_full.txt` lines 5920-5927 — the captain
points it out at a distance, mentions elves have settled there, nothing
more). The party never lands. Per this project's standing "don't invent to
fill a gap" rule (the same restraint that kept Plains of Dust and Tarsis
out of scope until real material existed), a location the party only sees
from a moving ship doesn't earn its own walkable zone — it's folded into
the Ice Wall Knight's `TOPIC "The Voyage South"` as a forward-looking
mention instead.

**Day-range placement.** `ice_wall 38 42` sits between the shared `tarsis
20 22` window and the trio's `high_clerist_tower 76 80` window — another
soft, invented placement (same disclosed-not-sourced treatment as every
other day range in this file), roughly the midpoint of the ~2-3 month gap
Milestone 35's research already established between Pax Tharkas and
Sturm's death, leaving room either side for the unnarrated legs of the
journey (Tarsis to Ice Wall, then Ice Wall to Sancrist to the Tower).

**How the boat is granted.** `data/zones/tarsis.txt`'s new `R "A Knight's
Runner"` POI, not the existing `S "An Old Sailor"` — see
`docs/ZONE_NOTES.md`'s "Boats: POIs that grant sea travel" for why (the
Old Sailor's own dialogue already establishes Tarsis's harbor as
permanently dead; contradicting that would undercut Milestone 28's
flavor). The Runner represents passage arranged by Derek Crownguard's
knights, who the text confirms "joined them at Tarsis" (`dwn_full.txt`
line 5720) after making camp outside the city during the dragon attack
(lines 2424-2425).

## Silvanesti (Milestone 37)

Tanis, Raistlin, Caramon, Goldmoon, and Riverwind's own Winter Night
destination — the third and final arc from this project's Winter Night
backlog, and the one that finally lets their schedules extend past
Tarsis (the "don't invent to fill a gap" placeholder from Milestone 35's
section above). Unlike Ice Wall, this needed no new engine feature: the
Thon-Thalas River bounding Silvanesti is a river, not open ocean —
confirmed by cropping the reference map — matching the source text's own
crossing (a ferry, on foot, not a ship voyage). See `docs/MAP_NOTES.md`
for the placement/road details.

**Sourcing.** `dwn_full.txt` lines 3770-3990: three days of griffon
flight east from Tarsis (piloted by Alhana Starbreeze, a Silvanesti
princess who joined the party at Tarsis), landing at the Thon-Thalas
riverbank when the griffons refuse to fly further, then crossing on foot
via "the ferry landing... down around the bend" (line 3918). Lines
4800-4950: inside the Tower of the Stars, they find Lorac Caladon
(Silvanesti's king, Alhana's father) enthroned and half-conscious,
trapped by a second dragon orb he tried and failed to control years
earlier. The orb summoned the green dragon Cyan Bloodbane to guard
Silvanesti, and has been tormenting Lorac with nightmares so vivid his
own grief-stricken empathy with the land made them physically real — the
"trees weep blood" imagery (lines 4043-4058) is the corrupted land
itself, not a separate monster. Raistlin drives Cyan Bloodbane off with
help he refuses to name (line 4926: "With help, I was able to defeat the
dragon" — deliberately unresolved in the source text itself, not a gap
this project's restraint introduced). Lorac's own fate is left open too
("He lives. For the time being," line 4940) — the zone's static text is
written to hold up regardless of exactly when a player reads it, same
"evergreen despite dramatic content" treatment as the Tower's Muster Yard.

**Alhana Starbreeze is deliberately not a named NPC**, despite being a
major on-page character throughout this arc (she pilots the griffons,
leads the party to her father, has her own extensive future plot in
*Dragons of Spring Dawning* and beyond) — same reasoning that's kept Derek
Crownguard and Lord Gunthar off-stage at the Tower: a character with
significant ongoing canon fate doesn't become a static, permanently-
available NPC. `data/zones/silvanesti.txt`'s talkable Warder is generic,
the same "unnamed sentinel" pattern used for Pax Tharkas's Fortress
Guard and the Tower/Ice Wall Knights.

**Day-range placement.** `silvanesti 25 30` follows directly from the
text's own "the third day" flight-time cue (line 3774) after leaving
Tarsis (`tarsis 20 22`), plus a few days' settling time once they arrive
— soft and invented, same disclosed-estimate convention as every other
day range in this file. This window is entirely disjoint from the other
group's `ice_wall 38 42`/`high_clerist_tower 76 80` windows, both in the
characters involved and the days — the two groups' stories don't need to
stay in lockstep, matching how the novel itself cuts between them freely.

**Silver dragons, Cyan Bloodbane, and Thanoi all stay flavor-only, not
monster-roster entries.** Unique, named canon creatures with specific
plot roles (the ice-bound silver dragon and its rider at Ice Wall, Cyan
Bloodbane here) were never candidates for `data/monsters.txt` — that file
holds generic, repeatable encounter types, not named story beats. This
isn't a new restraint call, just a restatement of the same one Milestone
36 already made for Feal-thas and the Thanoi.

## Sturm's death (Milestone 38)

The bridging event between *Dragons of Winter Night* (this project's
existing content) and *Dragons of Spring Dawning* (the next content arc):
Spring Dawning opens with Sturm already dead and buried beneath the
Tower's ruins, but the timeline had no death event modeled — his schedule
simply stopped at `high_clerist_tower 76 80`, "freshly vigiled and
formally sworn," with no siege ever happening. Researched and sourced
directly from `Dragons_of_Winter_Night_-_Margaret_Weis.pdf` (a fresh
`pdftotext -layout` extraction, lines ~14800-15450), not from memory or
from Spring Dawning's own backward references to it.

**What the source says.** At dawn, Sturm climbs alone to the Tower's high
wall specifically to draw the attacking dragons' attention away from the
courtyard, where Laurana is readying the dragonlance-armed knights and
Tasslehoff is preparing the dragon orb. He wounds the lead dragon and a
second rider's wing with arrows, then meets the Dragon Highlord's killing
dive with a formal knight's salute — returned in kind, gravely, before the
Highlord's spear runs him through. His stand buys the exact seconds
needed: the dragon orb's captured call lures the attacking dragons into
two ambush chambers deeper in the Tower, where dragonlance-armed knights
kill them at close range (Flint is the one who springs the portcullis trap
on the first). The siege breaks; the army outside, leaderless in the
moment, routs.

**Added as a single-day `PRESENCE ... 81 81` window** at
`high_clerist_tower` for Sturm, Flint, and Tasslehoff — one day, not a
multi-day range like `ice_wall 38 42`, since the source describes the
whole siege happening within one dawn. Sturm's window deliberately has
**no `SAY`** — the first deliberate use of the existing "no SAY = not
talkable" mechanic (previously just meant "not authored yet") to represent
a character who is unavailable because he's dying in the scene being
narrated. It's also the schedule's last-ever window for him: no further
`PRESENCE` lines exist for `sturm` anywhere in the file, which is how the
engine already represents "no longer encounterable" — no new state or
mechanic was needed. Flint and Tasslehoff's day-81 windows carry real
`SAY`/`SAY_AGAIN`/`TOPIC` content, a grief beat continuing each of their
existing 76-80 windows.

**The Dragon Highlord who kills him is deliberately unnamed** (he's
Kitiara in the source text, revealed to Laurana afterward) — same
off-stage-major-character precedent as Alhana Starbreeze at Silvanesti and
Derek Crownguard/Lord Gunthar at the Tower itself: a character with
extensive ongoing canon plot (Kitiara is a Dragon Highlord antagonist
across all three Chronicles books) doesn't get named here. Laurana herself
also stays unnamed throughout, for the same reason.

**`data/zones/high_clerist_tower.txt` is untouched.** The Tower physically
collapses in the source text, but zone content in this project is static
and not day-gated (only `timeline.txt` `PRESENCE` text is) — a player can
walk to the Tower's zone before day 76 with nothing preventing it, so a
POI description couldn't accurately show a post-siege collapsed state
without being wrong for every earlier visit. The siege and its aftermath
stay entirely in `timeline.txt`, same restraint already used for Ice
Wall's and Silvanesti's "evergreen regardless of when a player reads it"
zone text.

## Kalaman (Milestone 39)

The first *Dragons of Spring Dawning* content milestone, and the biggest
single content pass to date: a new `LOCATION kalaman` (see
`docs/MAP_NOTES.md`), a new zone (see `docs/ZONE_NOTES.md`), and
`PRESENCE` windows for six of the eight Heroes — every one still alive and
not already split off elsewhere at this point in the story (Sturm died at
Milestone 38; Raistlin escapes the Blood Sea maelstrom via the dragon orb
directly to Palanthas and is never physically at Kalaman in this arc, so
he gets no window here). Sourced directly from a fresh `pdftotext -layout`
extraction of `Dragons_of_Spring_Dawning_-_Margaret_Weis.pdf`, not from
the earlier summary-level research pass alone.

**Two day-windows, matching two distinct sourced story beats.**
`kalaman 90 92` covers the Spring Dawning festival and the Knights of
Solamnia's triumphal parade (lines 4714-4995) — Flint and Tasslehoff only,
since they're the two Heroes present in the book at this point (having
continued on from the Tower via Palanthas and the Vingaard Keep dragon
battle, neither modeled as their own zones this milestone). `kalaman 100
100` covers the reunion and the Dragon Highlord's ultimatum (lines
8060-8362, 8438-8446) — all six: Tanis, Caramon, Goldmoon, and Riverwind
(shipwrecked, sea-elf-rescued, and washed ashore north of the city — their
first appearance since `silvanesti 25 30`; the Flotsam/Blood Sea/shipwreck
chapters between the two stay unmodeled, same "don't invent to fill a gap"
restraint as every prior time-skip in this file) plus Flint and Tasslehoff,
now grieving both Sturm's death and the Golden General's capture at
Dargaard Keep (referenced in dialogue only — the ambush itself isn't
separately modeled, same restraint as Sancrist Isle/Southern Ergoth staying
off-page). Day 100 is deliberately a single day, matching the "single noon"
pattern Milestone 38 already established for Sturm's death, and is
consistent with Flint's own book line that the groups were "parted in
Tarsis months ago" (`tarsis 20 22`).

**Kitiara ("the Dark Lady") and Laurana ("the Golden General") both stay
unnamed.** Same off-stage-major-character precedent as every prior
milestone (Alhana, Derek, Gunthar, and Kitiara herself already at
Milestone 38) — both have extensive ongoing canon plot beyond this
project's scope. This milestone also references, without naming as a
tracked character, Berem (the "Green Gemstone Man" the ultimatum demands);
he isn't a Hero of the Lance and isn't modeled.

## Palanthas (Milestone 44)

The second *Dragons of Spring Dawning* content milestone, and — unlike
Kalaman — resolves a gap Milestone 39 left open rather than starting one:
Raistlin escapes the Blood Sea maelstrom alone via the dragon orb
("Caramon collapses... Raistlin was gone," confirmed Caramon does *not* go
with him) with nowhere modeled for him to land until now. Sourced from a
fresh `pdftotext -layout` extraction of `Dragons_of_Spring_Dawning_-_
Margaret_Weis.pdf`, independently re-verified line-by-line against the
live text before any dialogue was written (not from the earlier
summary-level pass alone — same standard Milestone 39 set).

**Three characters, one shared arrival scene.** The chapter immediately
following Raistlin's escape opens on Astinus of Palanthas; Bertrem finds a
dying mage on the Great Library's steps and Astinus has him carried inside
— "no one has ever been admitted except those of our order." Raistlin
searches forbidden spellbooks, triggers a magical explosion, and Astinus
delivers the "I am the world... every tear shed, mine have flowed" speech
before Raistlin invokes the dragon orb once more and collapses, apparently
dead — left ambiguous on the page, not resolved until later books. The very
same event is witnessed from outside: Tasslehoff, walking with Flint,
glimpses "Raistlin" carried into the library. This is why all three
characters' `PRESENCE palanthas` windows sit at the same location (the
Great Library, also the zone's `TIMELINE_ANCHOR` — see `docs/ZONE_NOTES.md`)
rather than being scattered across the zone's other sourced beats (the
Shoikan Oak Grove, Lord Amothus's map room) that only Flint and Tasslehoff
actually experience.

**Raistlin's window (`palanthas 83 83`) has no `SAY`, deliberately mirroring
Sturm's own `high_clerist_tower 81 81` death window.** `TimelineLoader`
never requires `SAY` to follow a `PRESENCE` — this is an intentional,
reusable device for "the scene is dramatic/final, not a conversation," not
a gap to fill in later. His fate stays as ambiguous in this game as it is
in the book: he silently drops out of both Talk and the announce log after
day 83 (`!dialogue.empty()` filters him out of `handleTalk`'s candidates,
same as Sturm), but the Look mechanic (Milestone 43) still surfaces his
flavor text on that one day, since Look is deliberately unfiltered.

**Flint and Tasslehoff's window (`palanthas 83 89`) fits the one open gap
in their own schedule exactly**, confirmed against the live file rather
than assumed: their prior window is `high_clerist_tower 81 81` (witnessing
Sturm's death), their next is `kalaman 90 92` — days 82-89 are open. The
in-scene line "She's still not over Sturm's death. It's only been a week,"
said on the Old City Wall's battlements, dates that specific scene to
≈day 88 — inside this window, not just adjacent to it. `TOPIC "A City Worth
Seeing"` (Flint) and the Grove-visit content (Tasslehoff) are both drawn
directly from that same battlements/map-room chapter stretch.

**No window for Caramon, Tanis, Goldmoon, or Riverwind.** Confirmed by
direct text search (not inference) that none of them are ever physically in
Palanthas in this book — they're on the separate Flotsam/Blood Sea/
sea-elf-rescue track that lands at Kalaman on day 100, already modeled.
Caramon's own gap between `silvanesti 25 30` and `kalaman 100 100` stays
unmodeled, same "don't invent to fill a gap" restraint as every prior
time-skip in this file.

## Godshome (Milestone 45)

The third scripted-death `PRESENCE` window in this file, after Sturm
(Milestone 38) and Raistlin's ambiguous collapse (Milestone 44) — this
time sourced from a fresh `pdftotext -layout` extraction of *Dragons of
Spring Dawning* itself (lines ~9570-10450), not summary-level research.

**Who's there, confirmed by direct text search.** Tanis, Caramon,
Tasslehoff, and Flint travel on together from Kalaman (lines 9576-9710
confirm this exact group, plus Tika/Fizban/Berem, none of them tracked
characters); Goldmoon and Riverwind are confirmed staying behind at
Kalaman in the same farewell scene that sends the others onward (lines
8580-8716: "Your work is done, my friend... Here our roads separate")
— consistent with their `kalaman 100 100` window already being their
schedule's last entry, unchanged by this milestone. Raistlin is
separately in Palanthas, already resolved at Milestone 44.

**Flint's death is a heart attack, not violence** — foreshadowed
earlier the same chapter (Tanis noticing him rub his left arm, dismissed
as "rheumatism," line 9599). He collapses chasing a trail; Berem — misread
by Tanis, in blind grief-rage, as an attacker, and stabbed for it — was
actually catching Flint as he fell (Flint's own account, lines 10014-10018:
"this old heart of mine finally burst"). Berem heals instantly, the first
mention in this project of his nature as "the Everman... died countless
deaths, only to rise again" (line 10175) — established just enough for
Tanis's own `godshome` dialogue to reference, not fully explained (his
full Jasla/green-gemstone backstory, lines 10161-10420, stays out of scope
for whenever Neraka itself is built). Flint dies peacefully afterward,
surrounded by friends (lines 10010-10069) — gives Tasslehoff his helm,
has a last exchange with Tanis. An old mage (left unnamed, same treatment
as below) carries the body into a circle of standing stones and a black,
star-filled pool, and both vanish (lines 10079-10143) — heavy
foreshadowing of a divine identity, left exactly as ambiguous here as the
source text leaves it.

**Single-day window, `godshome 103 103`**, added to all four present
Heroes immediately after their shared `kalaman 100 100` window — the
whole scene (arrival, the chase, the death, departure) happens inside one
afternoon in the source text ("leaving Godshome, never to see it again,"
line 10446). Day 103 is a soft, disclosed-as-invented placement, same
convention as every other date in this file — roughly the book's own "a
long and weary day" of wandering (line 9709) then "the second day in the
mountains" when Berem vanishes and Flint dies (line 9793), counted from
the `kalaman 100 100` night departure.

**Flint's window has no `SAY`** — third use of this project's established
"no SAY = scripted death, not absence" device, and his schedule's final
entry: no `PRESENCE` line exists for him anywhere after this one. Tanis,
Caramon, and Tasslehoff each get full `SAY`/`SAY_AGAIN`/`TOPIC` content
reacting to it (Tanis's grief-rage and guilt over Berem; Caramon's guilt
at having lost track of Berem in the first place, distracted worrying
about Raistlin; Tasslehoff's grief over the helm and reaching for the
vanishing pool before Tanis stops him, lines 10037-10073/10144-10153) —
their own three schedules simply end at `godshome` too, for now, same
"don't invent to fill a gap" restraint as every prior stop that came
before its sequel was built (Neraka, the next stop in the book and the
next natural backlog candidate, is not modeled this milestone).

**Berem and the old mage (Fizban, in the source text) both stay
unnamed**, referenced only descriptively in the Heroes' own dialogue —
same off-stage-major-character precedent as Kitiara, Laurana, Alhana
Starbreeze, Derek Crownguard, and Lord Gunthar. The old mage's likely
divine identity is real, load-bearing foreshadowing in the source but
isn't surfaced at all here, consistent with how ambiguous/unresolved
source-text beats are already treated elsewhere in this file (Raistlin's
own fate, Lorac Caladon's).

## Neraka (Milestone 46)

Not a single chapter like every prior stop — the source span covered
(`pdftotext -layout` lines ~10440-15532) is the entire climax and ending
of *Dragons of Spring Dawning*, roughly a third of the novel. Compressed
into one multi-day `PRESENCE neraka 105 107` window per present Hero,
same "restraint over completeness" idiom already used for Pax Tharkas/
Ice Wall/Godshome, not a staged re-enactment.

**Who's there.** Tanis, Caramon, and Tasslehoff (Tika and Berem too,
neither tracked). Goldmoon and Riverwind do not appear anywhere in this
span of the book — confirmed by a full-text search (0 and 1 incidental
hits respectively between the Kalaman farewell and the book's epilogue);
their own schedules stay exactly as Milestone 45 left them, at
`kalaman 100 100`. Raistlin does have a real, direct scene here (see
below) but does **not** get a `PRESENCE neraka` entry of his own — his
tracked schedule stays at `palanthas 83 83`, Milestone 44's last entry,
untouched.

**The Raistlin-continuity note.** Caramon is genuinely reunited with
Raistlin in the Temple dungeons in the source text (lines 12578-14778) —
real magic, real dialogue, not a dream or vision, ending with Raistlin
telling Caramon plainly their paths have split ("You cannot help Tanis.
His fate is in his own hands," lines 14775-14776). This is rich,
important material and directly closes Caramon's own Kalaman-era "The
Brother He Can't Watch" `TOPIC`. Rather than adding a `PRESENCE neraka`
entry to Raistlin's own `CHARACTER` block (which would extend his
*trackable* schedule past where Milestone 44 deliberately ended it),
Caramon's own `neraka` `TOPIC` describes the reunion directly — exactly
the same technique already used everywhere in this file for major
characters who are never themselves tracked (Tanis's dialogue has always
described Kitiara this way, for instance). A player standing at
Raistlin's own overworld icon still finds nothing past Palanthas; that
silence describes what's trackable, not what happens to him off the
board. Worth remembering for any future stop that might be tempted to
extend a schedule that was deliberately closed out in an earlier
milestone: describing an encounter in someone *else's* dialogue is always
available without reopening the first character's own schedule.

**Berem's death is final this time** (lines 13779-13810) — reunited with
his dead sister in a vision, he throws himself onto a jeweled rock column
and dies for good ("The Everman was dead"). This directly resolves
Tanis's own Godshome-era `TOPIC "The Man Who Wouldn't Die"`, closed here
by a new `TOPIC "The Man Who Finally Rested"`.

**Tasslehoff nearly dies** in the dungeons and is saved by Raistlin's
magic (lines 14143-14260). Afterward, the old man from Godshome ("Fizban"
in the source text) has a quiet moment with him that **explicitly
confirms his divine identity** ("Draco Paladin," line 14970) — Godshome's
writeup called this "heavy foreshadowing... left exactly as ambiguous
here as the source text leaves it," and the source text stops leaving it
ambiguous in this chapter. This project keeps the restraint anyway: the
old man stays unnamed in `data/timeline.txt`, same as every other
reference to him so far, rather than retroactively naming a god the game
has never named. The same scene has the old man tell an unprompted,
warm, invented-in-character story about Flint waiting patiently under a
tree by Reorx's forge (lines 14984-15014) — closed here by Tasslehoff's
new `TOPIC "Flint's Tree"`, the single warmest piece of closure in the
milestone.

**Kitiara, Laurana, Ariakas, and Lord Soth** carry enormous plot weight
in this span (the Crown of Power, Ariakas's fall, Kitiara's own
confrontation with Lord Soth) but stay entirely off-stage, referenced
only as much as Tanis's own reaction requires — his refusal of the Crown
of Power for Laurana's sake (lines 13480-13499) is the one moment
surfaced, via his own `SAY`, not restaged. Kitiara survives this book
with her own fate left open (lines 14625-14638 are her last appearance,
shaken but alive) — this project doesn't invent a resolution the source
material doesn't cover here.

**Multi-day window, not the "no SAY" device.** All three tracked Heroes
survive Neraka, so `neraka 105 107` is an ordinary multi-day window like
any other stop, not the single-day scripted-death pattern used for
Sturm/Raistlin/Flint. Days 105-107 are a soft, disclosed-as-invented
placement (a day's travel out of Godshome, then the infiltration/
captivity/escape spanning a couple of days per the book's own structure).

**The book's own final scene** has the survivors talking about "going
back to Solace" (lines 15184-15201) — flagged in `docs/MILESTONES.md`'s
NEXT UP as a clean, low-effort next candidate (existing `LOCATION`, no
new zone needed), not built this milestone.

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
