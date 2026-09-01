# Current work

Nothing in flight. Milestone 128 (terrain accuracy pass) is implemented,
verified, and documented -- see `docs/MILESTONES.md` entry 128 and
`docs/MAP_NOTES.md`'s "Terrain accuracy pass" section for the full sourcing
and every fixed coordinate.

This grew out of mocking up a possible new "World Map" screen (a
zoomed-out, separate travel view -- see `docs/MILESTONES.md`'s NEXT UP
item 8) against the Cataclysm: Dark Days Ahead reference the user supplied.
The user liked the direction but flagged that terrain accuracy needed
checking against real sources before going further, pointing at two
references in `References/` never previously cross-checked against the
pipeline: the TSR 9400 Trailmap and TSR 8448 Atlas. That check found real
bugs in `data/overworld.grid` itself (zero grassland tiles anywhere,
silently doubling off-road movement time everywhere; a Blood Sea
misclassification leak on the route to Ice Wall Castle; no bog near Xak
Tsaroth despite already-asserted "swampy lowlands" flavor text) --
independent of the World Map screen's fate, so it shipped as its own
milestone rather than being bundled into a UI feature.

**Next open thread**: the World Map screen itself was never built this
session -- only mocked up and conceptually approved. If picked back up,
start from `docs/MILESTONES.md` NEXT UP item 8 rather than re-deriving the
approach; the mockup script and comparison artifact from that session are
gone (scratchpad), so the design decisions (3:1 sampling ratio, importance-
tiered town footprints) live only in that NEXT UP writeup now.
