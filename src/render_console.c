#include "render.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define COMMAND_BUFFER_SIZE 128
#define ANSI_RESET "\x1b[0m"

typedef struct {
    int x;
    int y;
} Point;

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

static const char *terrain_type_to_string(TileType type)
{
    switch (type) {
        case TILE_WATER:
            return "water";
        case TILE_PLAINS:
            return "plains";
        case TILE_FOREST:
            return "forest";
        case TILE_HILL:
            return "hill";
        case TILE_MOUNTAIN:
            return "mountain";
    }

    return "unknown";
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

static int distance_manhattan_local(int ax, int ay, int bx, int by)
{
    int dx = ax - bx;
    int dy = ay - by;

    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }

    return dx + dy;
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
    if (city->owner_id > 0) {
        return 'C';
    }
    return '?';
}

static char unit_type_symbol(UnitType type)
{
    switch (type) {
        case UNIT_WARRIOR:
            return 'W';
        case UNIT_ARCHER:
            return 'A';
        case UNIT_CATAPULT:
            return 'T';
        case UNIT_DEFENDER:
            return 'D';
        case UNIT_KNIGHT:
            return 'N';
        case UNIT_SETTLER:
            return 'L';
        case UNIT_ASSASSIN:
            return 'X';
        case UNIT_SLOOP:
            return 'P';
        case UNIT_BRIG:
            return 'B';
        case UNIT_GALLEON:
            return 'G';
    }

    return '?';
}

static const Player *player_for_owner(const Game *game, int owner_id)
{
    if (game == NULL || owner_id < 1 || owner_id > game->player_count) {
        return NULL;
    }

    return &game->players[owner_id - 1];
}

static void put_colored_cell(const char *color, char symbol)
{
    printf("%s%c%s", color, symbol, ANSI_RESET);
}

static void render_cell(const Game *game, const RenderConfig *config, int x, int y)
{
    const Unit *unit = get_living_unit_at_const(game, x, y);
    const City *city = get_city_at_const(game, x, y);
    const City *controller = get_city_controlling_tile_const(game, x, y);
    const Tile *tile = map_get_tile_const(&game->map, x, y);
    int use_color = config == NULL || config->use_color;
    char symbol = '?';
    const Player *owner = NULL;

    if (unit != NULL) {
        owner = player_for_owner(game, unit->owner_id);
        symbol = use_color ? unit_type_symbol(unit->type) : unit->symbol;
        if (use_color && owner != NULL) {
            put_colored_cell(civ_color_to_ansi_light_bg(owner->color), symbol);
        } else {
            putchar(symbol);
        }
        return;
    }

    if (city != NULL) {
        symbol = city->owner_id == OWNER_FREE ? 'F' : 'C';
        owner = player_for_owner(game, city->owner_id);
        if (use_color && owner != NULL) {
            put_colored_cell(civ_color_to_ansi_light_bg(owner->color), symbol);
        } else if (use_color && city->owner_id == OWNER_FREE) {
            put_colored_cell("\x1b[37m", symbol);
        } else {
            putchar(use_color ? symbol : city_symbol(city));
        }
        return;
    }

    if (tile != NULL && tile->building != BUILDING_NONE) {
        symbol = building_char(tile->building);
        owner = controller != NULL ? player_for_owner(game, controller->owner_id) : NULL;
        if (use_color && owner != NULL) {
            put_colored_cell(civ_color_to_ansi_dark_bg(owner->color), symbol);
        } else {
            putchar(symbol);
        }
        return;
    }

    if (tile != NULL) {
        symbol = terrain_char(tile->type);
    }

    if (use_color && controller != NULL) {
        owner = player_for_owner(game, controller->owner_id);
        if (owner != NULL) {
            printf("%s%c%s", civ_color_to_ansi_bg(owner->color), symbol, ANSI_RESET);
            return;
        }
        if (controller->owner_id == OWNER_FREE) {
            printf("\x1b[2m%c%s", symbol, ANSI_RESET);
            return;
        }
    }

    putchar(symbol);
}

