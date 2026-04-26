#include "render.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define COMMAND_BUFFER_SIZE 128

static char terrain_char(TileType type)
{
    switch (type) {
        case TILE_WATER:
            return '~';
        case TILE_PLAINS:
            return '.';
        case TILE_FOREST:
            return '*';
        case TILE_HILL:
            return '^';
        case TILE_MOUNTAIN:
            return 'M';
        default:
            return '?';
    }
}

static char building_char(TileBuilding building)
{
    switch (building) {
        case BUILDING_FARM:
            return 'f';
        case BUILDING_MINE:
            return 'm';
        case BUILDING_PORT:
            return 'p';
        case BUILDING_SAWMILL:
            return 's';
        case BUILDING_LUMBER_CAMP:
            return 'l';
        case BUILDING_NONE:
            return '\0';
    }

    return '\0';
}

static const char *unit_attack_status(const Unit *unit)
{
    if (unit->attack <= 0 || unit->range <= 0) {
        return "none";
    }

    return unit->attacks_remaining > 0 ? "ready" : "spent";
}

static char city_symbol(const City *city)
{
    if (city->owner_id == OWNER_FREE) {
        return 'F';
    }
    if (city->owner_id == 1) {
        return 'C';
    }
    if (city->owner_id == 2) {
        return 'K';
    }
    return '?';
}

void render_console_map(const Game *game)
{
    printf("\n");
    for (int y = 0; y < game->map.height; y++) {
        for (int x = 0; x < game->map.width; x++) {
            const Unit *unit = get_living_unit_at_const(game, x, y);
            const City *city = get_city_at_const(game, x, y);
            const Tile *tile = map_get_tile_const(&game->map, x, y);

            if (unit != NULL) {
                putchar(unit->symbol);
            } else if (city != NULL) {
                putchar(city_symbol(city));
            } else if (tile != NULL && tile->building != BUILDING_NONE) {
                putchar(building_char(tile->building));
            } else if (tile != NULL) {
                putchar(terrain_char(tile->type));
            } else {
                putchar('?');
            }
        }
        putchar('\n');
    }

    printf("\nLegend:\n");
    printf("  Terrain: ~=water .=plains *=forest ^=hill M=mountain\n");
    printf("  Buildings: f=Farm m=Mine p=Port s=Sawmill l=LumberCamp\n");
    printf("  Cities: C=Player 1 city K=Player 2 city\n");
    printf("  Free city: F\n");
    printf("  Player 1 units: W=Warrior A=Archer T=Catapult D=Defender N=Knight L=Settler X=Assassin P=Sloop B=Brig G=Galleon\n");
    printf("  Player 2 units: w=Warrior a=Archer t=Catapult d=Defender n=Knight l=Settler x=Assassin p=Sloop b=Brig g=Galleon\n");
    printf("  Commands: help map status economy tech research buildscience build upgrade train found leaderboard select move attack end quit\n");
}

