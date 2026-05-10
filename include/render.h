#ifndef RENDER_H
#define RENDER_H

#include "game.h"

typedef struct {
    int use_color;
} RenderConfig;

void render_console_run(Game *game, RenderConfig config);
void render_console_map(const Game *game, const RenderConfig *config);
void render_console_status(const Game *game);
void render_console_help(const Game *game);

#endif
