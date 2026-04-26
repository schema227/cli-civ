#include "unit.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    int hp;
    int attack;
    int defense;
    int range;
    int movement;
} UnitStats;

static UnitStats unit_stats(UnitType type)
{
    switch (type) {
        case UNIT_WARRIOR:
            return (UnitStats){10, 3, 2, 1, 1};
        case UNIT_ARCHER:
            return (UnitStats){7, 3, 1, 2, 1};
        case UNIT_CATAPULT:
            return (UnitStats){6, 5, 1, 3, 1};
        case UNIT_DEFENDER:
            return (UnitStats){12, 2, 4, 1, 1};
        case UNIT_KNIGHT:
            return (UnitStats){10, 4, 2, 1, 2};
        case UNIT_SETTLER:
            return (UnitStats){5, 0, 1, 0, 1};
        case UNIT_ASSASSIN:
            return (UnitStats){6, 6, 1, 1, 2};
        case UNIT_SLOOP:
            return (UnitStats){8, 3, 1, 2, 3};
        case UNIT_BRIG:
            return (UnitStats){12, 4, 2, 2, 2};
        case UNIT_GALLEON:
            return (UnitStats){16, 6, 3, 3, 1};
    }

    return (UnitStats){10, 3, 2, 1, 1};
}

void unit_init(Unit *unit, UnitType type, int owner_id, int x, int y)
{
    UnitStats stats = unit_stats(type);

    unit->type = type;
    unit->owner_id = owner_id;
    unit->x = x;
    unit->y = y;
    unit->hp = stats.hp;
    unit->max_hp = stats.hp;
    unit->attack = stats.attack;
    unit->defense = stats.defense;
    unit->range = stats.range;
    unit->movement_points = stats.movement;
    unit->max_movement_points = stats.movement;
    unit->attacks_remaining = 1;
    unit->symbol = unit_symbol_for_owner(type, owner_id);
    unit->alive = 1;
}

void unit_reset_turn(Unit *unit)
{
    if (!unit->alive) {
        return;
    }
    unit->movement_points = unit->max_movement_points;
    unit->attacks_remaining = 1;
}

int unit_type_from_string(const char *text, UnitType *type)
{
    if (text == NULL || type == NULL) {
        return 0;
    }

    if (strcmp(text, "warrior") == 0) {
        *type = UNIT_WARRIOR;
    } else if (strcmp(text, "archer") == 0) {
        *type = UNIT_ARCHER;
    } else if (strcmp(text, "catapult") == 0) {
        *type = UNIT_CATAPULT;
    } else if (strcmp(text, "defender") == 0) {
        *type = UNIT_DEFENDER;
    } else if (strcmp(text, "knight") == 0) {
        *type = UNIT_KNIGHT;
    } else if (strcmp(text, "settler") == 0) {
        *type = UNIT_SETTLER;
    } else if (strcmp(text, "assassin") == 0) {
        *type = UNIT_ASSASSIN;
    } else if (strcmp(text, "sloop") == 0) {
        *type = UNIT_SLOOP;
    } else if (strcmp(text, "brig") == 0) {
        *type = UNIT_BRIG;
    } else if (strcmp(text, "galleon") == 0) {
        *type = UNIT_GALLEON;
    } else {
        return 0;
    }

    return 1;
}

const char *unit_type_to_string(UnitType type)
{
    switch (type) {
        case UNIT_WARRIOR:
            return "Warrior";
        case UNIT_ARCHER:
            return "Archer";
        case UNIT_CATAPULT:
            return "Catapult";
        case UNIT_DEFENDER:
            return "Defender";
        case UNIT_KNIGHT:
            return "Knight";
        case UNIT_SETTLER:
            return "Settler";
        case UNIT_ASSASSIN:
            return "Assassin";
        case UNIT_SLOOP:
            return "Sloop";
        case UNIT_BRIG:
            return "Brig";
        case UNIT_GALLEON:
            return "Galleon";
    }

    return "Unknown";
}

char unit_symbol_for_owner(UnitType type, int owner_id)
{
    char symbol = '?';

    switch (type) {
        case UNIT_WARRIOR:
            symbol = 'W';
            break;
        case UNIT_ARCHER:
            symbol = 'A';
            break;
        case UNIT_CATAPULT:
            symbol = 'T';
            break;
        case UNIT_DEFENDER:
            symbol = 'D';
            break;
        case UNIT_KNIGHT:
            symbol = 'N';
            break;
        case UNIT_SETTLER:
            symbol = 'L';
            break;
        case UNIT_ASSASSIN:
            symbol = 'X';
            break;
        case UNIT_SLOOP:
            symbol = 'P';
            break;
        case UNIT_BRIG:
            symbol = 'B';
            break;
        case UNIT_GALLEON:
            symbol = 'G';
            break;
    }

    if (owner_id == 2 && symbol >= 'A' && symbol <= 'Z') {
        symbol = (char)(symbol + ('a' - 'A'));
    }

    return symbol;
}

int unit_get_cost(UnitType type)
{
    switch (type) {
        case UNIT_WARRIOR:
            return 6;
        case UNIT_ARCHER:
            return 8;
        case UNIT_DEFENDER:
            return 8;
        case UNIT_KNIGHT:
            return 12;
        case UNIT_CATAPULT:
            return 14;
        case UNIT_SETTLER:
            return 15;
        case UNIT_ASSASSIN:
            return 14;
        case UNIT_SLOOP:
            return 10;
        case UNIT_BRIG:
            return 14;
        case UNIT_GALLEON:
            return 20;
    }

    return 0;
}

int unit_is_ship_type(UnitType type)
{
    return type == UNIT_SLOOP || type == UNIT_BRIG || type == UNIT_GALLEON;
}

int unit_can_move_after_attack(UnitType type)
{
    return type == UNIT_ASSASSIN;
}

int unit_can_move_on_tile(const Unit *unit, TileType tile_type)
{
    if (tile_type == TILE_MOUNTAIN) {
        return 0;
    }

    if (unit_is_ship_type(unit->type)) {
        return tile_type == TILE_WATER;
    }

    return tile_type == TILE_PLAINS || tile_type == TILE_FOREST || tile_type == TILE_HILL;
}

int unit_can_move_to(const Unit *unit, const Map *map, int x, int y)
{
    const Tile *tile = map_get_tile_const(map, x, y);
    if (tile == NULL) {
        return 0;
    }

    return unit_can_move_on_tile(unit, tile->type);
}

int unit_move(Unit *unit, const Map *map, int dx, int dy)
{
    int new_x = unit->x + dx;
    int new_y = unit->y + dy;

    if (abs(dx) + abs(dy) != 1) {
        return 0;
    }
    if (!unit->alive) {
        return 0;
    }
    if (unit->movement_points <= 0) {
        return 0;
    }
    if (!unit_can_move_to(unit, map, new_x, new_y)) {
        return 0;
    }

    unit->x = new_x;
    unit->y = new_y;
    unit->movement_points--;
    return 1;
}
