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

typedef struct {
    unsigned int seed;
    int use_color;
} StartupOptions;

static StartupOptions parse_startup_options(int argc, char **argv)
{
    StartupOptions options = {(unsigned int)time(NULL), 1};
    int seed_set = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0) {
            options.use_color = 0;
        } else if (strcmp(argv[i], "--color") == 0) {
            options.use_color = 1;
        } else if (!seed_set) {
            errno = 0;
            char *end = NULL;
            unsigned long parsed = strtoul(argv[i], &end, 10);
            if (errno != 0 || end == argv[i] || *end != '\0' || parsed > UINT_MAX) {
                fprintf(stderr, "Unknown argument '%s'. Use an unsigned seed, --color, or --no-color.\n", argv[i]);
                exit(EXIT_FAILURE);
            }
            options.seed = (unsigned int)parsed;
            seed_set = 1;
        } else {
            fprintf(stderr, "Unknown argument '%s'. Use one seed plus optional --color or --no-color.\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    return options;
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

static void assign_civilization_colors(Game *game)
{
    CivColor colors[] = {
        CIV_COLOR_RED,
        CIV_COLOR_YELLOW,
        CIV_COLOR_BLUE,
        CIV_COLOR_GREEN
    };
    int color_count = (int)(sizeof(colors) / sizeof(colors[0]));

    for (int i = color_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        CivColor temp = colors[i];
        colors[i] = colors[j];
        colors[j] = temp;
    }

    for (int i = 0; i < PLAYER_COUNT; i++) {
        game->players[i].color = colors[i % color_count];
        printf("%s assigned color: %s\n",
            game->players[i].name,
            civ_color_to_string(game->players[i].color));
    }
}

int main(int argc, char **argv)
{
    Game game;
    StartupOptions options = parse_startup_options(argc, argv);
    MapSize map_size = prompt_map_size();
    RenderConfig render_config = {options.use_color};

    if (!game_init(&game, options.seed, map_size.width, map_size.height)) {
        fprintf(stderr, "Failed to initialize game map.\n");
        return EXIT_FAILURE;
    }
    prompt_civilization_names(&game);
    assign_civilization_colors(&game);
    render_console_run(&game, render_config);
    if (render_config.use_color) {
        printf("\x1b[0m");
    }
    game_free(&game);

    return 0;
}
