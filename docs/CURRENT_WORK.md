# Current work

Nothing in flight.

Milestone 101 (2026-08-26) shipped per-location shop wares, closing the
"there's no per-location wares" scope cut documented since Milestone 28,
at the user's explicit request. Six shops now exist across five towns
(Solace x2, Haven, Tarsis, Kalaman, Palanthas), each with a distinct
`character::ShopCatalog` filtering `character::Equipment`'s items instead
of every shop sharing one identical catalog; `purchaseItem` was also
switched from fragile position-based offset math to dispatching directly
on a new `ShopItemKind` discriminant. Flint's Smithy (Solace) is a new
shop, gated behind the existing `ore_for_the_forge` quest via a new
`SHOP_LOCKED <char> <quest-id>` zone-grammar line (modeled on `QUEST`,
cross-validated in `main.cpp`). Crossing/Port Balifor/Flotsam were
deliberately left without a shop (no plausible friendly-merchant POI).
Verified via a throwaway self-test, a clean `/W4` rebuild, and the piped
smoke test; **interactive verification (confirming the smithy's lock/
unlock and each shop's distinct catalog on the user's own keyboard) is
still needed** -- see `docs/MILESTONES.md` entry 101 and
`docs/CHARACTER_NOTES.md`'s "Six shops, six catalogs".