void render_console_map(const Game *game, const RenderConfig *config)
{
    printf("\n");
    for (int y = 0; y < game->map.height; y++) {
        for (int x = 0; x < game->map.width; x++) {
            render_cell(game, config, x, y);
        }
        putchar('\n');
    }

    printf("\nLegend:\n");
    printf("  Terrain: ~=water .=plains *=forest ^=hill M=mountain\n");
    printf("  Buildings: f=Farm m=Mine p=Port s=Sawmill l=LumberCamp\n");
    printf("  Cities: C=owned city F=free city\n");
    printf("  Units: W=Warrior A=Archer T=Catapult D=Defender N=Knight L=Settler X=Assassin P=Sloop B=Brig G=Galleon\n");
    if (config == NULL || config->use_color) {
        printf("  Color: bright unit/city=owner, colored background=city border, dim building=tile building\n");
        for (int i = 0; i < game->player_count; i++) {
            printf("  Player %d %s: %s%s\n",
                game->players[i].id,
                game->players[i].name,
                civ_color_to_string(game->players[i].color),
                game->players[i].is_eliminated ? " (eliminated)" : "");
        }
    } else {
        printf("  No-color mode: unit capitalization/symbol variants remain as fallback ownership hints.\n");
        for (int i = 0; i < game->player_count; i++) {
            printf("  Player %d %s: %s%s\n",
                game->players[i].id,
                game->players[i].name,
                civ_color_to_string(game->players[i].color),
                game->players[i].is_eliminated ? " (eliminated)" : "");
        }
    }
    printf("  Commands: help map status economy tech available todo buildoptions list cities inspect moves enemies attacks end quit\n");
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

    for (int p = 0; p < game->player_count; p++) {
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

    printf("Commands: available todo list mycities tech research name build type x y train unit city_x city_y select x y move dir attack x y end quit\n");
}

void render_console_help(const Game *game)
{
    printf("\nCommands:\n");
    printf("Core:\n");
    printf("  help | h\n");
    printf("  map | m\n");
    printf("  status | s\n");
    printf("  end | e\n");
    printf("  quit | q\n");
    printf("Selection and movement:\n");
    printf("  list | units | selectable\n");
    printf("  select x y\n");
    printf("  where\n");
    printf("  moves\n");
    printf("  move north|south|east|west\n");
    printf("Combat:\n");
    printf("  enemies\n");
    printf("  attacks\n");
    printf("  attack x y\n");
    printf("  Capture cities by holding a unit on the city tile until your next turn.\n");
    printf("Cities and economy:\n");
    printf("  cities | citylist\n");
    printf("  mycities\n");
    printf("  economy | eco\n");
    printf("  available [city_x city_y]\n");
    printf("  buildoptions city_x city_y\n");
    printf("  build farm|mine|port|sawmill|lumbercamp x y\n");
    printf("  buildscience studyhall|campus|academy|observatory city_x city_y\n");
    printf("  upgrade walls city_x city_y\n");
    printf("  train warrior|archer|catapult|defender|knight|settler|assassin|sloop|brig|galleon city_x city_y\n");
    printf("  found [city_name]\n");
    printf("Science:\n");
    printf("  tech | techs\n");
    printf("  researchable\n");
    printf("  research tech_name\n");
    printf("Info:\n");
    printf("  inspect x y\n");
    printf("  leaderboard\n");
    printf("  todo | actions | advice\n");
    if (game_is_debug_mode(game)) {
        printf("Debug commands:\n");
        printf("  spawn warrior|archer|catapult|defender|knight|settler|assassin|sloop|brig|galleon x y\n");
    }
    printf("Type available to see what you can do. Type list for units, cities for all cities, and mycities for your cities.\n");
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

static int unit_can_attack_now(const Unit *unit)
{
    return unit != NULL
        && unit->alive
        && unit->attack > 0
        && unit->range > 0
        && unit->attacks_remaining > 0;
}

static int player_count_ready_units(const Game *game, int player_id)
{
    int count = 0;

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].alive && game->units[i].owner_id == player_id && game->units[i].movement_points > 0) {
            count++;
        }
    }

    return count;
}

static int player_count_units_able_to_attack(const Game *game, int player_id)
{
    int count = 0;

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].owner_id == player_id && unit_can_attack_now(&game->units[i])) {
            count++;
        }
    }

    return count;
}

static int player_count_cities_under_siege(const Game *game, int player_id)
{
    int count = 0;

    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].owner_id == player_id && game->cities[i].siege_player_id != 0) {
            count++;
        }
    }

    return count;
}

static int player_has_any_researchable_tech(const Player *player)
{
    for (int tech = 0; tech < TECH_COUNT; tech++) {
        if (player_can_research_tech(player, (TechType)tech)) {
            return 1;
        }
    }

    return 0;
}

