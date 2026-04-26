#include "game.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int x;
    int y;
} Point;

Player *game_current_player(Game *game)
{
    return &game->players[game->current_player_index];
}

const Player *game_current_player_const(const Game *game)
{
    return &game->players[game->current_player_index];
}

static int distance_squared(int ax, int ay, int bx, int by)
{
    int dx = ax - bx;
    int dy = ay - by;
    return dx * dx + dy * dy;
}

static int distance_manhattan(int ax, int ay, int bx, int by)
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

static City *add_city(Game *game, int owner_id, int x, int y, const char *name)
{
    City *city;

    if (game->city_count >= MAX_CITIES) {
        return NULL;
    }
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    city = &game->cities[game->city_count];
    city_init(city, owner_id, x, y, name);
    game->city_count++;

    if (owner_id >= 1 && owner_id <= PLAYER_COUNT && game_count_cities_for_player(game, owner_id) == 1) {
        city->is_capital = 1;
        game->players[owner_id - 1].starting_city = *city;
        game->players[owner_id - 1].capital_x = x;
        game->players[owner_id - 1].capital_y = y;
    }

    return city;
}

int player_is_active(const Game *game, int player_id)
{
    return game != NULL
        && player_id >= 1
        && player_id <= PLAYER_COUNT
        && !game->players[player_id - 1].is_eliminated;
}

const char *get_player_name_safe(const Game *game, int player_id)
{
    if (game == NULL || player_id == OWNER_FREE) {
        return "Free Cities";
    }
    if (player_id < 1 || player_id > PLAYER_COUNT) {
        return "Unknown";
    }
    return game->players[player_id - 1].name;
}

static int find_start_city(const Map *map, const Point *avoid, Point *out)
{
    int best_score = -1;
    Point best = {0, 0};
    int margin = 3;

    if (map->width <= margin * 2 || map->height <= margin * 2) {
        return 0;
    }

    for (int attempt = 0; attempt < 3000; attempt++) {
        int x = margin + rand() % (map->width - margin * 2);
        int y = margin + rand() % (map->height - margin * 2);

        if (!map_is_land_passable(map, x, y)) {
            continue;
        }

        int score = 1;
        if (avoid != NULL) {
            score = distance_squared(x, y, avoid->x, avoid->y);
        }

        if (score > best_score) {
            best_score = score;
            best.x = x;
            best.y = y;
        }
    }

    if (best_score < 0) {
        return 0;
    }

    *out = best;
    return 1;
}

static int find_adjacent_unit_tile(const Map *map, const Point *city, Point *out)
{
    static const Point directions[] = {
        {0, -1},
        {1, 0},
        {0, 1},
        {-1, 0},
        {1, -1},
        {1, 1},
        {-1, 1},
        {-1, -1}
    };

    for (size_t i = 0; i < sizeof(directions) / sizeof(directions[0]); i++) {
        int x = city->x + directions[i].x;
        int y = city->y + directions[i].y;
        if (map_is_land_passable(map, x, y)) {
            out->x = x;
            out->y = y;
            return 1;
        }
    }

    return 0;
}

static void ensure_start_patch(Map *map, const Point *point)
{
    map_set_tile(map, point->x, point->y, TILE_PLAINS);
    map_set_tile(map, point->x + 1, point->y, TILE_PLAINS);
    map_set_tile(map, point->x - 1, point->y, TILE_PLAINS);
    map_set_tile(map, point->x, point->y + 1, TILE_PLAINS);
    map_set_tile(map, point->x, point->y - 1, TILE_PLAINS);
}

static void fallback_start_points(const Map *map, Point *city_a, Point *city_b)
{
    city_a->x = map->width / 4;
    city_a->y = map->height / 4;
    city_b->x = (map->width * 3) / 4;
    city_b->y = (map->height * 3) / 4;

    if (city_a->x < 3) {
        city_a->x = 3;
    }
    if (city_a->y < 3) {
        city_a->y = 3;
    }
    if (city_b->x > map->width - 4) {
        city_b->x = map->width - 4;
    }
    if (city_b->y > map->height - 4) {
        city_b->y = map->height - 4;
    }
}

static int place_starting_assets(Game *game)
{
    Point city_a;
    Point city_b;
    Point unit_a;
    Point unit_b;
    int min_dim = game->map.width < game->map.height ? game->map.width : game->map.height;
    int min_distance_squared = (min_dim / 3) * (min_dim / 3);
    int used_fallback = 0;

    if (!find_start_city(&game->map, NULL, &city_a)) {
        fallback_start_points(&game->map, &city_a, &city_b);
        ensure_start_patch(&game->map, &city_a);
        ensure_start_patch(&game->map, &city_b);
        used_fallback = 1;
    }

    if (!used_fallback
        && (!find_start_city(&game->map, &city_a, &city_b)
            || distance_squared(city_a.x, city_a.y, city_b.x, city_b.y) < min_distance_squared)) {
        return 0;
    }

    if (!find_adjacent_unit_tile(&game->map, &city_a, &unit_a)) {
        unit_a.x = city_a.x + 1;
        unit_a.y = city_a.y;
        ensure_start_patch(&game->map, &city_a);
    }

    if (!find_adjacent_unit_tile(&game->map, &city_b, &unit_b)) {
        unit_b.x = city_b.x - 1;
        unit_b.y = city_b.y;
        ensure_start_patch(&game->map, &city_b);
    }

    if (!map_is_land_passable(&game->map, city_a.x, city_a.y)
        || !map_is_land_passable(&game->map, city_b.x, city_b.y)
        || !map_is_land_passable(&game->map, unit_a.x, unit_a.y)
        || !map_is_land_passable(&game->map, unit_b.x, unit_b.y)) {
        return 0;
    }

    if (add_city(game, 1, city_a.x, city_a.y, "Capital") == NULL
        || add_city(game, 2, city_b.x, city_b.y, "Keep") == NULL) {
        return 0;
    }
    (void)game_spawn_unit(game, UNIT_WARRIOR, unit_a.x, unit_a.y, NULL);

    game->current_player_index = 1;
    (void)game_spawn_unit(game, UNIT_WARRIOR, unit_b.x, unit_b.y, NULL);
    game->current_player_index = 0;

    return game->unit_count == 2;
}

