#ifndef RENDER_H
#define RENDER_H

#include "game.h"

void render_console_run(Game *game);
void render_console_map(const Game *game);
void render_console_status(const Game *game);
void render_console_help(void);

#endif