static int unit_spawn_tile_available(const Game *game, const City *city, UnitType type)
{
    int max_radius = unit_is_ship_type(type) ? 2 : 1;

    for (int radius = 1; radius <= max_radius; radius++) {
        for (int y = city->y - radius; y <= city->y + radius; y++) {
            for (int x = city->x - radius; x <= city->x + radius; x++) {
                Unit candidate;

                if (distance_manhattan_local(city->x, city->y, x, y) != radius) {
                    continue;
                }
                if (!map_in_bounds(&game->map, x, y) || is_tile_occupied_by_living_unit(game, x, y)) {
                    continue;
                }
                if (unit_is_ship_type(type) && !city_contains_tile(city, x, y)) {
                    continue;
                }

                unit_init(&candidate, type, city->owner_id, x, y);
                if (unit_can_move_to(&candidate, &game->map, x, y)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

static int player_can_afford_any_unit(const Game *game, const Player *player)
{
    for (int i = 0; i < game->city_count; i++) {
        const City *city = &game->cities[i];

        if (city->owner_id != player->id) {
            continue;
        }
        for (UnitType type = UNIT_WARRIOR; type <= UNIT_GALLEON; type++) {
            if (player->points >= unit_get_cost(type)
                && player_has_tech(player, tech_unlocks_unit(type))
                && unit_spawn_tile_available(game, city, type)) {
                return 1;
            }
        }
    }

    return 0;
}

static int city_has_available_science_slot(const Game *game, const City *city)
{
    return city != NULL && city->science_building_count < city_get_science_slots(game, city);
}

static int city_can_build_any_science_building_now(const Game *game, const Player *player, const City *city)
{
    if (!city_has_available_science_slot(game, city)) {
        return 0;
    }
    for (CityScienceBuilding building = CITY_SCIENCE_STUDY_HALL; building < CITY_SCIENCE_COUNT; building++) {
        if (city_can_build_science_building(game, player, city, building)
            && player->points >= city_science_building_get_cost(building)) {
            return 1;
        }
    }

    return 0;
}

static int player_count_cities_with_science_options(const Game *game, const Player *player)
{
    int count = 0;

    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].owner_id == player->id
            && city_can_build_any_science_building_now(game, player, &game->cities[i])) {
            count++;
        }
    }

    return count;
}

static int player_can_afford_city_walls(const Game *game, const Player *player)
{
    if (!player_has_tech(player, TECH_MINING) || player->points < 15) {
        return 0;
    }
    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].owner_id == player->id && !game->cities[i].has_walls) {
            return 1;
        }
    }

    return 0;
}

static void print_turn_banner(const Game *game)
{
    const Player *player = game_current_player_const(game);
    int city_count = game_count_cities_for_player(game, player->id);
    int under_siege = player_count_cities_under_siege(game, player->id);
    int unit_count = game_count_living_units_for_player(game, player->id);
    int ready_units = player_count_ready_units(game, player->id);
    int attack_units = player_count_units_able_to_attack(game, player->id);
    int warning_count = 0;

    printf("\n========================================\n");
    printf("Turn %d - %s\n", game->turn_number, player->name);
    printf("========================================\n");
    printf("Points: +%d this turn, total %d\n", game->last_points_gained, player->points);
    printf("Science: +%d this turn, stored %d\n", game->last_science_gained, player->science);
    if (player_current_research_is_valid(player)) {
        TechType tech = player->current_research;
        printf("Research: %s %d/%d\n",
            tech_type_to_string(tech),
            player->research_progress[tech],
            tech_get_cost(tech));
    } else {
        printf("Research: none selected\n");
    }
    printf("Cities: %d owned, %d under siege\n", city_count, under_siege);
    printf("Units: %d total, %d ready to move, %d able to attack\n", unit_count, ready_units, attack_units);

    for (int i = 0; i < game->turn_message_count; i++) {
        printf("%s\n", game->turn_messages[i]);
    }

    printf("Warnings:\n");
    for (int i = 0; i < game->city_count; i++) {
        const City *city = &game->cities[i];
        if (city->owner_id == player->id && city->siege_player_id != 0) {
            printf("- %s is under siege by %s.\n", city->name, get_player_name_safe(game, city->siege_player_id));
            warning_count++;
        }
    }
    if (!player_current_research_is_valid(player) && player_has_any_researchable_tech(player)) {
        printf("- Research is not selected.\n");
        warning_count++;
    }
    if (player_count_cities_with_science_options(game, player) > 0) {
        printf("- A city can build a science building.\n");
        warning_count++;
    }
    if (player_can_afford_city_walls(game, player)) {
        printf("- A city can build walls.\n");
        warning_count++;
    }
    if (player_can_afford_any_unit(game, player)) {
        printf("- You can train at least one unit.\n");
        warning_count++;
    }
    if (warning_count == 0) {
        printf("- none\n");
    }
    printf("Suggested commands: available, todo, list, mycities, researchable, map, help\n");
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

static int count_tile_buildings_for_city(const Game *game, const City *city)
{
    int count = 0;

    for (int y = city->y - city->border_radius; y <= city->y + city->border_radius; y++) {
        for (int x = city->x - city->border_radius; x <= city->x + city->border_radius; x++) {
            const Tile *tile;
            const City *controller;

            if (!map_in_bounds(&game->map, x, y) || !city_contains_tile(city, x, y)) {
                continue;
            }
            controller = get_city_controlling_tile_const(game, x, y);
            if (controller != city) {
                continue;
            }
            tile = map_get_tile_const(&game->map, x, y);
            if (tile != NULL && tile->building != BUILDING_NONE) {
                count++;
            }
        }
    }

    return count;
}

static void print_city_summary(const Game *game, const City *city, int index)
{
    printf("[%d] %s at (%d, %d)\n", index, city->name, city->x, city->y);
    printf("    Owner: %s\n", get_player_name_safe(game, city->owner_id));
    printf("    Capital: %s\n", city->is_capital ? "yes" : "no");
    printf("    Walls: %s\n", city->has_walls ? "yes" : "no");
    printf("    Border radius: %d\n", city->border_radius);
    printf("    Point income: %d\n", city_has_valid_owner(city) ? city_get_income(game, city) : 0);
    printf("    Science income: %d\n", city_get_science_per_turn(game, city));
    printf("    Science slots: %d/%d\n", city->science_building_count, city_get_science_slots(game, city));
    printf("    Tile buildings: %d\n", count_tile_buildings_for_city(game, city));
    printf("    Science buildings: ");
    print_science_buildings(city);
    printf("\n");
    if (city->siege_player_id != 0) {
        printf("    Under siege: by %s\n", get_player_name_safe(game, city->siege_player_id));
    } else {
        printf("    Under siege: no\n");
    }
}

static void render_console_cities(const Game *game)
{
    printf("\nAll cities:\n");
    if (game->city_count == 0) {
        printf("No cities exist.\n");
        return;
    }

    for (int i = 0; i < game->city_count; i++) {
        print_city_summary(game, &game->cities[i], i);
    }
}

static void render_console_mycities(const Game *game)
{
    const Player *player = game_current_player_const(game);
    int found = 0;

    printf("\nCities for %s:\n", player->name);
    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].owner_id == player->id) {
            print_city_summary(game, &game->cities[i], i);
            found = 1;
        }
    }

    if (!found) {
        printf("No owned cities.\n");
    }
}

