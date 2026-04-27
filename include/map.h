#ifndef MAP_H
#define MAP_H

typedef enum {
    TILE_WATER,
    TILE_PLAINS,
    TILE_FOREST,
    TILE_HILL,
    TILE_MOUNTAIN
} TileType;

typedef enum {
    BUILDING_NONE,
    BUILDING_FARM,
    BUILDING_MINE,
    BUILDING_PORT,
    BUILDING_SAWMILL,
    BUILDING_LUMBER_CAMP
} TileBuilding;

typedef struct {
    TileType type;
    TileBuilding building;
} Tile;

typedef struct {
    int width;
    int height;
    Tile *tiles;
} Map;

int map_init(Map *map, int width, int height, TileType fill_type);
void map_free(Map *map);
void map_generate_archipelago(Map *map);
int map_in_bounds(const Map *map, int x, int y);
Tile *map_get_tile(Map *map, int x, int y);
const Tile *map_get_tile_const(const Map *map, int x, int y);
void map_set_tile(Map *map, int x, int y, TileType type);
int map_is_land_passable(const Map *map, int x, int y);
int tile_can_host_city(TileType type);
int map_tile_can_host_city(const Map *map, int x, int y);
int building_get_cost(TileBuilding building);
int building_get_income_bonus(TileBuilding building);
int building_type_from_string(const char *text, TileBuilding *building);
const char *building_type_to_string(TileBuilding building);
int building_can_be_built_on_tile(TileBuilding building, TileType tile_type);

#endif
