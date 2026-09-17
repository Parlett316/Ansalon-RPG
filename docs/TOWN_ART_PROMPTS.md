# Town/landmark art brief (for AI image generation)

Reference doc for sourcing the illustrated "plate" images used by
`ansalon_sfml_phase1`'s zone landmark plates (Milestone 205) and Gold Box
town menu (Milestone 206) -- see `docs/ARCHITECTURE.md`'s SFML section
and `docs/ZONE_NOTES.md`'s "Town menus" for how these render in-engine.
This file is a working brief to hand to an AI image generator (or a
human artist), not game documentation itself -- update it as new zones
get art, don't treat it as authoritative once a real image exists.

## Style direction

- **Painted illustration, not photo-real, not pixel art.** Matches the
  Gold-Box UI chrome this project already ported (warm parchment/bronze
  panel tones -- see any recent screenshot). Think late-80s/early-90s
  fantasy paperback cover or SSI Gold Box box-art: painterly, moody
  lighting, a single strong focal point, not a busy or cluttered scene.
- **Inspired by Dragonlance, never copied from it.** Do not prompt for
  "Dragonlance," "Krynn," or the names of published Dragonlance
  illustrators (Larry Elmore, Jeff Easley, etc.) -- that steers a
  generator toward reproducing copyrighted compositions. Use the plain
  physical descriptions below instead (they're original-to-this-project
  paraphrases of the setting, already vetted against the source novels
  for the game's own text -- see `docs/MAP_NOTES.md`/`docs/TIMELINE_NOTES.md`
  for that sourcing work). Generic fantasy/medieval imagery in this style
  is what we want, not a specific franchise's look.
- **No people in the foreground, or at most small, distant figures.**
  These are establishing shots of a place, not character art -- keeps
  focus on the architecture/landscape and avoids the game's own canon
  characters (Tanis, Raistlin, etc.) ever needing to match a generated
  face.
- **One image per location.** No text, no logos, no borders/frames baked
  into the image -- the game's own UI already draws a frame around it.

## Technical specs

- **Format**: PNG.
- **Filename**: `assets/plates/<zone-id>.png` -- the exact id is given
  with each entry below (matches the `data/zones/<id>.txt` filename
  stem). Drop the file in with that exact name; no code or data changes
  needed to wire it up (see `docs/ARCHITECTURE.md`).
- **Two different aspect ratios, depending on the zone**:
  - **Banner format** -- wide and short, roughly **3:1 to 4:1**
    (e.g. 1600x450px). Used by any zone with `TOWN_MENU` set (currently
    just Solace), where the image sits in a fixed band above the letter
    menu the whole time the player is in that zone. A tall/square image
    here wastes most of the available width.
  - **Plate format** -- ordinary landscape, roughly **16:9 to 3:2**
    (e.g. 1600x900px). Used by every other (non-`TOWN_MENU`) zone, shown
    full-window on arrival. Either format is contain-fit scaled in-engine,
    so exact pixel dimensions don't matter -- the aspect ratio does.
- Solace is the only zone using the banner format today. Every other
  entry below should be generated in plate format, for whenever that
  zone gets `assets/plates/<id>.png`.

## Per-location prompts

### Solace -- `solace` (BANNER format)
A town built entirely in the upper branches of towering, ancient
vallenwood trees, linked at treetop height by swaying rope bridges and
timber stairways. Warm lamplight glows from windows built into the
trunks themselves; a broad, many-windowed inn with a smoking chimney
sits atop the tallest tree in the scene. Autumn evening light, cozy and
lived-in rather than grand.

### Inn of the Last Home (interior) -- `solace_inn`
The ground floor of a tavern built into the trunk of a great vallenwood:
a broad, low-raftered room lit by lantern-light and a stone hearth big
enough to roast a boar, its fire built straight into the living wood.
Deep-worn wooden tables, a long bar polished by years of use, a narrow
stair spiraling up around the trunk toward the rooms above. Warm,
smoky, lived-in -- a room that's hosted the same regulars for years, not
a grand hall. No people needed (see the "Dialogue portraits" section
below for Otik and Tika specifically).

### Palanthas -- `palanthas`
A wheel-shaped city of white marble ringing a wide bay, broad avenues
radiating out from a central hub toward the sea gates. Elegant, orderly
architecture on a grand scale -- the city clearly survived some disaster
its neighbors didn't. In the far distance, past the rooftops, a single
black tower stands apart from everything else, deliberately the one
dark, foreboding note in an otherwise bright, prosperous skyline.

### Thorbardin -- `thorbardin`
The entrance to a dwarven mountain kingdom: a colossal circular stone
gate, tens of feet across, set into a sheer mountain face, dwarfing any
figure standing near it. Torchlight glows faintly from within the
opening. Beyond hints of a vast, torchlit hall carved from living rock,
its ceiling lost in shadow far overhead. Cold mountain stone, warm
firelight -- underground grandeur, not a dungeon.

### Kalaman -- `kalaman`
A walled harbor city on a wide bay, tall stone walls enclosing streets
built for a busier age, now half-empty. White-sailed ships ride at
anchor in the harbor below. An open-air market spills toward the
waterfront. Overcast coastal light, a city under occupation rather than
at peace.

