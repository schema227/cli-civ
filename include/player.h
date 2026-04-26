#ifndef PLAYER_H
#define PLAYER_H

#include "city.h"
#include "tech.h"

#define PLAYER_NAME_LENGTH 32

typedef struct {
    int id;
    char name[PLAYER_NAME_LENGTH];
    char symbol;
    int points;
    int science;
    int science_generated_total;
    TechType current_research;
    int research_progress[TECH_COUNT];
    int unlocked_techs[TECH_COUNT];
    City starting_city;
    int is_eliminated;
    int capital_x;
    int capital_y;
    int elimination_turn;
    int eliminated_by_player_id;
    int is_winner;
    int cities_settled;
    int cities_conquered;
    int units_built;
    int units_lost;
    int enemy_units_destroyed;
    int production_points_generated;
    int buildings_built;
    int cities_lost;
    int turns_survived;
    int technologies_researched;
} Player;

void player_init(Player *player, int id, const char *name, char symbol);

#endif