void render_console_status(const Game *game)
{
    const Player *player = game_current_player_const(game);
    const Unit *selected_unit = game_selected_unit_const(game);

    printf("\nCurrent player: %s (%c)\n", player->name, player->symbol);
    printf("Points: %d income/turn=%d\n", player->points, player_get_income_per_turn(game, player->id));
    printf("Science: %d science/turn=%d\n", player->science, player_get_science_per_turn(game, player->id));
    if (player_current_research_is_valid(player)) {
        TechType tech = player->current_research;
        printf("Research: %s %d/%d\n",
            tech_type_to_string(tech),
            player->research_progress[tech],
            tech_get_cost(tech));
    } else {
        printf("Research: none\n");
    }
    printf("Cities: %d living units: %d\n",
        game_count_cities_for_player(game, player->id),
        game_count_living_units_for_player(game, player->id));
    if (selected_unit != NULL) {
        const Unit *unit = selected_unit;
        printf("Selected: %s at %d,%d hp=%d/%d move=%d/%d attack=%s\n",
            unit_type_to_string(unit->type),
            unit->x,
            unit->y,
            unit->hp,
            unit->max_hp,
            unit->movement_points,
            unit->max_movement_points,
            unit_attack_status(unit));
    } else {
        printf("Selected unit: none\n");
    }

    for (int i = 0; i < game->city_count; i++) {
        const City *city = &game->cities[i];
        if (city->owner_id == player->id) {
            printf("City: %s at %d,%d%s walls=%s",
                city->name,
                city->x,
                city->y,
                city->is_capital ? " capital" : "",
                city->has_walls ? "yes" : "no");
            if (city->siege_player_id != 0) {
                printf(" under siege by %s", get_player_name_safe(game, city->siege_player_id));
            }
            printf("\n");
        }
    }

    for (int p = 0; p < PLAYER_COUNT; p++) {
        printf("%s living units: %d\n",
            game->players[p].name,
            game_count_living_units_for_player(game, game->players[p].id));
        for (int i = 0; i < game->unit_count; i++) {
            const Unit *unit = &game->units[i];
            if (unit->alive && unit->owner_id == game->players[p].id) {
                printf("  %c %s at %d,%d hp=%d/%d move=%d/%d attack=%s\n",
                    unit->symbol,
                    unit_type_to_string(unit->type),
                    unit->x,
                    unit->y,
                    unit->hp,
                    unit->max_hp,
                    unit->movement_points,
                    unit->max_movement_points,
                    unit_attack_status(unit));
            }
        }
    }

    printf("Commands: tech research name buildscience type city_x city_y economy build type x y upgrade walls city_x city_y train unit city_x city_y found name select x y move dir attack x y end quit\n");
}

void render_console_help(void)
{
    printf("\nCommands:\n");
    printf("  help\n");
    printf("  map\n");
    printf("  status\n");
    printf("  economy\n");
    printf("  tech\n");
    printf("  research tech_name\n");
    printf("  buildscience studyhall|campus|academy|observatory city_x city_y\n");
    printf("  select x y\n");
    printf("  move north|south|east|west\n");
    printf("  attack x y\n");
    printf("  build farm|mine|port|sawmill|lumbercamp x y\n");
    printf("  upgrade walls city_x city_y\n");
    printf("  train warrior|archer|catapult|defender|knight|settler|assassin|sloop|brig|galleon city_x city_y\n");
    printf("  found city_name\n");
    printf("  spawn warrior|archer|catapult|defender|knight|settler|assassin|sloop|brig|galleon x y\n");
    printf("  leaderboard\n");
    printf("  end\n");
    printf("  quit\n");
    printf("Technology unlocks most units, buildings, city walls, and science buildings.\n");
}

static void trim_newline(char *text)
{
    size_t length = strlen(text);
    if (length > 0 && text[length - 1] == '\n') {
        text[length - 1] = '\0';
    }
}

static void to_lowercase(char *text)
{
    for (size_t i = 0; text[i] != '\0'; i++) {
        text[i] = (char)tolower((unsigned char)text[i]);
    }
}

static void normalize_lookup_text(char *text)
{
    size_t write_index = 0;

    to_lowercase(text);
    for (size_t read_index = 0; text[read_index] != '\0'; read_index++) {
        if (!isspace((unsigned char)text[read_index])) {
            text[write_index] = text[read_index];
            write_index++;
        }
    }
    text[write_index] = '\0';
}

static int parse_direction(const char *text, GameDirection *direction)
{
    if (strcmp(text, "north") == 0) {
        *direction = GAME_DIRECTION_NORTH;
    } else if (strcmp(text, "south") == 0) {
        *direction = GAME_DIRECTION_SOUTH;
    } else if (strcmp(text, "east") == 0) {
        *direction = GAME_DIRECTION_EAST;
    } else if (strcmp(text, "west") == 0) {
        *direction = GAME_DIRECTION_WEST;
    } else {
        return 0;
    }

    return 1;
}

