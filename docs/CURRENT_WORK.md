# Current work

Nothing in flight -- Milestone 118 (Phase 3a of the party system, "A real
multi-companion roster") is implemented and rebuilt clean. Much of it is
now confirmed by the user's own real playthrough: Dessa Corrin was
recruited alongside Bren Alder on the same character, both fought, and the
real save file shows both surviving combat with distinct, correctly
tracked HP (`COMPANION bren_alder 9` / `COMPANION dessa_corrin 2`).

**Bug found and fixed this session**: when a party wipe ends a fight
(`GameLoop::runCombat`'s `knockedOutBy`), only the player was restored to
full HP and carried back to the nearest refuge -- any companion knocked out
during that same fight stayed at 0 HP indefinitely (only a later Rest/
BedRest could revive them, silently benching them from every fight in
between). Fixed by healing every recruited companion to full alongside the
player in `knockedOutBy`. Rebuilt clean (`/W4`, zero new warnings); not yet
re-confirmed by the user in an actual party-wipe scenario.

**Interactive verification still open**: a party wipe (get both the player
and a companion knocked out, then take the killing blow) actually restores
every companion to full HP, not just the player; the coin-flip/random-pick
targeting behavior when a monster is adjacent to more than one party member;
Rest/BedRest healing every recruited companion (1 hp / full respectively);
and a knocked-out companion not ending the fight while the other keeps
fighting. See `docs/MILESTONES.md` entry 118 for the full design writeup,
and `docs/ARCHITECTURE.md`'s "Party companions" / `docs/CHARACTER_NOTES.md`'s
"Party companions" / `docs/COMBAT_NOTES.md`'s "Extending this later"
sections for how the system works. Prior milestones' own history lives in
`docs/MILESTONES.md`, not here -- see that file for the full numbered
writeup of everything shipped before this.