static void render_console_selectable(const Game *game)
{
    const Player *player = game_current_player_const(game);
    int found = 0;

    printf("\nSelectable units for %s:\n", player->name);
    if (player->is_eliminated) {
        printf("This civilization is eliminated.\n");
        return;
    }

    for (int i = 0; i < game->unit_count; i++) {
        const Unit *unit = &game->units[i];
        const Tile *tile;
        const City *city;
        const City *siege_city = NULL;

        if (!unit->alive || unit->owner_id != player->id) {
            continue;
        }

        tile = map_get_tile_const(&game->map, unit->x, unit->y);
        city = get_city_at_const(game, unit->x, unit->y);
        for (int c = 0; c < game->city_count; c++) {
            if (game->cities[c].siege_unit_index == i && game->cities[c].siege_player_id == player->id) {
                siege_city = &game->cities[c];
            }
        }

        printf("[%d] %s at (%d, %d), HP %d/%d, MP %d/%d, %s, tile: %s",
            i,
            unit_type_to_string(unit->type),
            unit->x,
            unit->y,
            unit->hp,
            unit->max_hp,
            unit->movement_points,
            unit->max_movement_points,
            unit_attack_status(unit),
            tile != NULL ? terrain_type_to_string(tile->type) : "unknown");
        if (city != NULL) {
            printf(", in city: %s", city->name);
        }
        if (siege_city != NULL) {
            printf(", sieging: %s", siege_city->name);
        }
        printf("\n");
        found = 1;
    }

    if (!found) {
        printf("No selectable units.\n");
    }
    printf("Usage: select x y\n");
}

static void render_console_where(const Game *game)
{
    const Unit *unit = game_selected_unit_const(game);
    const Tile *tile;

    if (unit == NULL) {
        printf("No unit selected.\n");
        return;
    }

    tile = map_get_tile_const(&game->map, unit->x, unit->y);
    printf("Selected %s at (%d, %d), HP %d/%d, MP %d/%d, attack=%s, tile=%s\n",
        unit_type_to_string(unit->type),
        unit->x,
        unit->y,
        unit->hp,
        unit->max_hp,
        unit->movement_points,
        unit->max_movement_points,
        unit_attack_status(unit),
        tile != NULL ? terrain_type_to_string(tile->type) : "unknown");
    printf("Use moves to inspect legal movement.\n");
}

static int tile_defense_bonus(const Game *game, int x, int y, int owner_id)
{
    const Tile *tile = map_get_tile_const(&game->map, x, y);
    const City *city = get_city_at_const(game, x, y);
    int bonus = 0;

    if (tile != NULL && (tile->type == TILE_HILL || tile->type == TILE_FOREST)) {
        bonus++;
    }
    if (city != NULL && city->owner_id == owner_id && city_has_valid_owner(city)) {
        bonus += 2;
        if (city->has_walls) {
            bonus += 2;
        }
    }

    return bonus;
}

static int current_player_can_build_any_tile_building(const Game *game, const Player *player, int x, int y)
{
    const Tile *tile = map_get_tile_const(&game->map, x, y);
    const City *city = get_city_at_const(game, x, y);
    const Unit *unit = get_living_unit_at_const(game, x, y);

    if (tile == NULL
        || tile->building != BUILDING_NONE
        || city != NULL
        || unit != NULL
        || !player_controls_tile(game, player->id, x, y)) {
        return 0;
    }

    for (TileBuilding building = BUILDING_FARM; building <= BUILDING_LUMBER_CAMP; building++) {
        if (building_can_be_built_on_tile(building, tile->type)
            && player_has_tech(player, tech_unlocks_building(building))) {
            return 1;
        }
    }

    return 0;
}

