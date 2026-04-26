#include "map.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int clamp_int(int value, int min, int max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

static Tile *tile_at(Map *map, int x, int y)
{
    return &map->tiles[y * map->width + x];
}

static const Tile *tile_at_const(const Map *map, int x, int y)
{
    return &map->tiles[y * map->width + x];
}

int map_init(Map *map, int width, int height, TileType fill_type)
{
    size_t tile_count;

    if (map == NULL) {
        return 0;
    }

    if (width <= 0 || height <= 0) {
        map->width = 0;
        map->height = 0;
        map->tiles = NULL;
        return 0;
    }

    tile_count = (size_t)width * (size_t)height;
    if ((size_t)width != tile_count / (size_t)height || tile_count > SIZE_MAX / sizeof(Tile)) {
        map->width = 0;
        map->height = 0;
        map->tiles = NULL;
        return 0;
    }

    map->width = width;
    map->height = height;
    map->tiles = malloc(tile_count * sizeof(Tile));
    if (map->tiles == NULL) {
        map->width = 0;
        map->height = 0;
        return 0;
    }

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Tile *tile = tile_at(map, x, y);
            tile->type = fill_type;
            tile->building = BUILDING_NONE;
        }
    }

    return 1;
}

void map_free(Map *map)
{
    if (map == NULL) {
        return;
    }

    free(map->tiles);
    map->tiles = NULL;
    map->width = 0;
    map->height = 0;
}

int map_in_bounds(const Map *map, int x, int y)
{
    return map != NULL
        && map->tiles != NULL
        && x >= 0
        && x < map->width
        && y >= 0
        && y < map->height;
}

Tile *map_get_tile(Map *map, int x, int y)
{
    if (!map_in_bounds(map, x, y)) {
        return NULL;
    }
    return tile_at(map, x, y);
}

const Tile *map_get_tile_const(const Map *map, int x, int y)
{
    if (!map_in_bounds(map, x, y)) {
        return NULL;
    }
    return tile_at_const(map, x, y);
}

void map_set_tile(Map *map, int x, int y, TileType type)
{
    Tile *tile = map_get_tile(map, x, y);
    if (tile != NULL) {
        tile->type = type;
        if (!building_can_be_built_on_tile(tile->building, type)) {
            tile->building = BUILDING_NONE;
        }
    }
}

int map_is_land_passable(const Map *map, int x, int y)
{
    const Tile *tile = map_get_tile_const(map, x, y);
    if (tile == NULL) {
        return 0;
    }
    return tile->type == TILE_PLAINS || tile->type == TILE_FOREST || tile->type == TILE_HILL;
}

int building_get_cost(TileBuilding building)
{
    switch (building) {
        case BUILDING_FARM:
            return 5;
        case BUILDING_MINE:
            return 7;
        case BUILDING_PORT:
            return 8;
        case BUILDING_SAWMILL:
            return 6;
        case BUILDING_LUMBER_CAMP:
            return 6;
        case BUILDING_NONE:
            return 0;
    }

    return 0;
}

int building_get_income_bonus(TileBuilding building)
{
    switch (building) {
        case BUILDING_FARM:
            return 2;
        case BUILDING_MINE:
            return 3;
        case BUILDING_PORT:
            return 3;
        case BUILDING_SAWMILL:
            return 2;
        case BUILDING_LUMBER_CAMP:
            return 3;
        case BUILDING_NONE:
            return 0;
    }

    return 0;
}

int building_type_from_string(const char *text, TileBuilding *building)
{
    if (text == NULL || building == NULL) {
        return 0;
    }

    if (strcmp(text, "farm") == 0) {
        *building = BUILDING_FARM;
    } else if (strcmp(text, "mine") == 0) {
        *building = BUILDING_MINE;
    } else if (strcmp(text, "port") == 0) {
        *building = BUILDING_PORT;
    } else if (strcmp(text, "sawmill") == 0) {
        *building = BUILDING_SAWMILL;
    } else if (strcmp(text, "lumbercamp") == 0
        || strcmp(text, "lumber_camp") == 0
        || strcmp(text, "lumber-camp") == 0) {
        *building = BUILDING_LUMBER_CAMP;
    } else {
        return 0;
    }

    return 1;
}

const char *building_type_to_string(TileBuilding building)
{
    switch (building) {
        case BUILDING_FARM:
            return "Farm";
        case BUILDING_MINE:
            return "Mine";
        case BUILDING_PORT:
            return "Port";
        case BUILDING_SAWMILL:
            return "Sawmill";
        case BUILDING_LUMBER_CAMP:
            return "Lumber Camp";
        case BUILDING_NONE:
            return "None";
    }

    return "Unknown";
}