static void direction_to_delta(GameDirection direction, int *dx, int *dy)
{
    *dx = 0;
    *dy = 0;

    switch (direction) {
        case GAME_DIRECTION_NORTH:
            *dy = -1;
            break;
        case GAME_DIRECTION_SOUTH:
            *dy = 1;
            break;
        case GAME_DIRECTION_EAST:
            *dx = 1;
            break;
        case GAME_DIRECTION_WEST:
            *dx = -1;
            break;
    }
}

int game_init(Game *game, unsigned int seed, int map_width, int map_height)
{
    srand(seed);
    game->seed = seed;
    game->unit_count = 0;
    game->city_count = 0;
    game->current_player_index = 0;
    game->selected_unit = NULL;
    game->map.width = 0;
    game->map.height = 0;
    game->map.tiles = NULL;
    game->turn_number = 1;
    game->game_over = 0;
    game->winner_player_id = 0;
    game->last_points_gained = 0;
    game->last_science_gained = 0;
    game->turn_message_count = 0;

    if (!map_init(&game->map, map_width, map_height, TILE_WATER)) {
        return 0;
    }

    player_init(&game->players[0], 1, "Player 1", '1');
    player_init(&game->players[1], 2, "Player 2", '2');

    for (int attempt = 0; attempt < 20; attempt++) {
        game->unit_count = 0;
        game->city_count = 0;
        game->selected_unit = NULL;
        game->current_player_index = 0;
        map_generate_archipelago(&game->map);
        if (place_starting_assets(game)) {
            return 1;
        }
    }

    game->unit_count = 0;
    game->city_count = 0;
    game->selected_unit = NULL;
    game->current_player_index = 0;
    for (int y = 0; y < game->map.height; y++) {
        for (int x = 0; x < game->map.width; x++) {
            map_set_tile(&game->map, x, y, TILE_WATER);
        }
    }
    if (!place_starting_assets(game)) {
        map_free(&game->map);
        return 0;
    }

    return 1;
}

void game_free(Game *game)
{
    map_free(&game->map);
    game->unit_count = 0;
    game->city_count = 0;
    game->selected_unit = NULL;
}

static int active_player_count(const Game *game)
{
    int count = 0;

    for (int i = 0; i < PLAYER_COUNT; i++) {
        if (!game->players[i].is_eliminated) {
            count++;
        }
    }

    return count;
}

static void add_turn_message(Game *game, const char *message)
{
    if (game->turn_message_count >= MAX_TURN_MESSAGES) {
        return;
    }

    snprintf(game->turn_messages[game->turn_message_count], TURN_MESSAGE_LENGTH, "%s", message);
    game->turn_message_count++;
}

static void check_conquest_victory(Game *game)
{
    int winner_id = 0;

    if (game->game_over) {
        return;
    }

    for (int i = 0; i < PLAYER_COUNT; i++) {
        if (!game->players[i].is_eliminated) {
            winner_id = game->players[i].id;
        }
    }

    if (active_player_count(game) == 1 && winner_id > 0) {
        game->game_over = 1;
        game->winner_player_id = winner_id;
        game->players[winner_id - 1].is_winner = 1;
    }
}

static void eliminate_player(Game *game, int eliminated_id, int by_player_id)
{
    Player *player;
    char message[TURN_MESSAGE_LENGTH];

    if (!player_is_active(game, eliminated_id)) {
        return;
    }

    player = &game->players[eliminated_id - 1];
    player->is_eliminated = 1;
    player->elimination_turn = game->turn_number;
    player->eliminated_by_player_id = by_player_id;

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].alive && game->units[i].owner_id == eliminated_id) {
            game->units[i].alive = 0;
            game->units[i].hp = 0;
            game->units[i].x = -1;
            game->units[i].y = -1;
        }
    }

    for (int i = 0; i < game->city_count; i++) {
        City *city = &game->cities[i];
        if (city->owner_id == eliminated_id && !city->is_capital) {
            city->owner_id = OWNER_FREE;
            city_clear_siege(city);
        }
    }

    snprintf(message, sizeof(message), "The civilization of %s has been eliminated by %s!",
        get_player_name_safe(game, eliminated_id),
        get_player_name_safe(game, by_player_id));
    add_turn_message(game, message);
    check_conquest_victory(game);
}