### Neraka -- `neraka`
A sprawling walled military camp built around one massive black temple,
its spires rising into unnatural, permanent cloud cover that blots out
the sun. Rows of tents and gallows crowd the open ground between the
walls and the temple. Ominous, oppressive, torch-and-campfire lit rather
than daylit -- a staging ground for an army, not a city.

### Pax Tharkas -- `pax_tharkas`
A massive fortress with twin towers carved directly into a narrow
mountain pass, straddling the only road through. The stonework looks
far older than any banner or army currently holding it. Steep cliffs on
either side, a thin ribbon of road disappearing into the gate far below
the towers. Scale is the point -- the fortress should dwarf the pass
itself.

### High Clerist's Tower -- `high_clerist_tower`
A single great fortress-tower built into the only pass through a jagged
mountain range, guarding a road that vanishes into the peaks beyond.
Weathered grey stone, high battlements, a sense of long abandonment
mixed with a garrison still holding on -- banners flying, but faded.
Dramatic mountain light, storm clouds gathering behind the peaks.

### Qualinost -- `qualinesti`
An elven city grown rather than built: living trees shaped over
centuries into soaring, organic halls and archways. Two especially tall
towers dominate the skyline, one gleaming silver, one burnished gold,
connected by delicate hanging walkways. Soft, dappled forest light,
graceful and unhurried -- the opposite mood of Neraka.

### Silvanost -- `silvanesti`
A slender white tower rising alone above a canopy of ancient aspens deep
in an untouched forest, reachable only by a single old ferry crossing on
a wide, still river. Marble paths wind between trees shaped into
strange, beautiful, faintly unsettling forms. Cold, clear, isolated
light -- a place that has shut itself away from the world.

### Tarsis -- `tarsis`
A city built for a coastline that isn't there anymore: stone piers and
the rusting hulks of beached ships half-buried in a vast, cracked expanse
of dry dust and dune, stretching to the horizon where an ocean should be.
Faded nautical rigging still strung between buildings that once faced a
harbor. Dry, sun-bleached, quietly tragic.

### Xak Tsaroth -- `xak_tsaroth`
The flooded ruins of an ancient dwarven city sunk into a swamp, broken
towers and shattered stonework rising out of dark, still water and
tangled mangrove roots. Faint carved dwarven details still visible on
the drowned stonework. Greenish, murky light filtering through overhanging
trees -- a drowned grandeur, unsettling rather than picturesque.

### Haven -- `haven`
A modest farming town on the edge of open plains, low timber-and-thatch
buildings around a busy market square, wagons and livestock in the
streets. Ordinary, lived-in, unglamorous -- a working town, not a
landmark, painted with warm midday light.

### Darken Wood -- `darken_wood`
A dense, ancient forest where sunlight barely reaches the ground, thick
moss and twisted roots underfoot, mist hanging low between the trunks.
No buildings -- the "landmark" is the forest itself, oppressive and
watchful. Muted greens and greys, deep shadow.

### Godshome -- `godshome`
A hidden, barren, bowl-shaped valley deep in jagged mountains, reached
through a narrow cleft in bare rock. At its center, a perfect ring of
ancient standing stones under open sky. Stark, austere, almost lunar
landscape -- deliberately empty and sacred-feeling rather than lush.

### Que-shu -- `que_shu`
The burned ruin of a Plainsmen village: scorched stone walls and a
melted, slumped temple structure, an iron gibbet standing crooked over a
sunken arena. Grassy plains stretch away in every direction beyond the
ruin's edge. Ash-grey and burnt-orange palette, a quiet, wind-swept
aftermath rather than an active disaster.

### Plains of Dust -- `plains_of_dust`
A wide, dry, wind-swept grassland dotted with felt nomad tents, thin
columns of dust rising in the distance where riders or wagons pass. Big
open sky dominates the composition -- the land itself, not any single
structure, is the subject. Warm, dusty, late-afternoon light.

### Hopeful Vale -- `hopeful_vale`
A hidden, narrow mountain valley, its floor scattered with rough lean-to
shelters and cookfire smoke, steep passes visible at either end. Modest
and improvised-looking -- a refugee camp making do, not a real
settlement -- against a backdrop of sheltering peaks.

### Mount Nevermind -- `mount_nevermind`
A gnomish city built into the hollowed interior of an extinct volcano,
crude scaffolding, pipework, and improbable mechanical contraptions
climbing every visible surface of the crater walls, faint smoke and
steam rising from odd machinery. Chaotic, inventive, slightly
dangerous-looking -- barely-controlled industrial energy rather than
elegant architecture.

### Qualimori -- `qualimori`
A refugee camp of elves built hastily along a windswept coastline,
lean-tos and salvaged-sailcloth tents pitched among dune grass, driftwood
cookfires. A stretch of grey ocean behind. Weary and displaced rather
than grand -- deliberately a lesser echo of Qualinost's elegance.

### Foghaven Vale -- `foghaven_vale`
A hollow mountain wrapped in permanent, unnaturally thick fog, a single
carved stone tomb entrance barely visible through the mist at its base.
Muted, near-monochrome palette, heavy atmosphere -- a sacred, half-hidden
place rather than a settlement.

