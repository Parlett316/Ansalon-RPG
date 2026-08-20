# Current work

Nothing in flight.

Feature (post-Milestone 51): proactive "quest ready to turn in"
notification, requested directly by the user after playtesting the quest
engine ("it would be nice: Quest complete, return to Whoever to collect
reward"). `QuestStatus` gained a third, append-only-safe value
(`ReadyToTurnIn = 2`) between `Active` and `Complete`; a new
`GameLoop::checkQuestReadiness()` promotes any `Active` quest to it (and
pushes a one-time log line naming the quest and a new required `GIVER
<text>` field, e.g. "The Wolves on the Solace Road is ready to turn in --
return to the Notice Board to collect your reward") from exactly four
places: after a new location is visited, after talking to someone, after
a monster kill, and after accepting a quest (to catch the "already did it
before being asked" edge case). `offerOrTurnInQuest` now branches on this
status directly instead of recomputing `allObjectivesMet` live. The
journal marks a ready quest with a "Ready to turn in!" note instead of
waiting for it to move to the Completed section. `data/quests.txt`'s
`road_wolves` got its `GIVER` line ("the Notice Board"). Verified via a
throwaway self-test (`QuestLoader` requires and parses `GIVER`;
`SaveGame` round-trips `ReadyToTurnIn` and still rejects out-of-range
values) plus a scratch-copy load of the user's real, now-fully-completed
`save.txt` (`QUEST road_wolves 1`, `KILL wolf 3` among others -- proof the
whole offer/accept/track/turn-in loop already worked end to end in real
play before this notification was added) and a clean `/W4` rebuild. See
`docs/QUEST_NOTES.md`'s "Proactive readiness notification" section and
`docs/ARCHITECTURE.md`'s "Quest system" follow-up note.

Bug fix (post-Milestone 51): killing a monster with `STEEL 0 0 0` (Timber
Wolf, Skeleton, Zombie -- animals/undead that carry no coin) crashed the
whole game with a debug assertion in `<random>`. Root cause:
`character::roll(count, sides)` constructed `uniform_int_distribution<int>
die(1, sides)` unconditionally, before ever checking `count`, so
`roll(0, 0)` violated the distribution's own `min <= max` precondition.
Reported by the user via `Bugs/errorinbattle.png` (a real Timber Wolf
fight -- their save also confirmed the quest system already working in
practice: `road_wolves` accepted from the Notice Board, a kobold kill
already tracked). Fixed in `character/Dice.cpp`: `count <= 0` now returns
`0` before constructing anything. Verified via a throwaway self-test
(`roll(0, 0)`/`roll(0, 8)`/`roll(0, 20)` return 0, ordinary rolls stay in
range across 500 iterations each) and a clean `/W4` rebuild. See
`docs/COMBAT_NOTES.md`'s "Bug fixed" section and `docs/GOTCHAS.md`'s
Combat section.

Milestone 51 (quest system engine) shipped: the first version of a quest
system, built as glue over systems that already existed rather than new
machinery -- `VISIT`/`TALK` objectives are pure queries over
`visitedLocations`/`metCharacters` (tracked since Milestones 3/18), and
`SLAY` needed only one new lifetime kill tally. New `quest::Quest`/
`QuestCatalog`/`QuestLoader` (`data/quests.txt`, its own fail-fast loader,
zero in-project dependencies, same shape as `timeline::Timeline`). Zone
files gained a `QUEST <char> <quest-id>` POI keyword (modelled on `PORTAL`,
validated the same way `BOAT` is -- must already have a `TALK` line).
`GameState` gained `quests` (id -> `QuestStatus`, Active/Complete, absence
= not started) and `monsterKills`; both round-trip through the save file as
new optional `QUEST`/`KILL` lines, written right after `BOAT` and before
`ZONESTACK` per its documented ordering invariant. Turn-in flow lives in
`GameLoop::offerOrTurnInQuest`, hooked into `talkTo` the same slot the
`grantsBoat` precedent established; the journal is a new `g` key (`j`/`q`/
`l` were all already taken) opening a one-keypress `drawJournalFrame`.
`conditionMatches` gained a `knight` condition so `REQUIRE` can gate a
quest to Knights of Solamnia. One proof-of-concept quest ships,
`road_wolves` (kill 3 timber wolves), offered by Solace's Notice Board --
its existing "armies on the move in the east" flavor text was the hook the
zone docs had already called out for this. `DELIVER`/item objectives were
explicitly deferred to Milestone 52 (a real quest-item subsystem, the
riskiest thing to build near the user's real save) -- see the "Decided with
the user" section of the approved plan and `docs/QUEST_NOTES.md`'s cut list.

Verified via: a throwaway self-test (`QuestLoader` against the real
`data/quests.txt` plus a battery of malformed-input cases; `SaveGame`
QUEST/KILL round-trip and backward compatibility against a
pre-Milestone-51-shaped save) -- all passed, then deleted per the standard
pattern. A second throwaway check loaded a scratch copy of the user's real
`save.txt` under the new `SaveGame::load` and confirmed it still loads
clean with empty `quests`/`monsterKills` maps. Clean `/W4` rebuild, zero
new warnings, twice (once after the quest-loader self-test, once after a
second throwaway real-save-load check -- both deleted before their
respective final rebuilds). Piped character-creation smoke test passed
(proves `QuestCatalog` loads alongside the other catalogs), real
`save.txt` moved aside and restored around every test, as always.
**Interactive UI verification (dialogue boxes, the Accept/Decline picker,
the journal screen) was NOT done via real keypresses** -- this project's
`_getch()`-based input can't be piped (see `docs/GOTCHAS.md`), and no
tmux/PTY driver exists for this Windows console app, so the `run` skill's
usual approach doesn't apply here either. That interactive path (talk to
the Solace Notice Board, accept `road_wolves`, check `g`, kill 3 wolves,
turn it in) still needs a real human playtest before calling the UI truly
done -- flagging this clearly rather than claiming full verification.

Docs updated: `docs/QUEST_NOTES.md` (new), `docs/ZONE_NOTES.md` (the
`QUEST` keyword), `docs/ARCHITECTURE.md` (module map, dependency
paragraph, "what's deliberately NOT abstracted yet", Milestone 51 section),
`docs/GOTCHAS.md` (save ordering, `j`-is-South, the unvalidated met-id,
the C4061 correction), `docs/CHARACTER_NOTES.md` (the `knight` condition),
`docs/MILESTONES.md`, `README.md`, `CLAUDE.md`'s doc table.

Next: `docs/MILESTONES.md`'s "NEXT UP" now has three live options --
Milestone 52 (real quest content: more quests from ordinary NPCs/Notice
Board/Knight of the Sword, plus the deferred `DELIVER` objective kind),
terrain-specific monster pools, and more monsters. Ask the user before
starting any of them.