static void capture_city(Game *game, City *city, int capturing_player_id)
{
    int old_owner = city->owner_id;
    char message[TURN_MESSAGE_LENGTH];

    if (!player_is_active(game, capturing_player_id) || old_owner == capturing_player_id) {
        city_clear_siege(city);
        return;
    }

    if (old_owner >= 1 && old_owner <= PLAYER_COUNT) {
        game->players[old_owner - 1].cities_lost++;
    }

    city->owner_id = capturing_player_id;
    city->border_growth_progress = 0;
    city_clear_siege(city);
    game->players[capturing_player_id - 1].cities_conquered++;

    snprintf(message, sizeof(message), "%s captured %s!",
        get_player_name_safe(game, capturing_player_id),
        city->name);
    add_turn_message(game, message);

    if (old_owner >= 1
        && old_owner <= PLAYER_COUNT
        && game->players[old_owner - 1].capital_x == city->x
        && game->players[old_owner - 1].capital_y == city->y) {
        eliminate_player(game, old_owner, capturing_player_id);
    }
}

static void check_siege_captures_for_player(Game *game, int player_id)
{
    for (int i = 0; i < game->city_count; i++) {
        City *city = &game->cities[i];
        Unit *unit;

        if (city->siege_player_id == 0) {
            continue;
        }
        if (!player_is_active(game, city->siege_player_id)) {
            city_clear_siege(city);
            continue;
        }
        if (city->siege_unit_index < 0 || city->siege_unit_index >= game->unit_count) {
            city_clear_siege(city);
            continue;
        }

        unit = &game->units[city->siege_unit_index];
        if (!unit->alive || unit->owner_id != city->siege_player_id || unit->x != city->x || unit->y != city->y) {
            city_clear_siege(city);
            continue;
        }

        if (city->siege_player_id == player_id && city->owner_id != player_id) {
            capture_city(game, city, player_id);
        }
    }
}

static void mark_sieges_for_player(Game *game, int player_id)
{
    for (int i = 0; i < game->unit_count; i++) {
        Unit *unit = &game->units[i];
        City *city;
        char message[TURN_MESSAGE_LENGTH];

        if (!unit->alive || unit->owner_id != player_id) {
            continue;
        }

        city = get_city_at(game, unit->x, unit->y);
        if (city == NULL || city->owner_id == player_id) {
            continue;
        }

        city->siege_player_id = player_id;
        city->siege_unit_index = i;
        city->siege_started_turn = game->turn_number;
        snprintf(message, sizeof(message), "%s is under siege by %s!",
            city->name,
            get_player_name_safe(game, player_id));
        add_turn_message(game, message);
    }
}

void game_start_turn(Game *game)
{
    Player *player = game_current_player(game);

    if (game->game_over) {
        return;
    }

    game->last_points_gained = 0;
    game->last_science_gained = 0;
    check_siege_captures_for_player(game, player->id);
    check_conquest_victory(game);
    if (game->game_over || player->is_eliminated) {
        return;
    }

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].owner_id == player->id) {
            unit_reset_turn(&game->units[i]);
        }
    }

    (void)player_add_turn_income(game, player);
    (void)player_add_science_progress(game, player);
    game->selected_unit = NULL;
}

void game_end_turn(Game *game)
{
    int checked = 0;
    Player *player = game_current_player(game);

    if (game->game_over) {
        return;
    }

    game->turn_message_count = 0;
    if (!player->is_eliminated) {
        mark_sieges_for_player(game, player->id);
        player->turns_survived++;
    }

    do {
        game->current_player_index = (game->current_player_index + 1) % PLAYER_COUNT;
        checked++;
    } while (checked <= PLAYER_COUNT && game_current_player(game)->is_eliminated);

    if (checked > PLAYER_COUNT) {
        check_conquest_victory(game);
        return;
    }

    game->turn_number++;
    game_start_turn(game);
}

static int selected_unit_is_current_player_living(const Game *game)
{
    const Player *player = game_current_player_const(game);

    return game->selected_unit != NULL
        && game->selected_unit->alive
        && game->selected_unit->owner_id == player->id;
}

Unit *get_unit_at(Game *game, int x, int y)
{
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].alive && game->units[i].x == x && game->units[i].y == y) {
            return &game->units[i];
        }
    }

    return NULL;
}

const Unit *get_unit_at_const(const Game *game, int x, int y)
{
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].alive && game->units[i].x == x && game->units[i].y == y) {
            return &game->units[i];
        }
    }

    return NULL;
}

Unit *get_living_unit_at(Game *game, int x, int y)
{
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].alive && game->units[i].x == x && game->units[i].y == y) {
            return &game->units[i];
        }
    }

    return NULL;
}

const Unit *get_living_unit_at_const(const Game *game, int x, int y)
{
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].alive && game->units[i].x == x && game->units[i].y == y) {
            return &game->units[i];
        }
    }

    return NULL;
}

int is_tile_occupied_by_living_unit(const Game *game, int x, int y)
{
    return get_living_unit_at_const(game, x, y) != NULL;
}

City *get_city_at(Game *game, int x, int y)
{
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].x == x && game->cities[i].y == y) {
            return &game->cities[i];
        }
    }

    return NULL;
}

const City *get_city_at_const(const Game *game, int x, int y)
{
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].x == x && game->cities[i].y == y) {
            return &game->cities[i];
        }
    }

    return NULL;
}

City *get_city_controlling_tile(Game *game, int x, int y)
{
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    for (int i = 0; i < game->city_count; i++) {
        if (city_contains_tile(&game->cities[i], x, y)) {
            return &game->cities[i];
        }
    }

    return NULL;
}

const City *get_city_controlling_tile_const(const Game *game, int x, int y)
{
    if (!map_in_bounds(&game->map, x, y)) {
        return NULL;
    }

    for (int i = 0; i < game->city_count; i++) {
        if (city_contains_tile(&game->cities[i], x, y)) {
            return &game->cities[i];
        }
    }

    return NULL;
}

