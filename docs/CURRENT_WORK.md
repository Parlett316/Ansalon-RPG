# Current work

Nothing in flight. All completed milestones are documented in the
relevant `docs/*_NOTES.md` files and `README.md`'s Status paragraph — see
`CLAUDE.md` for which doc covers which system.

Most recent milestone: Milestone 37 (Silvanesti — the third and final
Winter Night arc, completing the party split Milestone 35 started).
Tanis, Raistlin, Caramon, Goldmoon, and Riverwind — griffon-carried away
from Tarsis in the same dragon attack that sent Sturm/Flint/Tasslehoff to
Ice Wall — finally get their own destination and timeline stop
(`silvanesti 25 30`), instead of their schedules simply dead-ending at
Tarsis. Unlike Ice Wall, this needed no new engine feature: checking the
reference map found Silvanesti is bounded by a river (the Thon-Thalas),
not open ocean, matching the source text's own on-foot ferry crossing —
pure data again, same shape as Milestone 35. New `LOCATION silvanesti`
(a fully-invented ~127-tile road east from Solace — the party actually
arrives by griffon, so unlike every prior road there's no walkable route
in the source material at all to lean on) and a new zone depicting
Silvanost: the Ferry Landing, the Tower of the Stars (Lorac Caladon,
trapped by a dragon orb, tormented by the green dragon Cyan Bloodbane),
the nightmare-corrupted Twisted Gardens, and a generic Warder NPC
(deliberately not Alhana Starbreeze by name — she has extensive ongoing
canon plot, same reasoning that's kept Derek Crownguard and Gunthar
off-stage). Regenerating the overworld grid for the new road silently
wiped Milestone 36's hand-painted Ice Wall glacier patch — reapplied
identically afterward, a live example of the caveat `docs/MAP_NOTES.md`
already documented. Verified via clean rebuild (zero new warnings, no
`.cpp`/`.h` changes), the piped smoke test, and a throwaway self-test. See
`docs/MAP_NOTES.md`, `docs/ZONE_NOTES.md`, `docs/TIMELINE_NOTES.md`.

Next step: nothing pending — ask the user what's next. All three Winter
Night arcs have now shipped; `docs/MILESTONES.md`'s "NEXT UP" list has
terrain-specific monster pools and more monsters as ready candidates, or
starting fresh research into *Dragons of Spring Dawning* for the next
story arc.
