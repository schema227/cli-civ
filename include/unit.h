#ifndef UNIT_H
#define UNIT_H

#include "map.h"

typedef enum {
    UNIT_WARRIOR,
    UNIT_ARCHER,
    UNIT_CATAPULT,
    UNIT_DEFENDER,
    UNIT_KNIGHT,
    UNIT_SETTLER,
    UNIT_ASSASSIN,
    UNIT_SLOOP,
    UNIT_BRIG,
    UNIT_GALLEON
} UnitType;

typedef struct {
    UnitType type;
    int owner_id;
    int x;
    int y;
    int hp;
    int max_hp;
    int attack;
    int defense;
    int range;
    int movement_points;
    int max_movement_points;
    int attacks_remaining;
    char symbol;
    int alive;
} Unit;

void unit_init(Unit *unit, UnitType type, int owner_id, int x, int y);
void unit_reset_turn(Unit *unit);
int unit_type_from_string(const char *text, UnitType *type);
const char *unit_type_to_string(UnitType type);
char unit_symbol_for_owner(UnitType type, int owner_id);
int unit_get_cost(UnitType type);
int unit_is_ship_type(UnitType type);
int unit_can_move_after_attack(UnitType type);
int unit_can_move_on_tile(const Unit *unit, TileType tile_type);
int unit_can_move_to(const Unit *unit, const Map *map, int x, int y);
int unit_move(Unit *unit, const Map *map, int dx, int dy);

#endif
