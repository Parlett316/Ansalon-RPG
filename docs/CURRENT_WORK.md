# Current work

Nothing in flight.

Milestone 68 (Anticipation dialogue) just shipped -- the direct follow-up
to Milestone 67's movement-granularity change: since ordinary movement now
typically lands a player *before* a location's `PRESENCE` window opens
rather than mid-window, new `timeline::Timeline::earliestDayStart` (mirror
of `latestDayEnd`) backs a new `TALK_BEFORE` zone grammar so a zone-native
POI can react to "they haven't arrived yet". Deliberately shown on *every*
early visit rather than the "exactly once" treatment `TALK_AFTER` uses --
see `docs/MILESTONES.md`'s Milestone 68 entry and `docs/ZONE_NOTES.md`'s
"Anticipation dialogue" section for the full design/rationale. No
save-format change. One proof-of-concept POI: Haven's Seeker Guard
(`data/zones/haven.txt`). Verified via a throwaway self-test
(`earliestDayStart` against synthetic schedules, `ZoneLoader` parsing
`TALK_BEFORE` plus its two fail-fast cases), a clean `/W4` rebuild, and the
piped smoke test. Interactive verification (talking to the Guard before day
2, confirming the anticipation line repeats across visits, then falls back
to ordinary `TALK`/`TALK_AGAIN` on/after day 2) still needs the user's own
keyboard.
