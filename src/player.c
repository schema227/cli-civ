#include "player.h"

#include <stdio.h>

void player_init(Player *player, int id, const char *name, char symbol)
{
    player->id = id;
    player->symbol = symbol;
    player->points = 10;
    player->science = 0;
    player->science_generated_total = 0;
    player->current_research = TECH_NONE;
    for (int i = 0; i < TECH_COUNT; i++) {
        player->research_progress[i] = 0;
        player->unlocked_techs[i] = 0;
    }
    player->unlocked_techs[TECH_AGRICULTURE] = 1;
    snprintf(player->name, PLAYER_NAME_LENGTH, "%s", name);
    city_init(&player->starting_city, id, 0, 0, "");
    player->is_eliminated = 0;
    player->capital_x = 0;
    player->capital_y = 0;
    player->elimination_turn = 0;
    player->eliminated_by_player_id = 0;
    player->is_winner = 0;
    player->cities_settled = 0;
    player->cities_conquered = 0;
    player->units_built = 0;
    player->units_lost = 0;
    player->enemy_units_destroyed = 0;
    player->production_points_generated = 0;
    player->buildings_built = 0;
    player->cities_lost = 0;
    player->turns_survived = 0;
    player->technologies_researched = 0;
}