static void print_turn_banner(const Game *game)
{
    const Player *player = game_current_player_const(game);
    printf("\n%s's turn.\n", player->name);
    printf("Gained %d points. Total points: %d.\n", game->last_points_gained, player->points);
    printf("Gained %d science. Stored science: %d.\n", game->last_science_gained, player->science);
    if (player_current_research_is_valid(player)) {
        TechType tech = player->current_research;
        printf("Research %s: %d/%d.\n",
            tech_type_to_string(tech),
            player->research_progress[tech],
            tech_get_cost(tech));
    } else {
        printf("Research: none selected.\n");
    }
    for (int i = 0; i < game->turn_message_count; i++) {
        printf("%s\n", game->turn_messages[i]);
    }
}

static void print_science_buildings(const City *city)
{
    if (city->science_building_count == 0) {
        printf("none");
        return;
    }

    for (int i = 0; i < city->science_building_count; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("%s", city_science_building_to_string(city->science_buildings[i]));
    }
}

static void render_console_economy(const Game *game)
{
    const Player *player = game_current_player_const(game);

    printf("\nEconomy for %s\n", player->name);
    printf("Points: %d\n", player->points);
    printf("Income per turn: %d\n", player_get_income_per_turn(game, player->id));
    printf("Science: %d\n", player->science);
    printf("Science per turn: %d\n", player_get_science_per_turn(game, player->id));
    printf("Owned cities: %d\n", game_count_cities_for_player(game, player->id));

    for (int i = 0; i < game->city_count; i++) {
        const City *city = &game->cities[i];
        if (city->owner_id == player->id) {
            printf("  %s at %d,%d income=%d border=%d growth=%d/%d\n",
                city->name,
                city->x,
                city->y,
                city_get_income(game, city),
                city->border_radius,
                city->border_growth_progress,
                city->border_radius >= 3 ? 0 : city->border_growth_needed);
            printf("     walls=%s%s\n",
                city->has_walls ? "yes" : "no",
                city->siege_player_id != 0 ? " under siege" : "");
            printf("     science=%d slots=%d/%d buildings=",
                city_get_science_per_turn(game, city),
                city->science_building_count,
                city_get_science_slots(game, city));
            print_science_buildings(city);
            printf("\n");
        }
    }
}

static void print_prerequisites(const Player *player, TechType tech)
{
    TechType prerequisites[3];
    int count = tech_get_prerequisites(tech, prerequisites, 3);

    if (count == 0) {
        printf("none");
        return;
    }

    for (int i = 0; i < count && i < 3; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("%s%s",
            tech_type_to_string(prerequisites[i]),
            player_has_tech(player, prerequisites[i]) ? "" : " missing");
    }
}

static void render_console_tech(const Game *game)
{
    const Player *player = game_current_player_const(game);

    printf("\nTechnology for %s\n", player->name);
    printf("Stored science: %d\n", player->science);
    printf("Science per turn: %d\n", player_get_science_per_turn(game, player->id));
    if (player_current_research_is_valid(player)) {
        TechType tech = player->current_research;
        printf("Current research: %s %d/%d\n",
            tech_type_to_string(tech),
            player->research_progress[tech],
            tech_get_cost(tech));
    } else {
        printf("Current research: none\n");
    }

    printf("\nUnlocked:\n");
    for (int tech = 0; tech < TECH_COUNT; tech++) {
        if (player_has_tech(player, (TechType)tech)) {
            printf("  %s - %s\n", tech_type_to_string((TechType)tech), tech_get_description((TechType)tech));
        }
    }

    printf("\nAvailable to research:\n");
    for (int tech = 0; tech < TECH_COUNT; tech++) {
        if (player_can_research_tech(player, (TechType)tech) && player->current_research != (TechType)tech) {
            printf("  %s (%d science) - %s\n",
                tech_type_to_string((TechType)tech),
                tech_get_cost((TechType)tech),
                tech_get_description((TechType)tech));
        }
    }

    printf("\nLocked:\n");
    for (int tech = 0; tech < TECH_COUNT; tech++) {
        TechType type = (TechType)tech;
        if (!player_has_tech(player, type) && !player_can_research_tech(player, type)) {
            printf("  %s requires ", tech_type_to_string(type));
            print_prerequisites(player, type);
            printf(".\n");
        }
    }
}

