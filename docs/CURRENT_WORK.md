# Current work

Milestone 43 (frameless overworld/zone layout + NPC "Look") is
implemented and self-verified, but **not yet confirmed in a real,
human-at-the-keyboard playthrough** — `_getch()` can't be piped (see
`docs/GOTCHAS.md`), so none of the following could be scripted:

- The restyled overworld/zone frame reads correctly at a real terminal
  (default size, and ideally also near the new 72×20 minimum).
- The AC/THAC0/Steel two-line split and a long "Standing On" name (e.g.
  the High Clerist's Tower zone, or the Inn of the Last Home) don't
  truncate.
- Look (`;`) with zero NPCs present (unchanged landmark/"nothing here"
  fallback), exactly one present (shows the description directly), and
  2+ at once (the new "Look at whom?" picker — the shared 8-Hero
  schedule is the easy way to trigger this) — try this on both the
  overworld and inside a zone with a `TIMELINE_ANCHOR`.
- Arrival now shows `"<Name> is here."` for an NPC (canon character or a
  talkable zone POI) instead of their full description; a scenery POI
  (no `TALK` line) is unaffected and still shows its full description
  immediately.
- The `'v'` full-log pager (`drawLogFrame`) is visually unchanged — no
  `"> "` prefixes should have leaked in there.

What's already done: implementation, a throwaway self-test that visually
captured the restyled frame at both a comfortable size and the new
72×20 floor (confirmed header/rule/status-panel/log-prefix alignment,
then deleted per the established pattern), a clean `/W4` rebuild, and the
piped character-creation smoke test.

See `docs/MILESTONES.md` entry 43 and `docs/ARCHITECTURE.md`'s
"Frameless overworld/zone layout + NPC 'Look'" for the full writeup.

Next step: have the user play for a bit and confirm the items above, per
CLAUDE.md's session workflow. Once confirmed, clear this file back to
"nothing in flight" and offer the next backlog menu from
`docs/MILESTONES.md`'s "NEXT UP" (unaffected by this milestone).
