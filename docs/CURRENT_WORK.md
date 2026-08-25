# Current work

Nothing in flight.

Milestone 78 (Frostreaver, DLA p.94 -- a heavy battle axe of Icewall
Glacier ice) just shipped -- see `docs/MILESTONES.md`'s Milestone 78 entry
for the full sourcing and design writeup. A quest reward
(`data/quests.txt`'s `frostreaver_salvage`, `REQUIRE str_13`, `SLAY thanoi
2`), offered by Ice Wall's existing Young Knight POI, granting a
`character::ItemKind::Weapon` item whose +4 bonus is terrain-gated
(glacier only) via a this-fight-only local bonus in `game::GameLoop::
runCombat` -- the same mechanism spell buffs already use, not a permanent
character stat. Touches `src/character/Equipment.h` (new constants),
`src/game/GameLoop.cpp` (`conditionMatches`'s new `str_13` token, the
`REWARD_FROSTREAVER` grant branch, and the terrain check in `runCombat`),
`src/quest/Quest.h`/`QuestLoader.cpp` (`rewardFrostreaver`/
`REWARD_FROSTREAVER`), `data/quests.txt`, and `data/zones/ice_wall.txt`.
No `SaveGame.cpp` changes -- the `MAGICWEAPON` line format is already
generic over weapon name. Verified via a throwaway self-test
(`QuestLoader` parsing the real `data/quests.txt`, confirming the new
quest's shape and `REWARD_FROSTREAVER`'s fail-fast case -- deleted after
passing), a clean `/W4` rebuild (zero new warnings), and the piped smoke
test (confirms `main.cpp`'s cross-validation accepts the new zone
binding; the user's real executable-relative save was moved aside, left
untouched by the failed piped run, and restored byte-identical --
confirmed via checksum). Docs updated: `docs/CHARACTER_NOTES.md` ("Magic
items", "Extending this later"), `docs/QUEST_NOTES.md` ("Shipped
quests"), `docs/COMBAT_NOTES.md` ("Player actions"), `docs/ZONE_NOTES.md`
("Quests: POIs that offer them"), `docs/MILESTONES.md` (new entry, NEXT UP
trimmed/renumbered).

**Interactive verification still needs the user's own keyboard** --
accepting `frostreaver_salvage` as a Strength-13+ character, killing 2
Thanoi (glacier-biased, near Ice Wall), turning in, equipping the
Frostreaver, and confirming the +4 to-hit/damage applies on a glacier
tile but not off it. This is the one place this milestone most needs real
playtesting, since the terrain-gating is new, untested-by-precedent
logic -- everything else follows an established pattern (Solamnic
Armor/Staff of Striking/Curing) that's already been playtest-confirmed.

Next backlog candidates (not started, not committed) -- see
`docs/MILESTONES.md`'s NEXT UP: interactive verification of Milestone 62's
spellcasting UI (item 1), real Draconian mechanics (item 2), SFML-backed
rendering revisit with real sprite art (item 3), widening `TALK_AFTER` to
three remaining zones (item 4), widening the color palette further (item
5), or zone-NPC `SUBJECT` content beyond Milestone 71's initial pass
(item 6).