static void render_console_inspect(const Game *game, const char *arguments)
{
    int x;
    int y;
    char extra;
    const Tile *tile;
    const City *controller;
    const City *city;
    const Unit *unit;
    const Player *player = game_current_player_const(game);
    const Unit *selected = game_selected_unit_const(game);

    if (sscanf(arguments, "%d %d %c", &x, &y, &extra) != 2) {
        printf("Usage: inspect x y\n");
        return;
    }
    if (!map_in_bounds(&game->map, x, y)) {
        printf("Coordinates are outside the map.\n");
        return;
    }

    tile = map_get_tile_const(&game->map, x, y);
    controller = get_city_controlling_tile_const(game, x, y);
    city = get_city_at_const(game, x, y);
    unit = get_living_unit_at_const(game, x, y);

    printf("Tile (%d, %d)\n", x, y);
    printf("  Terrain: %s\n", tile != NULL ? terrain_type_to_string(tile->type) : "unknown");
    printf("  Controlled by: %s\n", controller != NULL ? get_player_name_safe(game, controller->owner_id) : "none");
    printf("  City: %s\n", city != NULL ? city->name : "none");
    printf("  Unit: %s\n", unit != NULL ? unit_type_to_string(unit->type) : "none");
    printf("  Building: %s\n", tile != NULL ? building_type_to_string(tile->building) : "None");
    printf("  Defensive bonus for current player: +%d\n", tile_defense_bonus(game, x, y, player->id));
    printf("  Current player can build a tile building here: %s\n",
        current_player_can_build_any_tile_building(game, player, x, y) ? "yes" : "no");
    if (selected != NULL) {
        printf("  Passable for selected unit: %s\n", unit_can_move_to(selected, &game->map, x, y) ? "yes" : "no");
    }
}

static void print_move_probe(const Game *game, const Unit *unit, const char *name, int dx, int dy)
{
    int x = unit->x + dx;
    int y = unit->y + dy;
    const Tile *tile;
    const Unit *occupant;

    printf("%s -> (%d, %d): ", name, x, y);
    if (!map_in_bounds(&game->map, x, y)) {
        printf("outside map\n");
        return;
    }
    occupant = get_living_unit_at_const(game, x, y);
    if (occupant != NULL && occupant->owner_id == unit->owner_id) {
        printf("occupied by friendly unit\n");
        return;
    }
    if (occupant != NULL) {
        printf("blocked by enemy unit\n");
        return;
    }
    tile = map_get_tile_const(&game->map, x, y);
    if (tile == NULL || !unit_can_move_to(unit, &game->map, x, y)) {
        printf("%s not passable for this unit\n", tile != NULL ? terrain_type_to_string(tile->type) : "unknown terrain");
        return;
    }
    printf("legal\n");
}

static void render_console_moves(const Game *game)
{
    const Unit *unit = game_selected_unit_const(game);

    if (unit == NULL) {
        printf("Select one of your units first.\n");
        return;
    }
    if (unit->movement_points <= 0) {
        printf("Selected unit has no movement points left.\n");
    }

    print_move_probe(game, unit, "north", 0, -1);
    print_move_probe(game, unit, "south", 0, 1);
    print_move_probe(game, unit, "east", 1, 0);
    print_move_probe(game, unit, "west", -1, 0);
}

static void render_console_enemies(const Game *game)
{
    const Player *player = game_current_player_const(game);
    int found = 0;

    printf("\nVisible enemy units:\n");
    for (int i = 0; i < game->unit_count; i++) {
        const Unit *unit = &game->units[i];
        const Tile *tile;

        if (!unit->alive || unit->owner_id == player->id) {
            continue;
        }

        tile = map_get_tile_const(&game->map, unit->x, unit->y);
        printf("[%d] %s %s at (%d, %d), HP %d/%d, tile: %s\n",
            i,
            get_player_name_safe(game, unit->owner_id),
            unit_type_to_string(unit->type),
            unit->x,
            unit->y,
            unit->hp,
            unit->max_hp,
            tile != NULL ? terrain_type_to_string(tile->type) : "unknown");
        found = 1;
    }

    if (!found) {
        printf("No visible enemy units.\n");
    }
}

static void render_console_attacks(const Game *game)
{
    const Unit *selected = game_selected_unit_const(game);
    int found = 0;

    if (selected == NULL) {
        printf("Select one of your units first.\n");
        return;
    }
    if (selected->attack <= 0 || selected->range <= 0 || selected->attacks_remaining <= 0) {
        printf("Selected unit cannot attack right now.\n");
        return;
    }

    printf("\nTargets in range for %s at (%d, %d):\n",
        unit_type_to_string(selected->type),
        selected->x,
        selected->y);
    for (int i = 0; i < game->unit_count; i++) {
        const Unit *unit = &game->units[i];
        int distance;

        if (!unit->alive || unit->owner_id == selected->owner_id) {
            continue;
        }
        distance = distance_manhattan_local(selected->x, selected->y, unit->x, unit->y);
        if (distance <= selected->range) {
            printf("  unit at (%d, %d): %s %s HP %d/%d distance %d\n",
                unit->x,
                unit->y,
                get_player_name_safe(game, unit->owner_id),
                unit_type_to_string(unit->type),
                unit->hp,
                unit->max_hp,
                distance);
            found = 1;
        }
    }

    for (int i = 0; i < game->city_count; i++) {
        const City *city = &game->cities[i];
        int distance = distance_manhattan_local(selected->x, selected->y, city->x, city->y);
        if (city->owner_id != selected->owner_id && distance == 1) {
            printf("  city at (%d, %d): %s owned by %s, enter to siege, distance %d\n",
                city->x,
                city->y,
                city->name,
                get_player_name_safe(game, city->owner_id),
                distance);
            found = 1;
        }
    }

    if (!found) {
        printf("No attack or siege targets in range.\n");
    }
}

