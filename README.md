# schema227's cli civilization

schema227's cli civilization is a C17 console hotseat strategy game inspired by Civilization and The Battle of Polytopia. It is a playable conquest prototype with separated game logic and console rendering, so a future renderer such as raylib can replace the console layer without rewriting the rules.

## Features

- 2-5 human hotseat players
- Standard mode for normal play and Debug mode for test commands
- Dynamic map sizes: 32x32, 48x48, 64x64, and 96x96
- Random archipelago world generation
- Terrain: water, plains, forest, hill, mountain
- Colored ANSI console rendering with `--no-color` fallback
- Random Romanian city names, plus custom founded city names
- City founding, borders, walls, sieges, capture, free cities, and conquest victory
- Points economy, tile buildings, science, city science buildings, and a non-linear tech tree
- Multiple land and naval unit types
- Combat with terrain, city, and wall defense bonuses
- Endgame leaderboard and player statistics

## Requirements

- GCC
- Make
- Linux, WSL, or a similar POSIX-style environment
- No external libraries

Windows users should use WSL or a terminal that supports ANSI escape codes. Use `--no-color` if color output is not readable.

## Build

```sh
make
```

```sh
make run
```

```sh
make clean
```

## Running

```sh
./civ_console
./civ_console 12345
./civ_console --no-color
./civ_console 12345 --no-color
```

If no seed is provided, the game uses `time(NULL)`. The seed is printed at startup so a map can be replayed.

## Startup

The game asks for:

1. Map size: `small`, `medium`, `large`, or `xlarge`
2. Game mode: `standard` or `debug`
3. Number of human players: 2-5
4. Civilization names

Defaults:

- Map size: `large` (64x64)
- Game mode: `standard`
- Players: 2
- Blank civilization names become `Player 1`, `Player 2`, etc.

Each civilization is assigned a color from red, yellow, blue, and green. In 5-player games one color is reused; player names and numbers remain visible in text commands and legends.

## Game Modes

Standard mode is normal play. Debug and creative commands are hidden and disabled.

Debug mode enables normal play plus test commands such as `spawn unit_type x y`. Debug mode is useful for testing combat, balance, sieges, and map situations.

## How To Play

Pick a research target, inspect the map, manage cities, train units, expand with settlers, and capture enemy capitals. A city is captured only if a unit remains on that city tile until the attacker’s next turn, giving the defender a chance to respond. Capturing a capital eliminates that civilization. The last surviving civilization wins.

Helpful first commands:

- `researchable`
- `research mining`
- `list`
- `mycities`
- `available`
- `todo`
- `map`

## Commands

Core:

- `help` or `h`
- `map` or `m`
- `status` or `s`
- `end` or `e`
- `quit` or `q`

Selection and movement:

- `list`, `units`, or `selectable`
- `select x y`
- `where`
- `moves`
- `move north`
- `move south`
- `move east`
- `move west`

Combat:

- `enemies`
- `attacks`
- `attack x y`

Cities and economy:

- `cities` or `citylist`
- `mycities`
- `economy` or `eco`
- `available`
- `available city_x city_y`
- `buildoptions x y`
- `build building_type x y`
- `buildscience building_type city_x city_y`
- `upgrade walls city_x city_y`
- `train unit_type city_x city_y`
- `found`
- `found custom city name`

Science:

- `tech` or `techs`
- `researchable`
- `research tech_name`

Info and helpers:

- `inspect x y`
- `leaderboard`
- `todo`, `actions`, or `advice`

Debug mode only:

- `spawn unit_type x y`

## Terrain

| Terrain | Symbol | Land units | Ships | Defense |
| --- | --- | --- | --- | --- |
| Water | `~` | No | Yes | +0 |
| Plains | `.` | Yes | No | +0 |
| Forest | `*` | Yes | No | +1 |
| Hill | `^` | Yes | No | +1 |
| Mountain | `M` | No | No | impassable |

Cities can only be founded or generated on plains, forests, or hills.

## Units

