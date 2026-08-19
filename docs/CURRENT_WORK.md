# Current work

Nothing in flight. All completed milestones are documented in the
relevant `docs/*_NOTES.md` files and `README.md`'s Status paragraph — see
`CLAUDE.md` for which doc covers which system.

Most recent milestone: Milestone 41 (Inn-gated complete bed rest), the
faster healing tier Milestone 40 deferred. A new `render::Key::BedRest`
(`'z'`/`'Z'` — not `'b'`, already `SouthWest` in the `yubn` diagonal-
movement scheme; `GameLoop::handleBedRest`) works only standing on a zone
POI newly markable `BED <char>` (`world::PointOfInterest::isBed`, parsed
by `ZoneLoader` exactly like `SHOP`, no `TALK` prerequisite). Shares
`Character::lastRestDay` with ordinary Rest (one overnight action per
day, whichever kind), advances `hoursElapsed` by the same 8 hours, but
heals fully to `maxHp` instead of 1 hp — a deliberate simplification of
the DMG's literal "complete bed-rest" rule (p.74: 3 hp/day plus a
Constitution bonus per full week), dropped because it's a multi-day/
weekly grind that doesn't fit this project's timeline-driven pace. Only
`data/zones/solace_inn.txt`'s existing `U "The Stairs Up"` POI carries
`BED U` — no other zone has an authored Inn/lodging POI. No new save
fields (reuses `RESTDAY`/`lastRestDay`). Verified via a clean rebuild
(zero new `/W4` warnings) and the piped smoke test (confirms the new
`BED` grammar parses); no throwaway self-test, since `handleBedRest` has
no new pure/extractable logic beyond what the build and a real
playthrough cover. See `docs/CHARACTER_NOTES.md`'s "Rest and spell
memorization" and `docs/ZONE_NOTES.md`'s "Beds" section for full sourcing.

**Still needs a live human playthrough** — `_getch()` can't be piped (see
`docs/GOTCHAS.md`), so the interactive keypress loop itself wasn't
exercised by this session. Next time you're at the keyboard: walk to
Solace's Inn, stand on the Stairs Up tile (`U`), press `z`, confirm you
heal fully and the log message reads right; confirm `r` elsewhere still
does the old 1 hp/8 hr behavior; confirm doing both `r` and `z` on the
same day is blocked after the first, in either order.

Next step: nothing else committed to yet. Backlog candidates (see
`docs/MILESTONES.md`'s "NEXT UP"): terrain-specific monster pools, more
monsters, or continuing *Dragons of Spring Dawning* at Palanthas/
Godshome/Neraka. Ask the user which to start next, per the project's
standing workflow.