static void render_console_researchable(const Game *game)
{
    const Player *player = game_current_player_const(game);
    int found = 0;

    printf("\nResearchable technologies for %s:\n", player->name);
    for (int tech = 0; tech < TECH_COUNT; tech++) {
        if (player_can_research_tech(player, (TechType)tech) && player->current_research != (TechType)tech) {
            printf("  %s (%d science) - %s\n",
                tech_type_to_string((TechType)tech),
                tech_get_cost((TechType)tech),
                tech_get_description((TechType)tech));
            found = 1;
        }
    }

    if (!found) {
        printf("No new technologies are currently researchable.\n");
    }
}

static int tile_building_options_for_city(const Game *game, const Player *player, const City *city, TileBuilding building, Point examples[], int max_examples)
{
    int count = 0;

    for (int y = city->y - city->border_radius; y <= city->y + city->border_radius; y++) {
        for (int x = city->x - city->border_radius; x <= city->x + city->border_radius; x++) {
            const Tile *tile;
            const City *controller;

            if (!map_in_bounds(&game->map, x, y) || !city_contains_tile(city, x, y)) {
                continue;
            }
            controller = get_city_controlling_tile_const(game, x, y);
            if (controller != city || get_city_at_const(game, x, y) != NULL || is_tile_occupied_by_living_unit(game, x, y)) {
                continue;
            }
            tile = map_get_tile_const(&game->map, x, y);
            if (tile == NULL || tile->building != BUILDING_NONE) {
                continue;
            }
            if (!building_can_be_built_on_tile(building, tile->type)
                || !player_has_tech(player, tech_unlocks_building(building))
                || player->points < building_get_cost(building)) {
                continue;
            }
            if (count < max_examples) {
                examples[count].x = x;
                examples[count].y = y;
            }
            count++;
        }
    }

    return count;
}

static void print_examples(const Point examples[], int count, int max_examples)
{
    int shown = count < max_examples ? count : max_examples;

    for (int i = 0; i < shown; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("(%d,%d)", examples[i].x, examples[i].y);
    }
}

static void print_city_available_actions(const Game *game, const Player *player, const City *city)
{
    int printed;

    printf("%s at (%d,%d)\n", city->name, city->x, city->y);
    printf("  Train: ");
    printed = 0;
    for (UnitType type = UNIT_WARRIOR; type <= UNIT_GALLEON; type++) {
        if (player_has_tech(player, tech_unlocks_unit(type))
            && player->points >= unit_get_cost(type)
            && unit_spawn_tile_available(game, city, type)) {
            printf("%s%s", printed ? ", " : "", unit_type_to_string(type));
            printed = 1;
        }
    }
    printf("%s\n", printed ? "" : "none");

    printf("  Upgrades: ");
    if (!city->has_walls && player_has_tech(player, TECH_MINING) && player->points >= 15) {
        printf("walls");
    } else {
        printf("none");
    }
    printf("\n");

    printf("  Science buildings: ");
    printed = 0;
    for (CityScienceBuilding building = CITY_SCIENCE_STUDY_HALL; building < CITY_SCIENCE_COUNT; building++) {
        if (city_can_build_science_building(game, player, city, building)
            && player->points >= city_science_building_get_cost(building)) {
            printf("%s%s", printed ? ", " : "", city_science_building_to_string(building));
            printed = 1;
        }
    }
    printf("%s\n", printed ? "" : "none");

    printf("  Tile buildings:\n");
    for (TileBuilding building = BUILDING_FARM; building <= BUILDING_LUMBER_CAMP; building++) {
        Point examples[3];
        int count = tile_building_options_for_city(game, player, city, building, examples, 3);
        if (count > 0) {
            printf("    %s: %d tiles, examples: ", building_type_to_string(building), count);
            print_examples(examples, count, 3);
            printf("\n");
        }
    }
}

static void render_console_available_for_city(const Game *game, const City *city)
{
    const Player *player = game_current_player_const(game);

    if (city == NULL || city->owner_id != player->id) {
        printf("No current-player city at those coordinates.\n");
        return;
    }

    printf("\nAvailable actions for %s\n", player->name);
    printf("Points: %d\n", player->points);
    print_city_available_actions(game, player, city);
}

