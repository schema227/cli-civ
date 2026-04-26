#include "game.h"
#include "render.h"

#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int width;
    int height;
} MapSize;

static unsigned int parse_seed_or_default(int argc, char **argv)
{
    if (argc < 2) {
        return (unsigned int)time(NULL);
    }

    errno = 0;
    char *end = NULL;
    unsigned long parsed = strtoul(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' || parsed > UINT_MAX) {
        fprintf(stderr, "Invalid seed '%s'. Use an unsigned integer.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return (unsigned int)parsed;
}

static void trim_newline(char *text)
{
    size_t length = strlen(text);
    if (length > 0 && text[length - 1] == '\n') {
        text[length - 1] = '\0';
    }
}

static void to_lowercase(char *text)
{
    for (size_t i = 0; text[i] != '\0'; i++) {
        text[i] = (char)tolower((unsigned char)text[i]);
    }
}

static void discard_remaining_line(void)
{
    int ch;

    do {
        ch = getchar();
    } while (ch != '\n' && ch != EOF);
}

static int parse_map_size(const char *text, MapSize *size)
{
    if (strcmp(text, "") == 0 || strcmp(text, "3") == 0 || strcmp(text, "large") == 0) {
        *size = (MapSize){64, 64};
    } else if (strcmp(text, "1") == 0 || strcmp(text, "small") == 0) {
        *size = (MapSize){32, 32};
    } else if (strcmp(text, "2") == 0 || strcmp(text, "medium") == 0) {
        *size = (MapSize){48, 48};
    } else if (strcmp(text, "4") == 0 || strcmp(text, "xlarge") == 0) {
        *size = (MapSize){96, 96};
    } else {
        return 0;
    }

    return 1;
}

static MapSize prompt_map_size(void)
{
    char input[64];
    MapSize size;

    for (;;) {
        printf("\nChoose map size:\n");
        printf("1. small  - 32x32\n");
        printf("2. medium - 48x48\n");
        printf("3. large  - 64x64\n");
        printf("4. xlarge - 96x96\n\n");
        printf("Enter map size: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return (MapSize){64, 64};
        }

        if (strchr(input, '\n') == NULL) {
            discard_remaining_line();
        }
        trim_newline(input);
        to_lowercase(input);
        if (parse_map_size(input, &size)) {
            return size;
        }

        printf("Invalid map size. Enter 1, 2, 3, 4, small, medium, large, or xlarge.\n");
    }
}

static void prompt_civilization_names(Game *game)
{
    char input[PLAYER_NAME_LENGTH];

    for (int i = 0; i < PLAYER_COUNT; i++) {
        printf("Enter name for Civilization %d: ", i + 1);
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            snprintf(game->players[i].name, PLAYER_NAME_LENGTH, "Player %d", i + 1);
            continue;
        }
        if (strchr(input, '\n') == NULL) {
            discard_remaining_line();
        }
        trim_newline(input);

        if (input[0] == '\0') {
            snprintf(game->players[i].name, PLAYER_NAME_LENGTH, "Player %d", i + 1);
        } else {
            snprintf(game->players[i].name, PLAYER_NAME_LENGTH, "%s", input);
        }
    }
}

int main(int argc, char **argv)
{
    Game game;
    unsigned int seed = parse_seed_or_default(argc, argv);
    MapSize map_size = prompt_map_size();

    if (!game_init(&game, seed, map_size.width, map_size.height)) {
        fprintf(stderr, "Failed to initialize game map.\n");
        return EXIT_FAILURE;
    }
    prompt_civilization_names(&game);
    render_console_run(&game);
    game_free(&game);

    return 0;
}
