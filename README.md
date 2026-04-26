# Console Civ

A small C17 console strategy-game foundation inspired by Civilization and Battle of Polytopia.

This is v0.1 groundwork: map generation, two hotseat civilizations, conquest, city capture, borders, points economy, science, a non-linear technology tree, tile buildings, multiple unit types, simple combat, settlers, and console rendering. Game logic is kept separate from rendering so another renderer, such as raylib, can replace the console layer later.

## Build

```sh
make
```

## Run

```sh
make run
```

Or run the binary directly:

```sh
./civ_console
```

On startup, the game asks for a map size:

```text
Choose map size:
1. small  - 32x32
2. medium - 48x48
3. large  - 64x64
4. xlarge - 96x96
```

Enter either the number or the word, such as `1` or `small`. Press Enter without typing anything to use the default: `large`, which is 64x64. The game then asks for civilization names; blank names fall back to `Player 1` and `Player 2`.

## Seeded Worlds

Pass an unsigned integer seed to reproduce a map:

```sh
./civ_console 12345
```

If no seed is provided, the game uses `time(NULL)`. The selected seed is printed at startup.

## Commands

- `help` - show commands
- `map` - render the full selected map
- `status` - show current player, points, science, research, units, and selection
- `economy` - show points, science, income, owned cities, science buildings, and border growth
- `tech` - show unlocked, available, and locked technologies
- `research tech_name` - choose the current technology to research
- `buildscience type city_x city_y` - build an internal city science building
- `select x y` - select the current player's unit at a coordinate
- `move north` - move selected unit one tile north
- `move south` - move selected unit one tile south
- `move east` - move selected unit one tile east
- `move west` - move selected unit one tile west
- `attack x y` - attack an enemy unit with the selected unit
- `build type x y` - instantly build a tile building inside your borders
- `upgrade walls city_x city_y` - build city walls
- `train type city_x city_y` - instantly train a unit near one of your cities
- `found city_name` - found a city with the selected settler
- `spawn type x y` - debug-spawn a unit for the current player
- `leaderboard` - show current conquest statistics
- `end` - end the current player's turn
- `quit` - exit cleanly

## Conquest

Conquest is the only mode. A civilization is eliminated when its capital is captured. Eliminated civilizations lose their remaining units, their non-capital cities become free cities, and their turns are skipped. The game ends when one civilization remains, then prints a leaderboard.

City capture uses a simple siege rule: if a unit ends its turn on an enemy or free city tile, the city is under siege. The defender gets one turn to respond. If the sieging unit is still alive on the city tile when the attacker’s next turn begins, the city is captured.

## Economy

Points are the main currency. Each player gains points at the start of their turn from owned cities.

Each city starts with:

- Border radius: `1`
- Base income: `2` points per turn
- Border growth: `1` progress per turn
- First border expansion requirement: `5`
- Second border expansion requirement: `10`
- Maximum border radius: `3`

City borders use Manhattan distance. Tile buildings must be inside the current player's controlled city borders.

## Science And Technology

Science is separate from points. Every owned city produces `1` base science per turn, plus bonuses from internal city science buildings. Science is stored by the player, then applied to the currently selected research at the start of that player's turn. If no research is selected, science accumulates until the player chooses one.

Agriculture is unlocked at the start and unlocks farms, warriors, and settlers. Other technologies must be researched:

- `mining` - costs `15`, unlocks Mine, Defender, City Walls
- `forestry` - costs `12`, unlocks Sawmill, Lumber Camp
- `archery` - costs `12`, unlocks Archer
- `sailing` - costs `14`, unlocks Port, Sloop
- `writing` - costs `10`, unlocks Study Hall
- `construction` - costs `25`, requires Mining, unlocks Catapult
- `horseback_riding` - costs `22`, unlocks Knight
- `shipbuilding` - costs `24`, requires Sailing, unlocks Brig
- `education` - costs `24`, requires Writing, unlocks Campus
- `stealth_tactics` - costs `35`, requires Archery and Horseback Riding, unlocks Assassin
- `navigation` - costs `38`, requires Shipbuilding, unlocks Galleon and Observatory
- `engineering` - costs `40`, requires Construction and Education, unlocks Academy
- `administration` - costs `28`, requires Writing and Mining, adds `+1` science building slot in every city

Technology names accept compact and separator forms where useful, such as `horseback riding`, `horseback_riding`, and `stealth-tactics`.

## City Science Buildings

Science buildings are internal city buildings. They do not appear on the map. A city has science slots based on border radius:

- Radius `1`: 1 slot
- Radius `2`: 2 slots
- Radius `3`: 3 slots
- Administration adds one extra slot, up to the number of implemented science buildings

Science buildings:

- `studyhall`, `study_hall`, or `study-hall` - costs `8`, requires Writing, gives `+2` science per turn
- `campus` - costs `15`, requires Education, gives `+4` science per turn
- `academy` - costs `22`, requires Engineering, gives `+6` science per turn
- `observatory` - costs `20`, requires Navigation, gives `+5` science per turn

A city cannot build the same science building twice. Free cities and enemy cities cannot build science buildings.

## Tile Buildings

- `farm` - costs `5`, requires Agriculture, built on plains, gives `+2` points per turn
- `mine` - costs `7`, requires Mining, built on hills, gives `+3` points per turn
- `port` - costs `8`, requires Sailing, built on water inside borders, gives `+3` points per turn
- `sawmill` - costs `6`, requires Forestry, built on plains or hills, gives `+2` points per turn
- `lumbercamp`, `lumber_camp`, or `lumber-camp` - costs `6`, requires Forestry, built on forest, gives `+3` points per turn

Only one building can exist on a tile. Buildings cannot be placed on mountains or city tiles.

City walls require Mining, cost `15` points, and give friendly units on that city an extra defensive bonus. Walls remain if the city is captured.

## Units

Unit training costs:

- `warrior` - `6`
- `archer` - `8`
- `defender` - `8`
- `knight` - `12`
- `catapult` - `14`
- `assassin` - `14`
- `settler` - `15`
- `sloop` - `10`
- `brig` - `14`
- `galleon` - `20`

Sloops, brigs, and galleons replace the old generic ship. They move only on water. Assassins are fragile melee units that can still move after attacking if they have movement points left.

Training requires the relevant technology. Debug `spawn` ignores technology requirements.

Settlers:

- Render as `L` for Player 1 and `l` for Player 2
- Cannot attack
- Move on plains, forests, and hills
- Can found cities with `found city_name`
- Are consumed when founding succeeds

Founding rules:

- The selected unit must be a living settler.
- The settler must stand on plains or hills.
- The tile cannot already contain a city.
- The new city must be at least 4 Manhattan tiles away from every existing city.

## Combat

Damage is based on attack minus half of total defense, clamped between `1` and attacker attack. Defense bonuses are simple:

- Forest: `+1`
- Hill: `+1`
- Own city: `+2`
- Own walled city: additional `+2`

Units sieging enemy or free cities do not receive the city defense bonus.

## Current Limitations

- Two human hotseat players only.
- Instant training only; no production queues yet.
- Debug spawning is still available.
- No AI, diplomacy, fog of war, save/load, or production queues.
- Land units cannot enter water or mountains.
- Ships can only move on water.
- The renderer always prints the entire selected map.

## Planned Future Features

- Production queues and richer city growth.
- More resources and terrain types.
- Fog of war and exploration.
- Better map generation controls.
- Save/load support.
- A replaceable graphical renderer, likely raylib.