static void render_console_available(const Game *game, const char *arguments)
{
    const Player *player = game_current_player_const(game);
    int x;
    int y;
    char extra;
    int city_found = 0;

    while (*arguments != '\0' && isspace((unsigned char)*arguments)) {
        arguments++;
    }
    if (*arguments != '\0') {
        if (sscanf(arguments, "%d %d %c", &x, &y, &extra) != 2) {
            printf("Usage: available [city_x city_y]\n");
            return;
        }
        if (!map_in_bounds(&game->map, x, y)) {
            printf("Coordinates are outside the map.\n");
            return;
        }
        render_console_available_for_city(game, get_city_at_const(game, x, y));
        return;
    }

    printf("\nAvailable actions for %s\n", player->name);
    printf("Points: %d, science: %d\n", player->points, player->science);
    printf("Research: ");
    if (player_current_research_is_valid(player)) {
        TechType tech = player->current_research;
        printf("%s %d/%d\n", tech_type_to_string(tech), player->research_progress[tech], tech_get_cost(tech));
    } else if (player_has_any_researchable_tech(player)) {
        printf("choose one with researchable\n");
    } else {
        printf("none available\n");
    }

    printf("\nCities:\n");
    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].owner_id == player->id) {
            print_city_available_actions(game, player, &game->cities[i]);
            city_found = 1;
        }
    }
    if (!city_found) {
        printf("No owned cities.\n");
    }

    printf("\nUnits:\n");
    printf("  Ready to move: %d\n", player_count_ready_units(game, player->id));
    printf("  Able to attack: %d\n", player_count_units_able_to_attack(game, player->id));
    printf("  Settlers able to found: ");
    int settlers = 0;
    for (int i = 0; i < game->unit_count; i++) {
        const Unit *unit = &game->units[i];
        const Tile *tile = map_get_tile_const(&game->map, unit->x, unit->y);
        int too_close = 0;
        for (int c = 0; c < game->city_count; c++) {
            if (distance_manhattan_local(unit->x, unit->y, game->cities[c].x, game->cities[c].y) < 4) {
                too_close = 1;
            }
        }
        if (unit->alive
            && unit->owner_id == player->id
            && unit->type == UNIT_SETTLER
            && tile != NULL
            && tile_can_host_city(tile->type)
            && get_city_at_const(game, unit->x, unit->y) == NULL
            && !too_close) {
            printf("%s(%d,%d)", settlers ? ", " : "", unit->x, unit->y);
            settlers++;
        }
    }
    printf("%s\n", settlers ? "" : "none");
}

static void render_console_todo(const Game *game)
{
    const Player *player = game_current_player_const(game);
    int ready = player_count_ready_units(game, player->id);
    int attacks = player_count_units_able_to_attack(game, player->id);
    int science_cities = player_count_cities_with_science_options(game, player);
    int siege_count = player_count_cities_under_siege(game, player->id);
    int city_actions = science_cities > 0
        || player_can_afford_any_unit(game, player)
        || player_can_afford_city_walls(game, player);

    printf("\nTodo for %s:\n", player->name);
    if (!player_current_research_is_valid(player) && player_has_any_researchable_tech(player)) {
        printf("[!] Choose research: use researchable, then research <tech>\n");
    }
    if (ready > 0) {
        printf("[ ] Move %d ready units: use list, select x y, moves\n", ready);
    }
    if (attacks > 0) {
        printf("[ ] Attack with %d units: use attacks\n", attacks);
    }
    if (city_actions) {
        printf("[ ] City actions available: use available or mycities\n");
    }
    for (int i = 0; i < game->city_count; i++) {
        const City *city = &game->cities[i];
        if (city->owner_id == player->id && city->siege_player_id != 0) {
            printf("[!] %s is under siege.\n", city->name);
        }
    }
    if (ready == 0 && attacks == 0 && !city_actions && siege_count == 0
        && (player_current_research_is_valid(player) || !player_has_any_researchable_tech(player))) {
        printf("[ ] No urgent actions found.\n");
    }
}

static void print_end_turn_summary(const Game *game)
{
    const Player *player = game_current_player_const(game);
    int ready = player_count_ready_units(game, player->id);
    int attacks = player_count_units_able_to_attack(game, player->id);
    int science_cities = player_count_cities_with_science_options(game, player);
    int siege_count = player_count_cities_under_siege(game, player->id);

    printf("\nEnd turn summary for %s:\n", player->name);
    if (ready > 0) {
        printf("- %d units still have movement points.\n", ready);
    }
    if (attacks > 0) {
        printf("- %d units can still attack.\n", attacks);
    }
    if (science_cities > 0) {
        printf("- %d cities can build a science building.\n", science_cities);
    }
    if (siege_count > 0) {
        printf("- %d cities are under siege.\n", siege_count);
    }
    if (!player_current_research_is_valid(player) && player_has_any_researchable_tech(player)) {
        printf("- Research is not selected.\n");
    }
    printf("End turn? Passing to next civilization.\n");
}

