# Map notes

## Data file grammar (`data/locations.txt`)

Block-structured plain text, parsed line-by-line by `world::WorldLoader`.
Blank lines and lines starting with `#` are ignored anywhere.

```
LOCATION <id>              starts a block; <id> is the stable key (lowercase, no spaces)
NAME <text>                display name, rest of line
REGION <text>               region name, rest of line (not read by game logic yet --
                            reserved for the future timeline engine, see ARCHITECTURE.md)
TERRAIN <tag>                free-form single-word-ish tag, informational only right now
GLYPH <char>                single character drawn on the schematic map (first char of the rest of the line)
POS <row> <col>              two non-negative integers -- stylized grid position, NOT geographic
DESC <text>                 one-line description, rest of line
CONNECT <id> <days> "<text>"   a ONE-WAY road: target id, integer travel days, quoted flavor text
                                shown as "You travel <text>." -- add the reverse CONNECT in the
                                target's own block to make a road two-way (all roads in the current
                                data file are two-way, but nothing requires that)
END                          closes the block
```

Every field is single-line. `WorldLoader` throws a `file:line: message` error
for any malformed line, an `END` missing before end-of-file, or a `CONNECT`
pointing at an id that no `LOCATION` block defines.

## What's canon vs. invented (current data set)

The current `data/locations.txt` covers the opening route of *Dragons of
Autumn Twilight*: Solace, Darken Wood, Haven, Xak Tsaroth, Qualinesti, Pax
Tharkas, Plains of Dust, and Tarsis.

- **Names, relative geography (who's roughly north/south/east/west of
  whom), and the overall travel route** are drawn from the reference map
  (`DragonLance_-_Continent_of_Ansalon_-_Age_of_Despair.jpg`, kept locally,
  not committed — see `.gitignore`) and general published Dragonlance
  geography.
- **Exact `(row, col)` schematic positions are invented** for this project —
  they preserve rough relative direction, not real distances or the
  reference map's actual pixel layout. Don't treat them as authoritative
  geography; they exist only to make the ASCII schematic view legible.
- **Travel times (`CONNECT ... <days>`) are invented** for gameplay pacing,
  not derived from any canon source.
- **Location description text is original prose written for this project**,
  inspired by general knowledge of these locations' role in the novels, not
  copied from any published text.
- **`plains_of_dust` represents a single generic Plainsfolk village**
  standing in for the wider Plains of Dust region (home of the Que-shu and
  related tribes in canon) — it is not meant to represent one specific named
  canon settlement.

## Extending the map

When adding a new `LOCATION`:

1. Pick a `POS` that preserves its rough real/canon direction relative to
   its neighbors — the schematic doesn't need to be precise, but "should be
   roughly northwest of X" should still hold.
2. Add `CONNECT` lines in both the new location and any neighbor it should
   be reachable from (roads are one-way unless declared on both ends).
3. Run the game and check `map` — if the new location doesn't share a row or
   column with a connected neighbor, the road won't draw as a line on the
   schematic (this is expected — see `GOTCHAS.md` — the road still works for
   `go`).
4. Note in this file whether the new location's placement/description is
   drawn from canon or invented, following the pattern above, so this stays
   a reliable record for whoever adds map content next.