static void handle_select(Game *game, const char *arguments)
{
    int x;
    int y;
    char extra;
    GameActionResult result;

    if (sscanf(arguments, "%d %d %c", &x, &y, &extra) != 2) {
        printf("Usage: select x y\n");
        return;
    }

    result = game_select_unit_at(game, x, y);
    if (result == GAME_ACTION_OK) {
        const Unit *unit = game->selected_unit;
        printf("Selected %s at %d,%d hp=%d/%d move=%d/%d attack=%s.\n",
            unit_type_to_string(unit->type),
            x,
            y,
            unit->hp,
            unit->max_hp,
            unit->movement_points,
            unit->max_movement_points,
            unit_attack_status(unit));
    } else if (result == GAME_ACTION_INVALID_INPUT) {
        printf("Coordinates are outside the map.\n");
    } else {
        printf("No living current-player unit at %d,%d.\n", x, y);
    }
}

static void handle_move(Game *game, const char *arguments)
{
    char direction_text[16];
    GameDirection direction;
    GameActionResult result;
    char extra;

    if (sscanf(arguments, "%15s %c", direction_text, &extra) != 1) {
        printf("Usage: move north|south|east|west\n");
        return;
    }

    to_lowercase(direction_text);
    if (!parse_direction(direction_text, &direction)) {
        printf("Usage: move north|south|east|west\n");
        return;
    }

    result = game_move_selected_unit(game, direction);
    if (result == GAME_ACTION_OK) {
        printf("Moved to %d,%d.\n", game->selected_unit->x, game->selected_unit->y);
    } else if (result == GAME_ACTION_NO_SELECTION) {
        printf("Select one of your units first.\n");
    } else if (result == GAME_ACTION_NO_MOVEMENT) {
        printf("That unit has no movement points left.\n");
    } else if (result == GAME_ACTION_OCCUPIED) {
        printf("A friendly unit already occupies that tile.\n");
    } else if (result == GAME_ACTION_WRONG_TERRAIN) {
        printf("That unit cannot move onto that terrain.\n");
    } else {
        printf("Cannot move there. Enemy units block movement; use attack x y.\n");
    }
}

static void print_valid_unit_types(void)
{
    printf("Valid unit types: warrior, archer, catapult, defender, knight, settler, assassin, sloop, brig, galleon\n");
}

static void print_valid_building_types(void)
{
    printf("Valid building types: farm, mine, port, sawmill, lumbercamp\n");
}

static void print_valid_science_building_types(void)
{
    printf("Valid science buildings: studyhall, campus, academy, observatory\n");
}

static void print_valid_tech_types(void)
{
    printf("Valid techs: mining, forestry, archery, construction, horseback_riding, stealth_tactics, sailing, shipbuilding, navigation, writing, education, engineering, administration\n");
}

