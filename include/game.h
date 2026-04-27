#ifndef GAME_H
#define GAME_H

#include "map.h"
#include "player.h"
#include "unit.h"

#define PLAYER_COUNT 2
#define MAX_UNITS 256
#define MAX_CITIES 64
#define MAX_TURN_MESSAGES 16
#define TURN_MESSAGE_LENGTH 96
#define CITY_NAME_POOL_SIZE 100

typedef struct {
    Map map;
    Player players[PLAYER_COUNT];
    Unit units[MAX_UNITS];
    City cities[MAX_CITIES];
    int unit_count;
    int city_count;
    int current_player_index;
    Unit *selected_unit;
    unsigned int seed;
    int turn_number;
    int game_over;
    int winner_player_id;
    int last_points_gained;
    int last_science_gained;
    int used_city_names[CITY_NAME_POOL_SIZE];
    int generated_city_name_count;
    int turn_message_count;
    char turn_messages[MAX_TURN_MESSAGES][TURN_MESSAGE_LENGTH];
} Game;

typedef enum {
    GAME_DIRECTION_NORTH,
    GAME_DIRECTION_SOUTH,
    GAME_DIRECTION_EAST,
    GAME_DIRECTION_WEST
} GameDirection;

typedef enum {
    GAME_ACTION_OK,
    GAME_ACTION_INVALID_INPUT,
    GAME_ACTION_NO_UNIT,
    GAME_ACTION_NO_SELECTION,
    GAME_ACTION_NO_MOVEMENT,
    GAME_ACTION_NO_ATTACKS,
    GAME_ACTION_BLOCKED,
    GAME_ACTION_OUT_OF_RANGE,
    GAME_ACTION_FRIENDLY_UNIT,
    GAME_ACTION_OCCUPIED,
    GAME_ACTION_WRONG_TERRAIN,
    GAME_ACTION_FULL,
    GAME_ACTION_NOT_ENOUGH_POINTS,
    GAME_ACTION_NO_CITY,
    GAME_ACTION_NO_SPAWN_TILE,
    GAME_ACTION_TOO_CLOSE,
    GAME_ACTION_NOT_SETTLER,
    GAME_ACTION_HAS_CITY,
    GAME_ACTION_CANNOT_ATTACK,
    GAME_ACTION_ALREADY_HAS_WALLS,
    GAME_ACTION_INVALID_OWNER,
    GAME_ACTION_GAME_OVER,
    GAME_ACTION_REQUIRES_TECH,
    GAME_ACTION_ALREADY_RESEARCHED,
    GAME_ACTION_MISSING_PREREQUISITE,
    GAME_ACTION_ALREADY_HAS_BUILDING,
    GAME_ACTION_NO_SCIENCE_SLOT
} GameActionResult;

typedef struct {
    GameActionResult result;
    Unit *attacker;
    Unit *defender;
    int damage;
    int counter_damage;
    int defender_destroyed;
    int attacker_destroyed;
} CombatResult;

int game_init(Game *game, unsigned int seed, int map_width, int map_height);
void game_free(Game *game);
void game_start_turn(Game *game);
void game_end_turn(Game *game);
Unit *get_unit_at(Game *game, int x, int y);
const Unit *get_unit_at_const(const Game *game, int x, int y);
Unit *get_living_unit_at(Game *game, int x, int y);
const Unit *get_living_unit_at_const(const Game *game, int x, int y);
int is_tile_occupied_by_living_unit(const Game *game, int x, int y);
City *get_city_at(Game *game, int x, int y);
const City *get_city_at_const(const Game *game, int x, int y);
City *get_city_controlling_tile(Game *game, int x, int y);
const City *get_city_controlling_tile_const(const Game *game, int x, int y);
int player_controls_tile(const Game *game, int owner_id, int x, int y);
int tile_is_inside_any_city_border(const Game *game, int x, int y);
int player_is_active(const Game *game, int player_id);
const char *get_player_name_safe(const Game *game, int player_id);
int game_count_living_units_for_player(const Game *game, int owner_id);
int game_count_cities_for_player(const Game *game, int owner_id);
const Unit *game_selected_unit_const(const Game *game);
int city_get_income(const Game *game, const City *city);
int player_get_income_per_turn(const Game *game, int owner_id);
int player_add_turn_income(Game *game, Player *player);
int city_add_border_growth(City *city);
int city_get_science_slots(const Game *game, const City *city);
int city_get_science_per_turn(const Game *game, const City *city);
int player_get_science_per_turn(const Game *game, int owner_id);
int player_has_tech(const Player *player, TechType tech);
int player_can_research_tech(const Player *player, TechType tech);
void player_unlock_tech(Player *player, TechType tech);
int player_current_research_is_valid(const Player *player);
int player_add_science_progress(Game *game, Player *player);
TechType tech_unlocks_unit(UnitType type);
TechType tech_unlocks_building(TileBuilding building);
TechType tech_unlocks_city_upgrade(const char *upgrade);
int city_can_build_science_building(const Game *game, const Player *player, const City *city, CityScienceBuilding building);
GameActionResult game_set_research(Game *game, TechType tech);
GameActionResult game_build_science_building(Game *game, CityScienceBuilding building, int city_x, int city_y);
GameActionResult game_upgrade_city_walls(Game *game, int city_x, int city_y);
GameActionResult game_build_tile_building(Game *game, TileBuilding building, int x, int y);
GameActionResult game_train_unit(Game *game, UnitType type, int city_x, int city_y, Unit **created_unit);
GameActionResult game_found_city(Game *game, const char *name, City **created_city);
GameActionResult game_spawn_unit(Game *game, UnitType type, int x, int y, Unit **created_unit);
GameActionResult game_select_unit_at(Game *game, int x, int y);
GameActionResult game_move_selected_unit(Game *game, GameDirection direction);
CombatResult game_attack_selected_unit(Game *game, int x, int y);
int player_calculate_score(const Player *player);
void game_print_leaderboard(const Game *game);
Player *game_current_player(Game *game);
const Player *game_current_player_const(const Game *game);

#endif