int player_controls_tile(const Game *game, int owner_id, int x, int y)
{
    const City *city = get_city_controlling_tile_const(game, x, y);
    return city != NULL && city->owner_id == owner_id;
}

int tile_is_inside_any_city_border(const Game *game, int x, int y)
{
    return get_city_controlling_tile_const(game, x, y) != NULL;
}

int game_count_living_units_for_player(const Game *game, int owner_id)
{
    int count = 0;

    for (int i = 0; i < game->unit_count; i++) {
        if (game->units[i].alive && game->units[i].owner_id == owner_id) {
            count++;
        }
    }

    return count;
}

int game_count_cities_for_player(const Game *game, int owner_id)
{
    int count = 0;

    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].owner_id == owner_id) {
            count++;
        }
    }

    return count;
}

int city_get_income(const Game *game, const City *city)
{
    int income = city->base_points_per_turn;

    for (int y = city->y - city->border_radius; y <= city->y + city->border_radius; y++) {
        for (int x = city->x - city->border_radius; x <= city->x + city->border_radius; x++) {
            const Tile *tile;
            const City *controller;

            if (!map_in_bounds(&game->map, x, y) || !city_contains_tile(city, x, y)) {
                continue;
            }

            controller = get_city_controlling_tile_const(game, x, y);
            if (controller == NULL
                || controller->owner_id != city->owner_id
                || controller->x != city->x
                || controller->y != city->y) {
                continue;
            }

            tile = map_get_tile_const(&game->map, x, y);
            if (tile != NULL) {
                income += building_get_income_bonus(tile->building);
            }
        }
    }

    return income;
}

int player_get_income_per_turn(const Game *game, int owner_id)
{
    int income = 0;

    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].owner_id == owner_id) {
            income += city_get_income(game, &game->cities[i]);
        }
    }

    return income;
}

int city_get_science_slots(const Game *game, const City *city)
{
    int slots;

    if (city == NULL) {
        return 0;
    }

    slots = city->border_radius;
    if (city_has_valid_owner(city)
        && game != NULL
        && city->owner_id <= PLAYER_COUNT
        && player_has_tech(&game->players[city->owner_id - 1], TECH_ADMINISTRATION)) {
        slots++;
    }

    if (slots > CITY_SCIENCE_COUNT) {
        slots = CITY_SCIENCE_COUNT;
    }

    return slots;
}

int city_get_science_per_turn(const Game *game, const City *city)
{
    int science = 1;

    (void)game;
    if (city == NULL || !city_has_valid_owner(city)) {
        return 0;
    }

    for (int i = 0; i < city->science_building_count; i++) {
        science += city_science_building_get_science(city->science_buildings[i]);
    }

    return science;
}

int player_get_science_per_turn(const Game *game, int owner_id)
{
    int science = 0;

    if (game == NULL || owner_id < 1 || owner_id > PLAYER_COUNT || game->players[owner_id - 1].is_eliminated) {
        return 0;
    }

    for (int i = 0; i < game->city_count; i++) {
        if (game->cities[i].owner_id == owner_id) {
            science += city_get_science_per_turn(game, &game->cities[i]);
        }
    }

    return science;
}

int player_has_tech(const Player *player, TechType tech)
{
    return player != NULL && tech >= 0 && tech < TECH_COUNT && player->unlocked_techs[tech];
}

int player_can_research_tech(const Player *player, TechType tech)
{
    TechType prerequisites[3];
    int prerequisite_count;

    if (player == NULL || tech < 0 || tech >= TECH_COUNT || player_has_tech(player, tech)) {
        return 0;
    }

    prerequisite_count = tech_get_prerequisites(tech, prerequisites, 3);
    for (int i = 0; i < prerequisite_count && i < 3; i++) {
        if (!player_has_tech(player, prerequisites[i])) {
            return 0;
        }
    }

    return 1;
}

void player_unlock_tech(Player *player, TechType tech)
{
    if (player == NULL || tech < 0 || tech >= TECH_COUNT || player->unlocked_techs[tech]) {
        return;
    }

    player->unlocked_techs[tech] = 1;
    player->technologies_researched++;
}

int player_current_research_is_valid(const Player *player)
{
    return player != NULL
        && player->current_research >= 0
        && player->current_research < TECH_COUNT
        && !player_has_tech(player, player->current_research);
}

static void add_research_complete_message(Game *game, TechType tech)
{
    char message[TURN_MESSAGE_LENGTH];

    snprintf(message, sizeof(message), "Research complete: %s!", tech_type_to_string(tech));
    add_turn_message(game, message);
    snprintf(message, sizeof(message), "Unlocked: %s", tech_get_description(tech));
    add_turn_message(game, message);
}

int player_add_science_progress(Game *game, Player *player)
{
    int gained;

    if (game == NULL || player == NULL || player->is_eliminated) {
        return 0;
    }

    gained = player_get_science_per_turn(game, player->id);
    player->science += gained;
    player->science_generated_total += gained;
    game->last_science_gained = gained;

    if (player_current_research_is_valid(player) && player->science > 0) {
        TechType tech = player->current_research;
        int cost = tech_get_cost(tech);
        int needed = cost - player->research_progress[tech];
        int applied = player->science < needed ? player->science : needed;

        if (applied > 0) {
            player->science -= applied;
            player->research_progress[tech] += applied;
        }

        if (player->research_progress[tech] >= cost) {
            player_unlock_tech(player, tech);
            player->current_research = TECH_NONE;
            add_research_complete_message(game, tech);
        }
    } else if (player->current_research != TECH_NONE && !player_current_research_is_valid(player)) {
        player->current_research = TECH_NONE;
    }

    return gained;
}

