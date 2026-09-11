## Build parity matrix

Feature parity between the two live build targets. **`ansalon_sfml_phase1`**
(SFML, pixel-space) is the primary build; **`ansalon_rpg`** (ASCII console) is
the legacy/reference build, still fully working. This table is the source of
truth for parity — prefer updating it over adding parity notes inline in prose.

Legend: **Done** = fully working · **Partial** = works but incomplete or
newly added/lightly tested · **Placeholder** = stubbed, logs/shows a
"not yet" message · **Not yet** = not implemented in this build.

| System | Console (`ansalon_rpg`) | SFML (`ansalon_sfml_phase1`) | Notes |
| --- | --- | --- | --- |
| Save-slot menu & character creation | Done | Done | SFML uses graphical pickers/text entry; console uses typed prompts. Both: 3 slots, 4d6-drop-lowest, race/subrace/class/alignment, weapon specialization. |
| Saves / autosave | Done | Partial | Hardened (Milestone 184): atomic write-then-rename, rotated backups, `VERSION` header, shared verbatim by both targets. Not yet "Done" only because a full multi-session live playthrough without a save issue hasn't been logged yet. |
| Overworld & movement | Done | Done | Same 480×320 grid; SFML renders the reference map in pixel space, console renders colored ASCII. |
| Zone interiors (Enter) | Done | Done | Walkable interiors, POIs, secret places (Foghaven Vale, Qualimori, Mount Nevermind). |
| Talk / dialogue | Done | Partial | SFML confirmed working for greetings, topics, free-text ask, boat accept/decline, companion recruit. Quest offers now open for real (see Quests row) but that path isn't separately confirmed live yet. |
| Chance encounters (Heroes) | Done | Done | Timeline-driven encounters on the overworld and inside zones. |
| Sea travel | Done | Done | Now works in both builds; SFML Talk offers the same Board/Not yet choice as console. |
| Companions | Done | Done | SFML recruitment confirmed at the keyboard; companions fight in SFML combat, sweep/backstab included. |
| Combat (tactical grid) | Done | Done | Real 2e attack/damage math, opportunity attacks, backstab/sweep, draconian abilities, area Fireball. |
| Magic: spells & items | Done | Done | Presentation migration only — the same 50-spell book and item logic apply to both. |
| Shops & equipment | Done | Done | 13 shop POIs across 9 towns; buy/sell, carried inventory, equip changes AC/damage. |
| Leveling | Done | Done | PHB level-by-level tables; Knight/Mage milestone nods. |
| Quests | Done | Partial | Ported (Milestone 183): full offer/accept/decline/progress/turn-in, all six reward flags, `SHOP_LOCKED` unlock, journal rendering real state. **Not yet interactively confirmed** — see `docs/CURRENT_WORK.md`'s Playtest backlog. |
| World map (`O`) | Done | Done | Read-only downscaled reference map with legend and player marker. |
| Look (`L` / `;`) | Done | Partial | Ported (Milestone 182): NPC-present detail view, nearest-location/compass fallback, POI/TIMELINE_ANCHOR descriptions inside a zone. **Not yet interactively confirmed** — see `docs/CURRENT_WORK.md`'s Playtest backlog. |
