# Current work

Nothing in flight.

Milestone 56 (Webnet and Brooch of Imog -- Mage-only combat items from
DLA's "Magical Items of Krynn" chapter) just shipped: two new consumable/
day-gated items, sold at every shop, used via the existing `'i'`-in-combat
key. See `docs/MILESTONES.md`'s Milestone 56 entry and
`docs/CHARACTER_NOTES.md`'s "Magic items" for the full design and sourcing.

**Not yet verified**: real interactive playthrough (buying/using a Webnet
and Brooch mid-fight, confirming the monster's attack is really skipped,
confirming the Brooch's once-per-day gate) -- `_getch()` can't be piped,
the same limitation flagged for every combat/quest milestone so far. Needs
the user's own keyboard before calling the UI path fully done.

NEXT UP (`docs/MILESTONES.md`) now leads with the still-unused `DELIVER`
objective kind, then terrain-specific monster pools, then more monsters.
Ask the user before starting any of them.
