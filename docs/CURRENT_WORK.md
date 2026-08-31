# Current work

Nothing in flight. Milestone 124 (a weapons/armor shop and a magic shop
in every town) is implemented and documented -- see `docs/MILESTONES.md`
entry 124 for the full history.

New `magic` catalog (`character::ShopCatalog::Magic`); 13 shop POIs now
exist across all 9 `TOWN`-flagged locations (up from 6 across 5), closing
the weapons/armor + magic coverage gap town by town -- reusing catalogs
that already qualified, adding new POIs only where needed, and reusing
already-written flavor-only POIs at Crossing/Port Balifor/Flotsam (all
three previously documented as *deliberately* shopless -- reversed at the
user's explicit request, reframed as black-market/smuggler commerce
rather than open storefronts).

Verified via a clean `/W4` rebuild (zero new warnings) and the piped
save-slot smoke test, which loads every zone file (including the 9
hand-edited `GRID` blocks) before the save-slot menu renders -- reaching
that menu confirms all of them parsed cleanly. **Not yet interactively
verified**: all three save slots (Mason, Mike, Regan) were occupied this
session, so the piped test couldn't reach character creation, and none of
this has a piped-testable path to a live shop screen (`_getch()` blocks
that) regardless. Trying at least a couple of the new/reused shops (e.g.
Haven's new Farrier's Forge/Relic Peddler's Cart, and Flotsam's now-shop
Back Alley/Saltbreeze Inn, since those needed no new POIs and are the
easiest to get definitively wrong) with the user's own keyboard would
confirm the catalog filtering and item lists look right in practice.