TechType tech_unlocks_unit(UnitType type)
{
    switch (type) {
        case UNIT_WARRIOR:
        case UNIT_SETTLER:
            return TECH_AGRICULTURE;
        case UNIT_DEFENDER:
            return TECH_MINING;
        case UNIT_ARCHER:
            return TECH_ARCHERY;
        case UNIT_CATAPULT:
            return TECH_CONSTRUCTION;
        case UNIT_KNIGHT:
            return TECH_HORSEBACK_RIDING;
        case UNIT_ASSASSIN:
            return TECH_STEALTH_TACTICS;
        case UNIT_SLOOP:
            return TECH_SAILING;
        case UNIT_BRIG:
            return TECH_SHIPBUILDING;
        case UNIT_GALLEON:
            return TECH_NAVIGATION;
    }

    return TECH_NONE;
}

TechType tech_unlocks_building(TileBuilding building)
{
    switch (building) {
        case BUILDING_FARM:
            return TECH_AGRICULTURE;
        case BUILDING_MINE:
            return TECH_MINING;
        case BUILDING_SAWMILL:
        case BUILDING_LUMBER_CAMP:
            return TECH_FORESTRY;
        case BUILDING_PORT:
            return TECH_SAILING;
        case BUILDING_NONE:
            return TECH_NONE;
    }

    return TECH_NONE;
}

TechType tech_unlocks_city_upgrade(const char *upgrade)
{
    if (upgrade != NULL && strcmp(upgrade, "walls") == 0) {
        return TECH_MINING;
    }

    return TECH_NONE;
}

int city_can_build_science_building(const Game *game, const Player *player, const City *city, CityScienceBuilding building)
{
    TechType required_tech;

    if (game == NULL || player == NULL || city == NULL || building < 0 || building >= CITY_SCIENCE_COUNT) {
        return 0;
    }
    if (city->owner_id != player->id || player->is_eliminated) {
        return 0;
    }
    required_tech = city_science_building_required_tech(building);
    if (!player_has_tech(player, required_tech)) {
        return 0;
    }
    if (city_has_science_building(city, building)) {
        return 0;
    }

    return city->science_building_count < city_get_science_slots(game, city);
}

GameActionResult game_set_research(Game *game, TechType tech)
{
    Player *player = game_current_player(game);

    if (game->game_over || player->is_eliminated) {
        return GAME_ACTION_GAME_OVER;
    }
    if (tech < 0 || tech >= TECH_COUNT) {
        return GAME_ACTION_INVALID_INPUT;
    }
    if (player_has_tech(player, tech)) {
        return GAME_ACTION_ALREADY_RESEARCHED;
    }
    if (!player_can_research_tech(player, tech)) {
        return GAME_ACTION_MISSING_PREREQUISITE;
    }

    player->current_research = tech;
    return GAME_ACTION_OK;
}

GameActionResult game_build_science_building(Game *game, CityScienceBuilding building, int city_x, int city_y)
{
    Player *player = game_current_player(game);
    City *city;
    int cost;
    TechType required_tech;

    if (building < 0 || building >= CITY_SCIENCE_COUNT || !map_in_bounds(&game->map, city_x, city_y)) {
        return GAME_ACTION_INVALID_INPUT;
    }

    city = get_city_at(game, city_x, city_y);
    if (city == NULL || city->owner_id != player->id) {
        return GAME_ACTION_NO_CITY;
    }

    required_tech = city_science_building_required_tech(building);
    if (!player_has_tech(player, required_tech)) {
        return GAME_ACTION_REQUIRES_TECH;
    }
    if (city_has_science_building(city, building)) {
        return GAME_ACTION_ALREADY_HAS_BUILDING;
    }
    if (city->science_building_count >= city_get_science_slots(game, city)) {
        return GAME_ACTION_NO_SCIENCE_SLOT;
    }

    cost = city_science_building_get_cost(building);
    if (player->points < cost) {
        return GAME_ACTION_NOT_ENOUGH_POINTS;
    }
    if (!city_add_science_building(city, building)) {
        return GAME_ACTION_FULL;
    }

    player->points -= cost;
    player->buildings_built++;
    return GAME_ACTION_OK;
}

int city_add_border_growth(City *city)
{
    if (city->border_radius >= 3) {
        return 0;
    }

    city->border_growth_progress += city->border_growth_per_turn;
    if (city->border_growth_progress < city->border_growth_needed) {
        return 0;
    }

    city->border_growth_progress -= city->border_growth_needed;
    city->border_radius++;
    city->border_growth_needed = city->border_radius == 2 ? 10 : city->border_growth_needed;
    if (city->border_radius >= 3) {
        city->border_growth_progress = 0;
    }

    return 1;
}

int player_add_turn_income(Game *game, Player *player)
{
    int gained = 0;

    if (player == NULL || player->is_eliminated) {
        return 0;
    }

    for (int i = 0; i < game->city_count; i++) {
        City *city = &game->cities[i];

        if (city->owner_id != player->id) {
            continue;
        }

        gained += city_get_income(game, city);
        if (city_add_border_growth(city)) {
            char message[TURN_MESSAGE_LENGTH];
            snprintf(message, sizeof(message), "%s borders expanded to radius %d.", city->name, city->border_radius);
            add_turn_message(game, message);
        }
    }

    player->points += gained;
    player->production_points_generated += gained;
    game->last_points_gained = gained;
    return gained;
}