### Sancrist Isle -- `sancrist_isle`
A grey stone castle on a rocky island coastline, battlements facing the
sea, banners of several different heraldic designs flying from its
towers (not matching one single order). Cold, salt-air light, waves
against dark rock below the walls.

### Crossing -- `crossing`
A small huddle of stone piers and net-hung fishing shacks on a narrow
spit of land jutting into a strait, flat-bottomed ferries tied up
waiting, fog rolling in over grey water. Modest and functional --
a ferry-crossing hamlet, not a landmark in its own right.

### Southern Ergoth -- `southern_ergoth`
A wind-bent stretch of coastal forest, driftwood and old bonfire scars
along the shoreline, a scattering of makeshift shelters just visible
among the trees at the wood's edge. Wild, weathered, storm-swept
Atlantic-coast feeling.

### Port O'Call -- `port_ocall`
A small fishing town climbing a headland, net-hung piers and low grey
stone houses, a coastal road visible running off toward distant hills.
Plain, workaday, slightly windswept -- an ordinary port, not a dramatic
one.

### Ice Wall Castle -- `ice_wall`
A half-ruined stone keep on a frozen, wind-scoured coastline, ice
sheeting over broken battlements and fallen masonry, no visible road --
only grey sea meeting grey ice at the horizon. Bleak, cold, isolated.

### Port Balifor -- `port_balifor`
A modest trade port town on a wide bay, fishing boats and a weathered
dockside tavern, a few armored patrol figures in the middle distance
(kept small/anonymous, not a specific army's uniform detail). Workaday
and a little rundown, but not devastated.

### Flotsam -- `flotsam`
A ramshackle port thrown together from mismatched salvage and shipwreck
timber, leaning buildings, a chaotic waterfront with no clear order to
it. Grim, opportunistic, slightly lawless mood -- weathered wood and
patched sailcloth rather than stone.

### Dargaard Keep -- `dargaard_keep`
A distant fortress glimpsed through a gap in dense, dark pine forest on
a steep mountainside, almost no daylight reaching the forest floor even
though it's midday. The keep itself should read as a shadowed silhouette
rather than a clearly lit subject -- unease and dread, not grandeur.

## Dialogue portraits (Milestone 208)

A second, separate art category from everything above: a small per-NPC
image shown beside the text during a conversation with that specific
character, not an establishing shot of a place. See
`docs/ARCHITECTURE.md`'s SFML section for how these render in-engine.

- **Unlike the location plates above, these feature one person, front
  and center** -- the conversation partner themselves, head-and-
  shoulders or waist-up, looking roughly toward the viewer. The "no
  people in the foreground" rule for location plates doesn't apply here;
  it's the opposite brief.
- **Same painted-illustration style direction** as the rest of this
  file (no photo-real, no pixel art; inspired by Dragonlance's voice,
  never copied from its published art or named illustrators).
- **Aspect ratio**: roughly **1:1 to 4:3** (e.g. 800x800 or 900x700px).
  Rendered in a small fixed square box, cover-fit scaled and cropped in-
  engine (same cover-fit idea as the Solace banner, Milestone 207) --
  exact pixel dimensions don't matter, but a portrait-oriented or very
  wide source image will lose more to the crop than a roughly square one.
- **Filename**: `assets/portraits/<zoneId>_<poiChar>.png` -- `<zoneId>`
  is the zone file's own id (matches `data/zones/<id>.txt`'s filename
  stem, same as the plate convention above) and `<poiChar>` is that
  character's own `POI`/`TALK` letter in that zone file. Drop the file
  in with that exact name; no code or data changes needed.
- No text, no logo, no border/frame baked in -- same as location plates.

### Otik Sandeth (Inn of the Last Home) -- `solace_inn_O`
A middle-aged innkeeper behind his own bar, sleeves rolled up, mid-wipe
with a rag or mid-pour -- the settled, good-natured look of a man who's
heard every excuse for an unpaid tab there is and forgiven most of them
anyway. Warm lamplight, the Inn's own hearth-and-lantern glow rather
than daylight.

### Tika Waylan (Inn of the Last Home) -- `solace_inn_Y`
A young woman balancing a full serving tray on one hand with practiced
ease, apron over simple tavern clothes, a quick and appraising look in
her eyes -- someone who's learned to spot trouble before it reaches a
table. Same warm lamplit Inn setting as Otik above.

## Notes for whoever generates these

- Generate a few variations per location and pick the one that best
  matches the mood description, rather than trying to over-specify a
  single "correct" prompt -- these are meant as art direction, not exact
  wording to paste verbatim.
- If a generator's output leans too close to a specific published
  fantasy franchise's iconography (a specific dragon design, a specific
  tower silhouette that reads as "that one book cover"), regenerate
  rather than keep it -- the goal is generic high-fantasy in this
  project's own voice, not a recognizable homage.
- Solace should be done first (it's the only zone that can actually show
  art right now, via the Gold Box town menu). Everything else is ready
  whenever art exists, with no further code changes needed.