static void handle_research(Game *game, const char *arguments)
{
    char text[64];
    TechType tech;
    GameActionResult result;

    while (*arguments != '\0' && isspace((unsigned char)*arguments)) {
        arguments++;
    }
    if (*arguments == '\0') {
        printf("Usage: research tech_name\n");
        return;
    }

    snprintf(text, sizeof(text), "%s", arguments);
    normalize_lookup_text(text);
    if (!tech_type_from_string(text, &tech)) {
        printf("Unknown technology '%s'.\n", arguments);
        print_valid_tech_types();
        return;
    }

    result = game_set_research(game, tech);
    if (result == GAME_ACTION_OK) {
        const Player *player = game_current_player_const(game);
        printf("Research set to %s (%d/%d).\n",
            tech_type_to_string(tech),
            player->research_progress[tech],
            tech_get_cost(tech));
    } else if (result == GAME_ACTION_ALREADY_RESEARCHED) {
        printf("%s is already researched.\n", tech_type_to_string(tech));
    } else if (result == GAME_ACTION_MISSING_PREREQUISITE) {
        const Player *player = game_current_player_const(game);
        printf("Missing prerequisites for %s: ", tech_type_to_string(tech));
        print_prerequisites(player, tech);
        printf(".\n");
    } else {
        printf("Cannot research that technology.\n");
    }
}

static void handle_buildscience(Game *game, const char *arguments)
{
    char type_text[32];
    CityScienceBuilding building;
    int x;
    int y;
    char extra;
    GameActionResult result;

    if (sscanf(arguments, "%31s %d %d %c", type_text, &x, &y, &extra) != 3) {
        printf("Usage: buildscience studyhall|campus|academy|observatory city_x city_y\n");
        return;
    }

    normalize_lookup_text(type_text);
    if (!city_science_building_from_string(type_text, &building)) {
        printf("Unknown science building '%s'.\n", type_text);
        print_valid_science_building_types();
        return;
    }

    result = game_build_science_building(game, building, x, y);
    if (result == GAME_ACTION_OK) {
        printf("Built %s at %d,%d for %d points.\n",
            city_science_building_to_string(building),
            x,
            y,
            city_science_building_get_cost(building));
    } else if (result == GAME_ACTION_INVALID_INPUT) {
        printf("Coordinates are outside the map.\n");
    } else if (result == GAME_ACTION_NO_CITY) {
        printf("No current-player city at %d,%d.\n", x, y);
    } else if (result == GAME_ACTION_REQUIRES_TECH) {
        TechType required = city_science_building_required_tech(building);
        printf("You need %s to build %s.\n",
            tech_type_to_string(required),
            city_science_building_to_string(building));
    } else if (result == GAME_ACTION_ALREADY_HAS_BUILDING) {
        printf("That city already has %s.\n", city_science_building_to_string(building));
    } else if (result == GAME_ACTION_NO_SCIENCE_SLOT) {
        printf("That city has no open science building slots.\n");
    } else if (result == GAME_ACTION_NOT_ENOUGH_POINTS) {
        printf("Not enough points. %s costs %d points.\n",
            city_science_building_to_string(building),
            city_science_building_get_cost(building));
    } else {
        printf("Cannot build that science building.\n");
    }
}

static void handle_spawn(Game *game, const char *arguments)
{
    char type_text[24];
    UnitType type;
    Unit *unit = NULL;
    int x;
    int y;
    GameActionResult result;
    char extra;

    if (sscanf(arguments, "%23s %d %d %c", type_text, &x, &y, &extra) != 3) {
        printf("Usage: spawn warrior|archer|catapult|defender|knight|settler|assassin|sloop|brig|galleon x y\n");
        return;
    }

    to_lowercase(type_text);
    if (!unit_type_from_string(type_text, &type)) {
        printf("Unknown unit type '%s'.\n", type_text);
        print_valid_unit_types();
        return;
    }

    result = game_spawn_unit(game, type, x, y, &unit);
    if (result == GAME_ACTION_OK) {
        printf("Spawned %s at %d,%d.\n", unit_type_to_string(unit->type), unit->x, unit->y);
    } else if (result == GAME_ACTION_INVALID_INPUT) {
        printf("Coordinates are outside the map.\n");
    } else if (result == GAME_ACTION_OCCUPIED) {
        printf("Cannot spawn there: tile is occupied.\n");
    } else if (result == GAME_ACTION_WRONG_TERRAIN) {
        printf("Cannot spawn there: land units need plains, forest, or hills; ships need water; mountains are impassable.\n");
    } else if (result == GAME_ACTION_FULL) {
        printf("Cannot spawn: maximum unit count reached.\n");
    } else {
        printf("Cannot spawn there.\n");
    }
}