| Unit | Role | Move | Range | Unlock |
| --- | --- | ---: | ---: | --- |
| Warrior | Basic melee | 1 | 1 | Agriculture |
| Archer | Ranged attacker | 1 | 2 | Archery |
| Catapult | Long-range siege | 1 | 3 | Construction |
| Defender | Durable garrison | 1 | 1 | Mining |
| Settler | Founds cities | 1 | 0 | Agriculture |
| Knight | Fast melee | 2 | 1 | Horseback Riding |
| Assassin | High damage, can move after attacking | 2 | 1 | Stealth Tactics |
| Sloop | Fast light ship | 3 | 2 | Sailing |
| Brig | Balanced ship | 2 | 2 | Shipbuilding |
| Galleon | Heavy ship | 1 | 3 | Navigation |

Training costs:

- Warrior 6
- Archer 8
- Defender 8
- Knight 12
- Catapult 14
- Assassin 14
- Settler 15
- Sloop 10
- Brig 14
- Galleon 20

Ships move only on water. Land units move on plains, forests, and hills.

## Buildings And Upgrades

Tile buildings:

| Building | Cost | Terrain | Unlock | Income |
| --- | ---: | --- | --- | ---: |
| Farm | 5 | Plains | Agriculture | +2 points |
| Mine | 7 | Hill | Mining | +3 points |
| Port | 8 | Water inside borders | Sailing | +3 points |
| Sawmill | 6 | Plains or hill | Forestry | +2 points |
| Lumber Camp | 6 | Forest | Forestry | +3 points |

City science buildings:

| Building | Cost | Unlock | Science |
| --- | ---: | --- | ---: |
| Study Hall | 8 | Writing | +2 |
| Campus | 15 | Education | +4 |
| Academy | 22 | Engineering | +6 |
| Observatory | 20 | Navigation | +5 |

Science slots come from city border radius: radius 1 gives 1 slot, radius 2 gives 2, and radius 3 gives 3. Administration adds one extra slot, capped by the implemented science building count.

City walls cost 15 points, require Mining, and give friendly units defending in that city an additional defensive bonus. Walls remain after capture.

## Technology Tree

- Agriculture: starting tech; unlocks Farm, Warrior, Settler
- Mining: requires Agriculture; unlocks Mine, Defender, City Walls
- Forestry: requires Agriculture; unlocks Sawmill, Lumber Camp
- Archery: requires Agriculture; unlocks Archer
- Sailing: requires Agriculture; unlocks Port, Sloop
- Writing: requires Agriculture; unlocks Study Hall
- Construction: requires Mining; unlocks Catapult
- Horseback Riding: requires Agriculture; unlocks Knight
- Shipbuilding: requires Sailing; unlocks Brig
- Education: requires Writing; unlocks Campus
- Stealth Tactics: requires Archery and Horseback Riding; unlocks Assassin
- Navigation: requires Shipbuilding; unlocks Galleon and Observatory
- Engineering: requires Construction and Education; unlocks Academy
- Administration: requires Writing and Mining; adds one city science slot

Technology names accept common separator forms such as `horseback riding`, `horseback_riding`, and `stealth-tactics`.

## Conquest And City Capture

Conquest is the only victory condition. The winner is the last non-eliminated civilization.

Siege rule:

- Move a unit onto an enemy or free city tile.
- End the turn.
- The defender gets a turn to kill or move the unit away.
- If the unit is still alive on the city tile when the attacker’s next turn begins, the city is captured.

Capital capture eliminates the old owner. Their units are removed, their non-capital cities become free cities, and their turns are skipped. Free cities can be captured by any active civilization.

## Scoring

The leaderboard includes all participating players. It tracks:

- Cities settled
- Cities conquered
- Units built and lost
- Enemy units destroyed
- Production points generated
- Science generated
- Technologies researched
- Buildings built
- Cities lost
- Turns survived

The winner is listed first, followed by eliminated players in inverse elimination order.

## Current Limitations

- No AI
- No diplomacy
- No fog of war
- No save/load
- No production queues
- No advanced graphics
- The console renderer prints the full map
- 5-player games reuse one of the four available colors

## Future Plans

- AI opponents
- Raylib renderer
- Save/load
- Fog of war and exploration
- More balance passes
- Richer city growth and production queues