const Unit *game_selected_unit_const(const Game *game)
{
    if (!selected_unit_is_current_player_living(game)) {
        return NULL;
    }

    return game->selected_unit;
}

GameActionResult game_build_tile_building(Game *game, TileBuilding building, int x, int y)
{
    Player *player = game_current_player(game);
    Tile *tile;
    int cost = building_get_cost(building);

    if (!map_in_bounds(&game->map, x, y)) {
        return GAME_ACTION_INVALID_INPUT;
    }

    tile = map_get_tile(&game->map, x, y);
    if (tile == NULL) {
        return GAME_ACTION_INVALID_INPUT;
    }
    if (!player_has_tech(player, tech_unlocks_building(building))) {
        return GAME_ACTION_REQUIRES_TECH;
    }
    if (!player_controls_tile(game, player->id, x, y)) {
        return GAME_ACTION_BLOCKED;
    }
    if (get_city_at(game, x, y) != NULL) {
        return GAME_ACTION_HAS_CITY;
    }
    if (is_tile_occupied_by_living_unit(game, x, y)) {
        return GAME_ACTION_OCCUPIED;
    }
    if (tile->building != BUILDING_NONE) {
        return GAME_ACTION_OCCUPIED;
    }
    if (!building_can_be_built_on_tile(building, tile->type)) {
        return GAME_ACTION_WRONG_TERRAIN;
    }
    if (player->points < cost) {
        return GAME_ACTION_NOT_ENOUGH_POINTS;
    }

    player->points -= cost;
    player->buildings_built++;
    tile->building = building;
    return GAME_ACTION_OK;
}

GameActionResult game_upgrade_city_walls(Game *game, int city_x, int city_y)
{
    Player *player = game_current_player(game);
    City *city;
    const int cost = 15;

    if (!map_in_bounds(&game->map, city_x, city_y)) {
        return GAME_ACTION_INVALID_INPUT;
    }

    city = get_city_at(game, city_x, city_y);
    if (city == NULL || city->owner_id != player->id) {
        return GAME_ACTION_NO_CITY;
    }
    if (!player_has_tech(player, tech_unlocks_city_upgrade("walls"))) {
        return GAME_ACTION_REQUIRES_TECH;
    }
    if (city->has_walls) {
        return GAME_ACTION_ALREADY_HAS_WALLS;
    }
    if (player->points < cost) {
        return GAME_ACTION_NOT_ENOUGH_POINTS;
    }

    player->points -= cost;
    city->has_walls = 1;
    return GAME_ACTION_OK;
}

static int find_unit_spawn_tile(Game *game, const City *city, UnitType type, Point *out)
{
    int max_radius = unit_is_ship_type(type) ? 2 : 1;

    for (int radius = 1; radius <= max_radius; radius++) {
        for (int y = city->y - radius; y <= city->y + radius; y++) {
            for (int x = city->x - radius; x <= city->x + radius; x++) {
                Unit candidate;

                if (distance_manhattan(city->x, city->y, x, y) != radius) {
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
                    out->x = x;
                    out->y = y;
                    return 1;
                }
            }
        }
    }

    return 0;
}

GameActionResult game_train_unit(Game *game, UnitType type, int city_x, int city_y, Unit **created_unit)
{
    Player *player = game_current_player(game);
    City *city;
    Point spawn;
    int cost = unit_get_cost(type);
    int old_player_index = game->current_player_index;
    GameActionResult result;

    if (created_unit != NULL) {
        *created_unit = NULL;
    }
    if (!map_in_bounds(&game->map, city_x, city_y)) {
        return GAME_ACTION_INVALID_INPUT;
    }
    if (game->unit_count >= MAX_UNITS) {
        return GAME_ACTION_FULL;
    }
    if (!player_has_tech(player, tech_unlocks_unit(type))) {
        return GAME_ACTION_REQUIRES_TECH;
    }

    city = get_city_at(game, city_x, city_y);
    if (city == NULL || city->owner_id != player->id) {
        return GAME_ACTION_NO_CITY;
    }
    if (player->points < cost) {
        return GAME_ACTION_NOT_ENOUGH_POINTS;
    }
    if (!find_unit_spawn_tile(game, city, type, &spawn)) {
        return GAME_ACTION_NO_SPAWN_TILE;
    }

    player->points -= cost;
    game->current_player_index = player->id - 1;
    result = game_spawn_unit(game, type, spawn.x, spawn.y, created_unit);
    game->current_player_index = old_player_index;

    if (result != GAME_ACTION_OK) {
        player->points += cost;
    } else {
        player->units_built++;
    }

    return result;
}

static int city_name_is_blank(const char *name)
{
    return name == NULL || name[0] == '\0';
}