static void handle_build(Game *game, const char *arguments)
{
    char type_text[24];
    TileBuilding building;
    int x;
    int y;
    char extra;
    GameActionResult result;

    if (sscanf(arguments, "%23s %d %d %c", type_text, &x, &y, &extra) != 3) {
        printf("Usage: build farm|mine|port|sawmill|lumbercamp x y\n");
        return;
    }

    to_lowercase(type_text);
    if (!building_type_from_string(type_text, &building)) {
        printf("Unknown building type '%s'.\n", type_text);
        print_valid_building_types();
        return;
    }

    result = game_build_tile_building(game, building, x, y);
    if (result == GAME_ACTION_OK) {
        printf("Built %s at %d,%d for %d points.\n",
            building_type_to_string(building),
            x,
            y,
            building_get_cost(building));
    } else if (result == GAME_ACTION_INVALID_INPUT) {
        printf("Coordinates are outside the map.\n");
    } else if (result == GAME_ACTION_BLOCKED) {
        printf("Cannot build there: tile is outside your city borders.\n");
    } else if (result == GAME_ACTION_HAS_CITY) {
        printf("Cannot build there: a city occupies that tile.\n");
    } else if (result == GAME_ACTION_OCCUPIED) {
        printf("Cannot build there: tile is occupied or already has a building.\n");
    } else if (result == GAME_ACTION_WRONG_TERRAIN) {
        printf("Cannot build that building on this terrain.\n");
    } else if (result == GAME_ACTION_REQUIRES_TECH) {
        printf("You need %s to build %s.\n",
            tech_type_to_string(tech_unlocks_building(building)),
            building_type_to_string(building));
    } else if (result == GAME_ACTION_NOT_ENOUGH_POINTS) {
        printf("Not enough points. %s costs %d points.\n",
            building_type_to_string(building),
            building_get_cost(building));
    } else {
        printf("Cannot build there.\n");
    }
}

static void handle_train(Game *game, const char *arguments)
{
    char type_text[24];
    UnitType type;
    Unit *unit = NULL;
    int x;
    int y;
    char extra;
    GameActionResult result;

    if (sscanf(arguments, "%23s %d %d %c", type_text, &x, &y, &extra) != 3) {
        printf("Usage: train warrior|archer|catapult|defender|knight|settler|assassin|sloop|brig|galleon city_x city_y\n");
        return;
    }

    to_lowercase(type_text);
    if (!unit_type_from_string(type_text, &type)) {
        printf("Unknown unit type '%s'.\n", type_text);
        print_valid_unit_types();
        return;
    }

    result = game_train_unit(game, type, x, y, &unit);
    if (result == GAME_ACTION_OK) {
        printf("Trained %s at %d,%d for %d points.\n",
            unit_type_to_string(unit->type),
            unit->x,
            unit->y,
            unit_get_cost(type));
    } else if (result == GAME_ACTION_INVALID_INPUT) {
        printf("Coordinates are outside the map.\n");
    } else if (result == GAME_ACTION_NO_CITY) {
        printf("No current-player city at %d,%d.\n", x, y);
    } else if (result == GAME_ACTION_NOT_ENOUGH_POINTS) {
        printf("Not enough points. %s costs %d points.\n",
            unit_type_to_string(type),
            unit_get_cost(type));
    } else if (result == GAME_ACTION_REQUIRES_TECH) {
        printf("You need %s to train %s.\n",
            tech_type_to_string(tech_unlocks_unit(type)),
            unit_type_to_string(type));
    } else if (result == GAME_ACTION_NO_SPAWN_TILE) {
        printf("No valid spawn tile near that city.\n");
    } else if (result == GAME_ACTION_FULL) {
        printf("Cannot train: maximum unit count reached.\n");
    } else {
        printf("Cannot train that unit there.\n");
    }
}