static void handle_buildoptions(Game *game, const char *arguments)
{
    int x;
    int y;
    char extra;

    if (sscanf(arguments, "%d %d %c", &x, &y, &extra) != 2) {
        printf("Usage: buildoptions city_x city_y\n");
        return;
    }
    if (!map_in_bounds(&game->map, x, y)) {
        printf("Coordinates are outside the map.\n");
        return;
    }

    render_console_available_for_city(game, get_city_at_const(game, x, y));
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
    for (size_t i = strlen(name); i > 0 && isspace((unsigned char)name[i - 1]); i--) {
        name[i - 1] = '\0';
    }
    result = game_found_city(game, name, &city);
    if (result == GAME_ACTION_OK) {
        printf("Founded %s at %d,%d.\n", city->name, city->x, city->y);
    } else if (result == GAME_ACTION_NO_SELECTION) {
        printf("Select one of your living settlers first.\n");
    } else if (result == GAME_ACTION_NOT_SETTLER) {
        printf("Selected unit is not a settler.\n");
    } else if (result == GAME_ACTION_WRONG_TERRAIN) {
        printf("Settlers can only found cities on plains, forests, or hills.\n");
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

static int handle_command(Game *game, char *command, const RenderConfig *config)
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

    if (strcmp(verb, "help") == 0 || strcmp(verb, "h") == 0) {
        render_console_help(game);
    } else if (strcmp(verb, "map") == 0 || strcmp(verb, "m") == 0) {
        render_console_map(game, config);
    } else if (strcmp(verb, "status") == 0 || strcmp(verb, "s") == 0) {
        render_console_status(game);
    } else if (strcmp(verb, "economy") == 0 || strcmp(verb, "eco") == 0) {
        render_console_economy(game);
    } else if (strcmp(verb, "tech") == 0 || strcmp(verb, "techs") == 0) {
        render_console_tech(game);
    } else if (strcmp(verb, "researchable") == 0) {
        render_console_researchable(game);
    } else if (strcmp(verb, "available") == 0) {
        render_console_available(game, arguments);
    } else if (strcmp(verb, "todo") == 0 || strcmp(verb, "actions") == 0 || strcmp(verb, "advice") == 0) {
        render_console_todo(game);
    } else if (strcmp(verb, "buildoptions") == 0) {
        handle_buildoptions(game, arguments);
    } else if (strcmp(verb, "research") == 0) {
        handle_research(game, arguments);
    } else if (strcmp(verb, "buildscience") == 0) {
        handle_buildscience(game, arguments);
    } else if (strcmp(verb, "list") == 0 || strcmp(verb, "units") == 0 || strcmp(verb, "selectable") == 0) {
        render_console_selectable(game);
    } else if (strcmp(verb, "cities") == 0 || strcmp(verb, "citylist") == 0) {
        render_console_cities(game);
    } else if (strcmp(verb, "mycities") == 0) {
        render_console_mycities(game);
    } else if (strcmp(verb, "where") == 0) {
        render_console_where(game);
    } else if (strcmp(verb, "inspect") == 0) {
        render_console_inspect(game, arguments);
    } else if (strcmp(verb, "moves") == 0) {
        render_console_moves(game);
    } else if (strcmp(verb, "enemies") == 0) {
        render_console_enemies(game);
    } else if (strcmp(verb, "attacks") == 0) {
        render_console_attacks(game);
    } else if (strcmp(verb, "select") == 0) {
        handle_select(game, arguments);
    } else if (strcmp(verb, "move") == 0) {
        handle_move(game, arguments);
    } else if (strcmp(verb, "attack") == 0) {
        handle_attack(game, arguments);
    } else if (strcmp(verb, "spawn") == 0) {
        if (!game_is_debug_mode(game)) {
            printf("This command is only available in debug mode.\n");
        } else {
            handle_spawn(game, arguments);
        }
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
    } else if (strcmp(verb, "end") == 0 || strcmp(verb, "e") == 0) {
        print_end_turn_summary(game);
        game_end_turn(game);
        if (game->game_over) {
            for (int i = 0; i < game->turn_message_count; i++) {
                printf("%s\n", game->turn_messages[i]);
            }
            game_print_leaderboard(game);
            return 0;
        }
        print_turn_banner(game);
    } else if (strcmp(verb, "quit") == 0 || strcmp(verb, "q") == 0) {
        return 0;
    } else {
        printf("Unknown command. Type help for commands.\n");
    }

    return 1;
}

void render_console_run(Game *game, RenderConfig config)
{
    char command[COMMAND_BUFFER_SIZE];
    int running = 1;

    render_console_help(game);
    render_console_map(game, &config);
    game_start_turn(game);
    print_turn_banner(game);

    while (running) {
        printf("> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }

        trim_newline(command);
        running = handle_command(game, command, &config);
    }

    if (config.use_color) {
        printf(ANSI_RESET);
    }
    printf("Goodbye.\n");
}