GameActionResult game_found_city(Game *game, const char *name, City **created_city)
{
    Player *player = game_current_player(game);
    Unit *settler = game->selected_unit;
    const Tile *tile;
    char generated_name[CITY_NAME_LENGTH];
    City *city;

    if (created_city != NULL) {
        *created_city = NULL;
    }
    if (!selected_unit_is_current_player_living(game)) {
        game->selected_unit = NULL;
        return GAME_ACTION_NO_SELECTION;
    }
    if (settler->type != UNIT_SETTLER) {
        return GAME_ACTION_NOT_SETTLER;
    }
    if (game->city_count >= MAX_CITIES) {
        return GAME_ACTION_FULL;
    }

    tile = map_get_tile_const(&game->map, settler->x, settler->y);
    if (tile == NULL) {
        return GAME_ACTION_INVALID_INPUT;
    }
    if (tile->type != TILE_PLAINS && tile->type != TILE_HILL) {
        return GAME_ACTION_WRONG_TERRAIN;
    }
    if (get_city_at(game, settler->x, settler->y) != NULL) {
        return GAME_ACTION_HAS_CITY;
    }

    for (int i = 0; i < game->city_count; i++) {
        if (distance_manhattan(settler->x, settler->y, game->cities[i].x, game->cities[i].y) < 4) {
            return GAME_ACTION_TOO_CLOSE;
        }
    }

    if (city_name_is_blank(name)) {
        snprintf(generated_name, sizeof(generated_name), "City%d", game->city_count + 1);
        name = generated_name;
    }

    city = add_city(game, player->id, settler->x, settler->y, name);
    if (city == NULL) {
        return GAME_ACTION_FULL;
    }

    settler->alive = 0;
    settler->hp = 0;
    settler->x = -1;
    settler->y = -1;
    game->selected_unit = NULL;
    player->cities_settled++;

    if (created_city != NULL) {
        *created_city = city;
    }

    return GAME_ACTION_OK;
}

GameActionResult game_spawn_unit(Game *game, UnitType type, int x, int y, Unit **created_unit)
{
    Unit candidate;

    if (created_unit != NULL) {
        *created_unit = NULL;
    }

    if (game->unit_count >= MAX_UNITS) {
        return GAME_ACTION_FULL;
    }
    if (!map_in_bounds(&game->map, x, y)) {
        return GAME_ACTION_INVALID_INPUT;
    }
    if (is_tile_occupied_by_living_unit(game, x, y)) {
        return GAME_ACTION_OCCUPIED;
    }

    unit_init(&candidate, type, game_current_player(game)->id, x, y);
    if (!unit_can_move_to(&candidate, &game->map, x, y)) {
        return GAME_ACTION_WRONG_TERRAIN;
    }

    game->units[game->unit_count] = candidate;
    if (created_unit != NULL) {
        *created_unit = &game->units[game->unit_count];
    }
    game->unit_count++;

    return GAME_ACTION_OK;
}

GameActionResult game_select_unit_at(Game *game, int x, int y)
{
    Player *player = game_current_player(game);
    Unit *unit;

    if (!map_in_bounds(&game->map, x, y)) {
        return GAME_ACTION_INVALID_INPUT;
    }

    unit = get_living_unit_at(game, x, y);
    if (unit == NULL || unit->owner_id != player->id) {
        return GAME_ACTION_NO_UNIT;
    }

    game->selected_unit = unit;
    return GAME_ACTION_OK;
}

GameActionResult game_move_selected_unit(Game *game, GameDirection direction)
{
    int dx;
    int dy;
    int new_x;
    int new_y;
    Unit *occupant;

    if (!selected_unit_is_current_player_living(game)) {
        game->selected_unit = NULL;
        return GAME_ACTION_NO_SELECTION;
    }

    direction_to_delta(direction, &dx, &dy);
    new_x = game->selected_unit->x + dx;
    new_y = game->selected_unit->y + dy;

    if (game->selected_unit->movement_points <= 0) {
        return GAME_ACTION_NO_MOVEMENT;
    }
    if (!map_in_bounds(&game->map, new_x, new_y)) {
        return GAME_ACTION_BLOCKED;
    }

    occupant = get_living_unit_at(game, new_x, new_y);
    if (occupant != NULL) {
        return occupant->owner_id == game->selected_unit->owner_id
            ? GAME_ACTION_OCCUPIED
            : GAME_ACTION_BLOCKED;
    }
    if (!unit_move(game->selected_unit, &game->map, dx, dy)) {
        return GAME_ACTION_WRONG_TERRAIN;
    }

    return GAME_ACTION_OK;
}

static int manhattan_distance(int ax, int ay, int bx, int by)
{
    int dx = ax - bx;
    int dy = ay - by;
    return abs(dx) + abs(dy);
}

static int get_unit_defense_bonus(const Game *game, const Unit *unit)
{
    const Tile *tile = map_get_tile_const(&game->map, unit->x, unit->y);
    const City *city = get_city_at_const(game, unit->x, unit->y);
    int bonus = 0;

    if (tile != NULL && (tile->type == TILE_HILL || tile->type == TILE_FOREST)) {
        bonus++;
    }

    if (city != NULL && city->owner_id == unit->owner_id && city_has_valid_owner(city)) {
        bonus += 2;
        if (city->has_walls) {
            bonus += 2;
        }
    }

    return bonus;
}

static int calculate_damage(const Unit *attacker, int defender_defense)
{
    int damage = attacker->attack - defender_defense / 2;

    if (damage < 1) {
        damage = 1;
    }
    if (damage > attacker->attack) {
        damage = attacker->attack;
    }

    return damage;
}

static void apply_damage(Unit *unit, int damage)
{
    unit->hp -= damage;
    if (unit->hp <= 0) {
        unit->hp = 0;
        unit->alive = 0;
        unit->x = -1;
        unit->y = -1;
    }
}