static void handle_upgrade(Game *game, const char *arguments)
{
    char upgrade_text[24];
    int x;
    int y;
    char extra;
    GameActionResult result;

    if (sscanf(arguments, "%23s %d %d %c", upgrade_text, &x, &y, &extra) != 3) {
        printf("Usage: upgrade walls city_x city_y\n");
        return;
    }

    to_lowercase(upgrade_text);
    if (strcmp(upgrade_text, "walls") != 0) {
        printf("Unknown upgrade '%s'. Valid upgrade: walls\n", upgrade_text);
        return;
    }

    result = game_upgrade_city_walls(game, x, y);
    if (result == GAME_ACTION_OK) {
        printf("Built walls at %d,%d for 15 points.\n", x, y);
    } else if (result == GAME_ACTION_INVALID_INPUT) {
        printf("Coordinates are outside the map.\n");
    } else if (result == GAME_ACTION_NO_CITY) {
        printf("No current-player city at %d,%d.\n", x, y);
    } else if (result == GAME_ACTION_ALREADY_HAS_WALLS) {
        printf("That city already has walls.\n");
    } else if (result == GAME_ACTION_REQUIRES_TECH) {
        printf("You need Mining to upgrade City Walls.\n");
    } else if (result == GAME_ACTION_NOT_ENOUGH_POINTS) {
        printf("Not enough points. Walls cost 15 points.\n");
    } else {
        printf("Cannot upgrade that city.\n");
    }
}


static void handle_found(Game *game, const char *arguments)
{
    City *city = NULL;
    char name[CITY_NAME_LENGTH];
    GameActionResult result;

    while (*arguments != '\0' && isspace((unsigned char)*arguments)) {
        arguments++;
    }

    snprintf(name, sizeof(name), "%s", arguments);
    result = game_found_city(game, name, &city);
    if (result == GAME_ACTION_OK) {
        printf("Founded %s at %d,%d.\n", city->name, city->x, city->y);
    } else if (result == GAME_ACTION_NO_SELECTION) {
        printf("Select one of your living settlers first.\n");
    } else if (result == GAME_ACTION_NOT_SETTLER) {
        printf("Selected unit is not a settler.\n");
    } else if (result == GAME_ACTION_WRONG_TERRAIN) {
        printf("Settlers can only found cities on plains or hills.\n");
    } else if (result == GAME_ACTION_HAS_CITY) {
        printf("Cannot found there: a city already exists on that tile.\n");
    } else if (result == GAME_ACTION_TOO_CLOSE) {
        printf("Cannot found there: too close to another city.\n");
    } else if (result == GAME_ACTION_FULL) {
        printf("Cannot found: maximum city count reached.\n");
    } else {
        printf("Cannot found a city there.\n");
    }
}

