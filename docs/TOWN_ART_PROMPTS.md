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

### Portrait checklist (full 14-character roster)

Status as of 2026-09-17: Otik and Tika now have both expressions
(`solace_inn_O.png`/`solace_inn_O_unknown.png`,
`solace_inn_Y.png`/`solace_inn_Y_unknown.png` -- the reaction shots
landed this session, sourced via `docs/PORTRAIT_PROMPTS.md`'s prompts).
Every other character below still has zero art. This is the full
to-source list for the in-flight "dialogue portraits, full roster" work
tracked in `docs/CURRENT_WORK.md`.

**Filename convention for the reaction expression** (not previously
committed to one): `<normal-filename-stem>_unknown.png` -- e.g. Tanis's
reaction shot is `assets/portraits/tanis_unknown.png`, Otik's is
`assets/portraits/solace_inn_O_unknown.png`. Named for the trigger event
(`SUBJECT_UNKNOWN`, an unrecognized ask-input question) rather than the
emotion, since the emotion itself differs per character (see table).
Nothing in code reads this second file yet -- `loadDialoguePortraitTexture`
(`sfml_phase1/main.cpp:547`) still loads exactly one static texture per
conversation; picking the reaction shot at the right moment is separate,
unwritten work.

Each reaction mood below is read directly off that character's own
`SUBJECT_UNKNOWN` line in `data/timeline.txt` (or `data/zones/
solace_inn.txt` for Otik/Tika) -- not invented, not a uniform "confused"
label.

| Character | Normal portrait | Reaction portrait | Reaction mood (from their own line) | Status |
|---|---|---|---|---|
| Tanis Half-Elven | `assets/portraits/tanis.png` | `assets/portraits/tanis_unknown.png` | Puzzled, apologetic half-smile | Both needed |
| Sturm Brightblade | `assets/portraits/sturm.png` | `assets/portraits/sturm_unknown.png` | Puzzled, formally apologetic | Both needed |
| Caramon Majere | `assets/portraits/caramon.png` | `assets/portraits/caramon_unknown.png` | Puzzled, good-natured, scratching his head | Both needed |
| Goldmoon | `assets/portraits/goldmoon.png` | `assets/portraits/goldmoon_unknown.png` | Puzzled but not unkind, studying you a moment | Both needed |
| Raistlin Majere | `assets/portraits/raistlin.png` | `assets/portraits/raistlin_unknown.png` | Irritated -- weary disdain, turning back to the fire | Both needed |
| Flint Fireforge | `assets/portraits/flint.png` | `assets/portraits/flint_unknown.png` | Irritated -- gruff grunt, unimpressed, out of patience | Both needed |
| Tasslehoff Burrfoot | `assets/portraits/tasslehoff.png` | `assets/portraits/tasslehoff_unknown.png` | Unbothered/delighted -- head tilted, cheerfully unfazed | Both needed |
| Fizban | `assets/portraits/fizban.png` | `assets/portraits/fizban_unknown.png` | Delighted -- face lighting up, patting his robes | Both needed |
| Riverwind | `assets/portraits/riverwind.png` | `assets/portraits/riverwind_unknown.png` | Composed -- steady, patient | Both needed |
| Laurana | `assets/portraits/laurana.png` | `assets/portraits/laurana_unknown.png` | Composed -- doesn't slip, takes a measured moment | Both needed |
| Alhana Starbreeze | `assets/portraits/alhana.png` | `assets/portraits/alhana_unknown.png` | Guarded -- expression closes off, thin patience | Both needed |
| Silvara | `assets/portraits/silvara.png` | `assets/portraits/silvara_unknown.png` | Wary -- won't meet your eyes, uncertain | Both needed |
| Otik Sandeth | `assets/portraits/solace_inn_O.png` (have) | `assets/portraits/solace_inn_O_unknown.png` (have) | Dry, unbothered -- still wiping the bar, "ask me something I can pour you an answer to" | Both have |
| Tika Waylan | `assets/portraits/solace_inn_Y.png` (have) | `assets/portraits/solace_inn_Y_unknown.png` (have) | Breezy, amused -- eyebrow arched, hands full | Both have |
| Astinus (Palanthas) | `assets/portraits/palanthas_Y.png` | `assets/portraits/palanthas_Y_unknown.png` | Unbothered, precise -- pen never slows, "I do not speculate on what might" | Both needed |

**Astinus was missed in the first pass of this checklist** -- he's a
zone POI (`palanthas.txt`, letter `Y`) with his own large `SUBJECT`
pool the user keeps adding to, not a `CHARACTER` entry in
`data/timeline.txt`, so the original sweep (which was keyed off that
file's 12-character roster) never caught him. A check of every zone
file turned up 30 total `SUBJECT_UNKNOWN` lines across 21 zones; the
other ~29 are all one-line generic brush-offs from unnamed guards,
dockhands, and merchants ("not my business," "ask someone who isn't
holding a manifest") -- not characters with enough presence to warrant
individual portrait art, unlike Otik/Tika/Astinus. Worth a second look
later: William (`port_balifor.txt`), who has a bit more personality
than the rest ("ask me about ale, illusions, or pig jokes") but nowhere
near Astinus's depth -- not added here, just noted in case that zone's
scope grows.

**Totals**: 13 normal portraits + 15 reaction portraits = 28 new images
originally to source (Otik/Tika's normal portraits already existed);
Otik/Tika's 2 reaction shots landed this session, leaving **26 still
needed** (12 canon Heroes x 2 + Astinus x 2). That's the "one normal + one
tailored reaction" plan as written in
`docs/CURRENT_WORK.md`'s "Plan so far" paragraph, now covering all 15
characters worth a portrait (12 canon Heroes + Otik + Tika + Astinus).
That plan doc also mentions "2-3 expressions each" and a "42 images"
figure elsewhere, without ever saying what a 3rd expression would be
for -- no 3rd-expression use case is defined anywhere in the docs, so
this checklist resolves the plan to the concrete 2-per-character
reading. Flag this to the user rather than assume if a 3rd expression
turns out to be wanted after all.

### Portrait backgrounds -- open question, not yet decided

The prompts below intentionally give each character's physical
description but leave the **background/setting unspecified** (a plain,
softly atmospheric backdrop -- no specific location baked in). That's
deliberate, not an oversight -- here's why, with the numbers behind it:

- Across all 12 canon Heroes' `PRESENCE` windows in `data/timeline.txt`,
  there are **25 distinct zones** a Hero can be encountered in. Tanis
  alone has 16 of them; Flint and Tasslehoff each have close to 20
  (their late-game wilderness/refugee-camp detours add up). Alhana is
  the one true exception, with exactly 1 (`silvanesti`).
- **Update, Milestone 216 (2026-09-17): every zone now has plate art**
  -- at the time this section was originally written only 11 of the 25
  had one, which is why option 3 below was written off as unworkable.
  That's no longer true; the numbers/analysis below are left as-is for
  the historical reasoning, but the "still falls back... for the 14
  zones with no plate art" caveat on option 3 no longer applies -- it
  can now composite against a real plate for every single encounter
  zone.

A true one-background-per-encounter-location matrix is not a small art
gap to fill in later -- it's real work either way, since it'd mean
roughly 25 backgrounds x 12 characters x 2 expressions for the
well-traveled Heroes alone if done as baked-in images (option 2) rather
than composited at runtime (option 3). This needs a real decision before
any of these prompts get generated for real, not something to quietly
assume. Three ways to resolve it, roughly cheapest to most work:

1. **Generic/neutral backdrop, same idea for every portrait.** A soft,
   indistinct painterly background (firelight, mist, unfocused stone or
   foliage -- whatever suits the character, not the specific room) baked
   into the one portrait image. No code changes, matches the existing
   `assets/portraits/<id>.png` convention exactly, cheapest by far. The
   prompts below are written to this option by default.
2. **One "signature" location per character, baked into the image.**
   Same zero-code-change convention as above, but the backdrop matches
   wherever that character is most narratively tied to (Solace for the
   original eight, Qualinost for Laurana, Silvanost for Alhana, etc.)
   rather than a neutral wash. Costs nothing extra in image count over
   option 1, just more specific (and more research/decision-making) per
   character, and still won't match most of that character's other 24
   encounter locations.
3. **Runtime compositing**: render the character on a simple/plain or
   transparent background, and have `loadDialoguePortraitTexture` (or a
   new function beside it) layer that over the *current* zone's own
   `assets/plates/<zoneId>.png` at draw time, when one exists. Most
   "correct" -- the background would actually match where the
   conversation is happening -- but real new code work. As of Milestone
   216, every zone has a plate to composite against, so this no longer
   needs a fallback for missing art.

Not deciding this now; flagging it for you. The character-description
half of the prompts below is unaffected by whichever option you pick --
only the setting line changes.

## Dialogue portrait prompts -- canon Hero roster

Sourced from the actual novel text (`References/Dragons_of_Autumn_Twilight_-_Margaret_Weis.pdf`
and `References/Dragons_of_Winter_Night_-_Margaret_Weis.pdf`, pre-extracted
to `.research/dat_full.txt` / `.research/dwn_full.txt` / `.research/dosd_full.txt`
for this research pass), not written from memory -- per this project's
sourcing rule. Citations below are `<file> lines ~<range>`. All 12 are
introduced with real physical description in Chapter 1 of *Dragons of
Autumn Twilight* except Alhana (*Dragons of Winter Night*) and Silvara
(*Dragons of Spring Dawning*), who appear in later books. Prompts are
original paraphrases, not transcriptions -- same non-infringement
discipline as the location plates above, and reaction moods reuse the
mood labels already established in the checklist (sourced from each
character's own `SUBJECT_UNKNOWN` line).

### Tanis Half-Elven -- `tanis`
*Sourced: `.research/dat_full.txt` lines ~330-365, 409-410, 1030-1040.*
Tan, weathered skin and a close-trimmed reddish-brown beard grown to
hide elven ancestry, over an elven fighter's lean, graceful build with a
human's thicker muscle. Dressed in supple, hand-tooled leather cut in
elven patterns, a green travel hood pushed back, a longbow visible over
one shoulder. A watchful, weighing expression -- a leader carrying more
than he says.
- **Normal**: as described above, steady and thoughtful, meeting the
  viewer's eyes directly.
- **Reaction (puzzled)**: same figure, an apologetic half-smile, brow
  creased in genuine confusion, one hand absently at his beard.

### Sturm Brightblade -- `sturm`
*Sourced: `.research/dat_full.txt` lines ~994-1042, 1100-1113.*
Tall and straight-backed in antique, dented plate armor bearing the Rose
emblem of his Order, over chain mail. Thick, sweeping moustaches groomed
with evident pride, brown hair touched with gray at the temples, warm
brown eyes despite a stern, formal set to his mouth. One hand resting on
the hilt of an old two-handed sword.
- **Normal**: as described above, proud and formally composed.
- **Reaction (puzzled, formally apologetic)**: same knight, a genuinely
  apologetic expression, brows drawn in confusion he's too well-mannered
  to hide, the faintest formal bow of the head.

### Caramon Majere -- `caramon`
*Sourced: `.research/dat_full.txt` lines ~761-781, 1104-1122, 1145-1150.*
A hugely muscled warrior with a broad, open, good-natured face and dark
hair, wearing a battered winged dragon-crest helm and plain banded or
leather armor, a sword at his hip. An easy grin that doesn't quite hide
real worry underneath.
- **Normal**: as described above, warm and open.
- **Reaction (puzzled, good-natured, lost)**: same big man, scratching
  the back of his head, brow furrowed in honest confusion, a sheepish
  half-grin.

### Goldmoon -- `goldmoon`
*Sourced: `.research/dat_full.txt` lines ~1175-1182, 1200-1245.*
A Plainswoman with a chieftain's daughter's bearing, her face composed
and striking as carved marble. Remarkable silver-and-gold hair, unlike
any other Plainsperson's, loose over her shoulders. Simple but regal
Que-shu tribal dress trimmed with feathers, one hand resting on a plain
staff bound with feathers.
- **Normal**: as described above, composed and quietly regal.
- **Reaction (puzzled but not unkind)**: same woman, head tilted
  slightly, a patient, searching look, the faintest concerned crease
  between her brows.

### Riverwind -- `riverwind`
*Sourced: `.research/dat_full.txt` lines ~1119-1128.*
An extraordinarily tall, rawboned Plainsman (taller than any of his
companions), dark-skinned face drawn thin and pale from past hardship,
wrapped in heavy Plains traveling furs. Quiet strength rather than
warmth -- the stillness of a man always watching the door.
- **Normal**: as described above, steady and watchful.
- **Reaction (composed)**: same figure, unreadable and patient, a
  single measuring look -- no irritation, just quiet attention.

### Raistlin Majere -- `raistlin`
*Sourced: `.research/dat_full.txt` lines ~792-892.*
A slight, gaunt young mage with unsettling golden, faintly metallic
skin stretched tight over sharp cheekbones, eyes with narrow
hourglass-shaped pupils and glittering gold irises, thin lips set in a
faint, private smile. Hooded red robes, one clawlike hand resting on a
plain wooden staff topped with a crystal held in a carved golden
dragon's talon (the Staff of Magius).
- **Normal**: as described above, cold and self-possessed.
- **Reaction (irritated)**: same mage, head turning away in weary, open
  disdain, eyes narrowed, thin hands drawing further into his sleeves --
  already finished with the question.

### Flint Fireforge -- `flint`
*Sourced: `.research/dat_full.txt` lines ~4053-4070, 309-340.*
A stout, heavy-set dwarf with a full gray beard and moustaches, bushy
overhanging white eyebrows, a weathered brown face creased like old
leather, gnarled hands. Dressed in a smith's sturdy leathers, a war-axe
close at hand.
- **Normal**: as described above, gruff but solid.
- **Reaction (irritated)**: same dwarf, arms crossed, a gruff scowl, one
  eyebrow raised in open, out-of-patience exasperation.

### Tasslehoff Burrfoot -- `tasslehoff`
*Sourced: `.research/dat_full.txt` lines ~734, 1469, 1562, 1733,
2427-2430.*
A small, slight, child-sized kender with a long topknot of hair and
bright, restless eyes. Festooned with an enormous number of bulging
pouches, a hoopak slung over one shoulder, an open, delighted grin.
- **Normal**: as described above, cheerful and endlessly curious.
- **Reaction (unbothered/delighted)**: same kender, head tilted with
  cheerful curiosity rather than confusion, entirely unfazed, already
  looking past the question toward something more interesting.

### Fizban -- `fizban`
*Sourced: `.research/dat_full.txt` lines ~186-270, 16000-16043; also
`data/timeline.txt`'s own `PRESENCE` lines ("white-bearded old man in a
scorched, wide-brimmed hat").*
A stooped, white-bearded old man in a tattered gray robe and a scorched,
wide-brimmed pointed hat, leaning on a worn oak staff. Sharp, hawkish
eyes that don't quite match his absent-minded manner.
- **Normal**: as described above, leaning on his staff, an inscrutable
  half-smile.
- **Reaction (delighted)**: same old man, face lit up with open delight
  rather than embarrassment, one hand patting down his robes as if the
  answer's hiding in a pocket, utterly untroubled by not knowing.

### Laurana -- `laurana`
*Sourced: `.research/dat_full.txt` lines ~12788-12839.*
An elven princess with extraordinarily long honey-gold hair spilling
past her waist, smooth woodland-brown skin, delicate elven features,
large expressive eyes. Fine Qualinesti court dress. Composed, with a
hint of shy self-consciousness underneath.
- **Normal**: as described above, graceful and composed.
- **Reaction (composed)**: same elfmaiden, composure held carefully in
  place, a measured breath before answering, a furrow of real thought
  rather than confusion.

### Alhana Starbreeze -- `alhana`
*Sourced: `.research/dwn_full.txt` lines ~1989-1996.*
A striking elven princess ("Muralasa," Princess of the Night) with
black hair soft as the night wind, bound in a fine jeweled net. Skin
pale as moonlight, deep dark-purple eyes, lips tinted like the red
moon's shadow. Rich, formal elven dress.
- **Normal**: as described above, regal and guarded.
- **Reaction (guarded)**: same princess, expression closing off like a
  shutting door, chin lifted, eyes cooling -- patience visibly thinning.

### Silvara -- `silvara`
*Sourced: `.research/dosd_full.txt` lines ~4771-4774; also
`data/timeline.txt`'s own `PRESENCE` line 877 ("silver-haired young
woman... flinches from every raised voice").*
An elfwoman with striking silver hair and deep blue eyes (her true form
is a silver dragon; this is her elven guise). Simple traveling clothes,
a gentle but watchful bearing -- someone who startles easily and stays
near the edges of a room.
- **Normal**: as described above, gentle and quietly alert.
- **Reaction (wary)**: same woman, gaze dropped rather than meeting the
  viewer's eyes, shoulders drawn slightly in, uncertain and apologetic
  rather than confused.

### Astinus -- `palanthas_Y`
*Sourced: `.research/tott_full.txt` lines ~79-193 (his first real
physical description, in *Time of the Twins*), `.research/dosd_full.txt`
lines ~2033-2115 (his own cameo in *Dragons of Spring Dawning*, the
scene this project's Palanthas zone already draws its Astinus
characterization from).* A face "handsome in a timeless, ageless
fashion" that no one who meets him ever quite remembers -- what they
remember instead are his eyes: dark, intent, constantly moving, seeing
everything. Seated at a great polished desk in his study within the
Great Library of Palanthas, a quill in hand moving in firm, unbroken
strokes across a page, a stack of finished parchment at his elbow.
Plain, dark, unadorned scholar's robes (the novel excerpts researched
didn't specify an exact robe color -- left deliberately muted/dark
rather than invented). Utterly composed, unhurried, already writing.
- **Normal**: as described above, mid-sentence, pen still moving,
  glancing up just enough to acknowledge the viewer.
- **Reaction (unbothered, precise)**: same historian, pen still moving
  without pause, an even, unreadable glance, already returning his gaze
  to the page -- not annoyed, just finished with a question that isn't
  history yet.

### Otik Sandeth reaction -- `solace_inn_O` (normal already exists)
*Original-to-project dialogue, not novel-sourced -- see his own
`SUBJECT_UNKNOWN` line in `data/zones/solace_inn.txt`.*
- **Reaction (dry, unbothered)**: same innkeeper as the existing
  portrait, still wiping the bar without pause, one eyebrow raised, a
  dry, patient almost-smile -- the look of a man who's heard every kind
  of question there is.

### Tika Waylan reaction -- `solace_inn_Y` (normal already exists)
*Original-to-project dialogue, not novel-sourced -- see her own
`SUBJECT_UNKNOWN` line in `data/zones/solace_inn.txt`.*
- **Reaction (breezy, amused)**: same server as the existing portrait,
  tray still balanced without looking, one eyebrow arched, a teasing
  almost-smile -- amused rather than annoyed.

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
