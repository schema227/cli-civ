#include "city.h"

#include <stdio.h>

void city_init(City *city, int owner_id, int x, int y, const char *name)
{
    city->owner_id = owner_id;
    city->x = x;
    city->y = y;
    snprintf(city->name, CITY_NAME_LENGTH, "%s", name);
    city->population = 1;
    city->border_radius = 1;
    city->border_growth_progress = 0;
    city->base_points_per_turn = 2;
    city->border_growth_per_turn = 1;
    city->border_growth_needed = 5;
    city->is_capital = 0;
    city->has_walls = 0;
    city->science_building_count = 0;
    city_clear_siege(city);
}

int city_contains_tile(const City *city, int x, int y)
{
    int dx;
    int dy;

    if (city == NULL) {
        return 0;
    }

    dx = city->x - x;
    dy = city->y - y;

    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }

    return dx + dy <= city->border_radius;
}

int city_is_free(const City *city)
{
    return city != NULL && city->owner_id == OWNER_FREE;
}

int city_has_valid_owner(const City *city)
{
    return city != NULL && city->owner_id > 0;
}

void city_clear_siege(City *city)
{
    if (city == NULL) {
        return;
    }

    city->siege_player_id = 0;
    city->siege_unit_index = -1;
    city->siege_started_turn = 0;
}

int city_has_science_building(const City *city, CityScienceBuilding building)
{
    if (city == NULL || building < 0 || building >= CITY_SCIENCE_COUNT) {
        return 0;
    }

    for (int i = 0; i < city->science_building_count; i++) {
        if (city->science_buildings[i] == building) {
            return 1;
        }
    }

    return 0;
}

int city_add_science_building(City *city, CityScienceBuilding building)
{
    if (city == NULL || building < 0 || building >= CITY_SCIENCE_COUNT) {
        return 0;
    }
    if (city->science_building_count >= CITY_SCIENCE_COUNT || city_has_science_building(city, building)) {
        return 0;
    }

    city->science_buildings[city->science_building_count] = building;
    city->science_building_count++;
    return 1;
}