int building_can_be_built_on_tile(TileBuilding building, TileType tile_type)
{
    if (tile_type == TILE_MOUNTAIN) {
        return 0;
    }

    switch (building) {
        case BUILDING_FARM:
            return tile_type == TILE_PLAINS;
        case BUILDING_MINE:
            return tile_type == TILE_HILL;
        case BUILDING_PORT:
            return tile_type == TILE_WATER;
        case BUILDING_SAWMILL:
            return tile_type == TILE_PLAINS || tile_type == TILE_HILL;
        case BUILDING_LUMBER_CAMP:
            return tile_type == TILE_FOREST;
        case BUILDING_NONE:
            return 0;
    }

    return 0;
}

static TileType choose_land_type(int distance_from_center, int island_radius)
{
    int roll = rand() % 100;
    int center_bonus = island_radius - distance_from_center;

    if (center_bonus > island_radius / 2 && roll < 10) {
        return TILE_MOUNTAIN;
    }
    if (roll < 18) {
        return TILE_HILL;
    }
    if (roll < 21) {
        return TILE_MOUNTAIN;
    }
    if (roll < 41) {
        return TILE_FOREST;
    }
    return TILE_PLAINS;
}

static void carve_island(Map *map, int center_x, int center_y, int radius)
{
    int walker_count = 5 + rand() % 8;
    int steps = radius * radius * (3 + rand() % 3);

    if (map == NULL || map->tiles == NULL || map->width < 5 || map->height < 5) {
        return;
    }

    for (int walker = 0; walker < walker_count; walker++) {
        int x = center_x + (rand() % (radius + 1)) - radius / 2;
        int y = center_y + (rand() % (radius + 1)) - radius / 2;

        for (int step = 0; step < steps; step++) {
            int dx = x - center_x;
            int dy = y - center_y;
            int distance = abs(dx) + abs(dy);
            int keep_chance = 95 - clamp_int(distance * 7, 0, 75);

            if (map_in_bounds(map, x, y) && x > 1 && y > 1 && x < map->width - 2 && y < map->height - 2) {
                if ((rand() % 100) < keep_chance) {
                    map_set_tile(map, x, y, choose_land_type(distance, radius));
                }
            }

            if ((rand() % 100) < 55) {
                if (x < center_x) {
                    x++;
                } else if (x > center_x) {
                    x--;
                }
                if ((rand() % 100) < 35) {
                    if (y < center_y) {
                        y++;
                    } else if (y > center_y) {
                        y--;
                    }
                }
            } else {
                int direction = rand() % 4;
                if (direction == 0) {
                    x++;
                } else if (direction == 1) {
                    x--;
                } else if (direction == 2) {
                    y++;
                } else {
                    y--;
                }
            }

            x = clamp_int(x, 2, map->width - 3);
            y = clamp_int(y, 2, map->height - 3);
        }
    }
}

static void soften_mountains(Map *map)
{
    for (int y = 1; y < map->height - 1; y++) {
        for (int x = 1; x < map->width - 1; x++) {
            if (tile_at(map, x, y)->type != TILE_MOUNTAIN) {
                continue;
            }

            int nearby_hills = 0;
            for (int yy = y - 1; yy <= y + 1; yy++) {
                for (int xx = x - 1; xx <= x + 1; xx++) {
                    if (tile_at(map, xx, yy)->type == TILE_HILL) {
                        nearby_hills++;
                    }
                }
            }

            if (nearby_hills == 0 && (rand() % 100) < 70) {
                tile_at(map, x, y)->type = TILE_HILL;
            }
        }
    }
}

void map_generate_archipelago(Map *map)
{
    if (map == NULL || map->tiles == NULL || map->width < 5 || map->height < 5) {
        return;
    }

    int min_dim = map->width < map->height ? map->width : map->height;
    int area = map->width * map->height;
    int island_count = area / 360 + rand() % (area / 700 + 3);
    int min_radius = min_dim >= 64 ? 4 : 3;
    int radius_variance = min_dim / 8;

    if (radius_variance < 4) {
        radius_variance = 4;
    }
    if (island_count < 4) {
        island_count = 4;
    }

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
            Tile *tile = tile_at(map, x, y);
            tile->type = TILE_WATER;
            tile->building = BUILDING_NONE;
        }
    }

    for (int i = 0; i < island_count; i++) {
        int margin = min_dim / 10;
        int radius = min_radius + rand() % radius_variance;
        int center_x;
        int center_y;

        if (margin < 4) {
            margin = 4;
        }
        if (margin * 2 >= map->width || margin * 2 >= map->height) {
            margin = 2;
        }

        center_x = margin + rand() % (map->width - margin * 2);
        center_y = margin + rand() % (map->height - margin * 2);
        carve_island(map, center_x, center_y, radius);
    }

    soften_mountains(map);

    for (int x = 0; x < map->width; x++) {
        map_set_tile(map, x, 0, TILE_WATER);
        map_set_tile(map, x, map->height - 1, TILE_WATER);
    }
    for (int y = 0; y < map->height; y++) {
        map_set_tile(map, 0, y, TILE_WATER);
        map_set_tile(map, map->width - 1, y, TILE_WATER);
    }
}