static void handle_attack(Game *game, const char *arguments)
{
    int x;
    int y;
    char extra;
    CombatResult result;

    if (sscanf(arguments, "%d %d %c", &x, &y, &extra) != 2) {
        printf("Usage: attack x y\n");
        return;
    }

    result = game_attack_selected_unit(game, x, y);
    if (result.result == GAME_ACTION_OK) {
        printf("%s attacked %s for %d damage.",
            unit_type_to_string(result.attacker->type),
            unit_type_to_string(result.defender->type),
            result.damage);
        if (result.defender_destroyed) {
            printf(" Target destroyed.");
        } else {
            printf(" Target hp=%d/%d.", result.defender->hp, result.defender->max_hp);
        }
        printf("\n");

        if (result.counter_damage > 0) {
            printf("%s counterattacked for %d damage.",
                unit_type_to_string(result.defender->type),
                result.counter_damage);
            if (result.attacker_destroyed) {
                printf(" Attacker destroyed.");
            } else {
                printf(" Attacker hp=%d/%d.", result.attacker->hp, result.attacker->max_hp);
            }
            printf("\n");
        }
    } else if (result.result == GAME_ACTION_NO_SELECTION) {
        printf("Select one of your units first.\n");
    } else if (result.result == GAME_ACTION_INVALID_INPUT) {
        printf("Coordinates are outside the map.\n");
    } else if (result.result == GAME_ACTION_NO_ATTACKS) {
        printf("That unit has already attacked this turn.\n");
    } else if (result.result == GAME_ACTION_NO_UNIT) {
        printf("There is no enemy unit at %d,%d.\n", x, y);
    } else if (result.result == GAME_ACTION_FRIENDLY_UNIT) {
        printf("Cannot attack friendly units.\n");
    } else if (result.result == GAME_ACTION_OUT_OF_RANGE) {
        printf("Target is outside this unit's attack range.\n");
    } else if (result.result == GAME_ACTION_CANNOT_ATTACK) {
        printf("That unit cannot attack.\n");
    } else {
        printf("Cannot attack that target.\n");
    }
}

static int handle_command(Game *game, char *command)
{
    char verb[COMMAND_BUFFER_SIZE];
    char *arguments = command;

    while (*arguments != '\0' && isspace((unsigned char)*arguments)) {
        arguments++;
    }

    if (sscanf(arguments, "%127s", verb) != 1) {
        return 1;
    }

    arguments += strlen(verb);
    while (*arguments != '\0' && isspace((unsigned char)*arguments)) {
        arguments++;
    }

    to_lowercase(verb);

    if (strcmp(verb, "help") == 0) {
        render_console_help();
    } else if (strcmp(verb, "map") == 0) {
        render_console_map(game);
    } else if (strcmp(verb, "status") == 0) {
        render_console_status(game);
    } else if (strcmp(verb, "economy") == 0) {
        render_console_economy(game);
    } else if (strcmp(verb, "tech") == 0) {
        render_console_tech(game);
    } else if (strcmp(verb, "research") == 0) {
        handle_research(game, arguments);
    } else if (strcmp(verb, "buildscience") == 0) {
        handle_buildscience(game, arguments);
    } else if (strcmp(verb, "select") == 0) {
        handle_select(game, arguments);
    } else if (strcmp(verb, "move") == 0) {
        handle_move(game, arguments);
    } else if (strcmp(verb, "attack") == 0) {
        handle_attack(game, arguments);
    } else if (strcmp(verb, "spawn") == 0) {
        handle_spawn(game, arguments);
    } else if (strcmp(verb, "build") == 0) {
        handle_build(game, arguments);
    } else if (strcmp(verb, "train") == 0) {
        handle_train(game, arguments);
    } else if (strcmp(verb, "upgrade") == 0) {
        handle_upgrade(game, arguments);
    } else if (strcmp(verb, "found") == 0) {
        handle_found(game, arguments);
    } else if (strcmp(verb, "leaderboard") == 0) {
        game_print_leaderboard(game);
    } else if (strcmp(verb, "end") == 0) {
        game_end_turn(game);
        print_turn_banner(game);
        if (game->game_over) {
            game_print_leaderboard(game);
            return 0;
        }
    } else if (strcmp(verb, "quit") == 0) {
        return 0;
    } else {
        printf("Unknown command. Type help for commands.\n");
    }

    return 1;
}

void render_console_run(Game *game)
{
    char command[COMMAND_BUFFER_SIZE];
    int running = 1;

    printf("Seed: %u\n", game->seed);
    render_console_help();
    render_console_map(game);
    game_start_turn(game);
    print_turn_banner(game);

    while (running) {
        printf("> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }

        trim_newline(command);
        running = handle_command(game, command);
    }

    printf("Goodbye.\n");
}