CombatResult game_attack_selected_unit(Game *game, int x, int y)
{
    CombatResult result = {GAME_ACTION_OK, NULL, NULL, 0, 0, 0, 0};
    Unit *attacker = game->selected_unit;
    Unit *defender;
    int distance;

    if (!selected_unit_is_current_player_living(game)) {
        game->selected_unit = NULL;
        result.result = GAME_ACTION_NO_SELECTION;
        return result;
    }
    if (!map_in_bounds(&game->map, x, y)) {
        result.result = GAME_ACTION_INVALID_INPUT;
        return result;
    }
    if (attacker->attacks_remaining <= 0) {
        result.result = GAME_ACTION_NO_ATTACKS;
        return result;
    }
    if (attacker->attack <= 0 || attacker->range <= 0) {
        result.result = GAME_ACTION_CANNOT_ATTACK;
        return result;
    }

    defender = get_living_unit_at(game, x, y);
    if (defender == NULL) {
        result.result = GAME_ACTION_NO_UNIT;
        return result;
    }
    if (defender->owner_id == attacker->owner_id) {
        result.result = GAME_ACTION_FRIENDLY_UNIT;
        return result;
    }

    distance = manhattan_distance(attacker->x, attacker->y, defender->x, defender->y);
    if (distance > attacker->range) {
        result.result = GAME_ACTION_OUT_OF_RANGE;
        return result;
    }

    result.attacker = attacker;
    result.defender = defender;
    result.damage = calculate_damage(attacker, defender->defense + get_unit_defense_bonus(game, defender));
    attacker->attacks_remaining = 0;
    if (!unit_can_move_after_attack(attacker->type)) {
        attacker->movement_points = 0;
    }
    apply_damage(defender, result.damage);
    result.defender_destroyed = !defender->alive;
    if (result.defender_destroyed) {
        if (defender->owner_id >= 1 && defender->owner_id <= PLAYER_COUNT) {
            game->players[defender->owner_id - 1].units_lost++;
        }
        if (attacker->owner_id >= 1 && attacker->owner_id <= PLAYER_COUNT) {
            game->players[attacker->owner_id - 1].enemy_units_destroyed++;
        }
    }

    if (defender->alive && distance == 1 && defender->range == 1) {
        result.counter_damage = calculate_damage(defender, attacker->defense + get_unit_defense_bonus(game, attacker));
        apply_damage(attacker, result.counter_damage);
        result.attacker_destroyed = !attacker->alive;
        if (result.attacker_destroyed) {
            if (attacker->owner_id >= 1 && attacker->owner_id <= PLAYER_COUNT) {
                game->players[attacker->owner_id - 1].units_lost++;
            }
            if (defender->owner_id >= 1 && defender->owner_id <= PLAYER_COUNT) {
                game->players[defender->owner_id - 1].enemy_units_destroyed++;
            }
        }
        if (!attacker->alive) {
            game->selected_unit = NULL;
        }
    }

    if (game->selected_unit != NULL && !game->selected_unit->alive) {
        game->selected_unit = NULL;
    }

    return result;
}

int player_calculate_score(const Player *player)
{
    return player->cities_settled * 10
        + player->cities_conquered * 20
        + player->enemy_units_destroyed * 5
        + player->production_points_generated
        + player->science_generated_total
        + player->technologies_researched * 15
        + player->buildings_built * 3
        + player->turns_survived * 2
        - player->units_lost * 2
        - player->cities_lost * 5;
}

void game_print_leaderboard(const Game *game)
{
    int printed[PLAYER_COUNT] = {0};
    int rank = 1;

    if (game->game_over) {
        printf("\n===== CONQUEST COMPLETE =====\n");
        printf("Winner: %s\n\n", get_player_name_safe(game, game->winner_player_id));
    } else {
        printf("\n===== CURRENT LEADERBOARD =====\n\n");
    }
    printf("Leaderboard:\n");

    for (int pass = 0; pass < PLAYER_COUNT; pass++) {
        int best = -1;

        for (int i = 0; i < PLAYER_COUNT; i++) {
            const Player *player = &game->players[i];
            if (printed[i]) {
                continue;
            }
            if (best < 0) {
                best = i;
                continue;
            }
            if (player->is_winner && !game->players[best].is_winner) {
                best = i;
            } else if (!player->is_eliminated && game->players[best].is_eliminated) {
                best = i;
            } else if (player->is_eliminated
                && game->players[best].is_eliminated
                && player->elimination_turn > game->players[best].elimination_turn) {
                best = i;
            }
        }

        if (best < 0) {
            break;
        }

        printed[best] = 1;
        const Player *player = &game->players[best];
        printf("%d. %s\n", rank, player->name);
        if (player->is_winner) {
            printf("   Status: Winner\n");
        } else if (player->is_eliminated) {
            printf("   Status: Eliminated by %s on turn %d\n",
                get_player_name_safe(game, player->eliminated_by_player_id),
                player->elimination_turn);
        } else {
            printf("   Status: Survived\n");
        }
        printf("   Score: %d\n", player_calculate_score(player));
        printf("   Cities settled: %d\n", player->cities_settled);
        printf("   Cities conquered: %d\n", player->cities_conquered);
        printf("   Units built: %d\n", player->units_built);
        printf("   Units lost: %d\n", player->units_lost);
        printf("   Enemy units destroyed: %d\n", player->enemy_units_destroyed);
        printf("   Production points generated: %d\n", player->production_points_generated);
        printf("   Science generated: %d\n", player->science_generated_total);
        printf("   Technologies researched: %d\n", player->technologies_researched);
        printf("   Buildings built: %d\n", player->buildings_built);
        printf("   Cities lost: %d\n", player->cities_lost);
        printf("   Turns survived: %d\n\n", player->turns_survived);
        rank++;
    }
}
