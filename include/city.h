#ifndef CITY_H
#define CITY_H

#include "tech.h"

#define CITY_NAME_LENGTH 32
#define OWNER_FREE -1

typedef struct {
    int owner_id;
    int x;
    int y;
    char name[CITY_NAME_LENGTH];
    int population;
    int border_radius;
    int border_growth_progress;
    int base_points_per_turn;
    int border_growth_per_turn;
    int border_growth_needed;
    int is_capital;
    int has_walls;
    int siege_player_id;
    int siege_unit_index;
    int siege_started_turn;
    CityScienceBuilding science_buildings[CITY_SCIENCE_COUNT];
    int science_building_count;
} City;

void city_init(City *city, int owner_id, int x, int y, const char *name);
int city_contains_tile(const City *city, int x, int y);
int city_is_free(const City *city);
int city_has_valid_owner(const City *city);
void city_clear_siege(City *city);
int city_has_science_building(const City *city, CityScienceBuilding building);
int city_add_science_building(City *city, CityScienceBuilding building);

#endif
