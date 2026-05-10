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
                fprintf(stderr, "Warning: ignoring unknown argument '%s'. Use an unsigned seed, --color, or --no-color.\n", argv[i]);
                continue;
            }
            options.seed = (unsigned int)parsed;
            seed_set = 1;
        } else {
            fprintf(stderr, "Warning: ignoring extra argument '%s'. Use one seed plus optional --color or --no-color.\n", argv[i]);
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

static const char *map_size_name(MapSize size)
{
    if (size.width == 32) {
        return "small";
    }
    if (size.width == 48) {
        return "medium";
    }
    if (size.width == 96) {
        return "xlarge";
    }
    return "large";
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
        printf("Enter map size [large]: ");
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

static int parse_game_mode(const char *text, GameMode *mode)
{
    if (strcmp(text, "") == 0 || strcmp(text, "1") == 0 || strcmp(text, "standard") == 0) {
        *mode = GAME_MODE_STANDARD;
    } else if (strcmp(text, "2") == 0 || strcmp(text, "debug") == 0) {
        *mode = GAME_MODE_DEBUG;
    } else {
        return 0;
    }

    return 1;
}

static GameMode prompt_game_mode(void)
{
    char input[64];
    GameMode mode;

    for (;;) {
        printf("\nChoose game mode:\n");
        printf("1. standard - normal play, no debug commands\n");
        printf("2. debug    - normal play plus spawn/test commands\n\n");
        printf("Enter game mode [standard]: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return GAME_MODE_STANDARD;
        }
        if (strchr(input, '\n') == NULL) {
            discard_remaining_line();
        }
        trim_newline(input);
        to_lowercase(input);
        if (parse_game_mode(input, &mode)) {
            return mode;
        }

        printf("Invalid game mode. Enter 1, 2, standard, or debug.\n");
    }
}

static int prompt_player_count(void)
{
    char input[64];

    for (;;) {
        printf("\nChoose number of human players:\n");
        for (int i = MIN_PLAYERS; i <= MAX_PLAYERS; i++) {
            printf("%d. %d players\n", i, i);
        }
        printf("Enter number of players [2]: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return MIN_PLAYERS;
        }
        if (strchr(input, '\n') == NULL) {
            discard_remaining_line();
        }
        trim_newline(input);
        if (input[0] == '\0') {
            return MIN_PLAYERS;
        }

        errno = 0;
        char *end = NULL;
        long parsed = strtol(input, &end, 10);
        if (errno == 0 && end != input && *end == '\0' && parsed >= MIN_PLAYERS && parsed <= MAX_PLAYERS) {
            return (int)parsed;
        }

        printf("Invalid player count. Enter a number from %d to %d.\n", MIN_PLAYERS, MAX_PLAYERS);
    }
}

static void prompt_civilization_names(char names[MAX_PLAYERS][PLAYER_NAME_LENGTH], int player_count)
{
    char input[PLAYER_NAME_LENGTH];

    for (int i = 0; i < player_count; i++) {
        printf("Enter name for Civilization %d [Player %d]: ", i + 1, i + 1);
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            snprintf(names[i], PLAYER_NAME_LENGTH, "Player %d", i + 1);
            continue;
        }
        if (strchr(input, '\n') == NULL) {
            discard_remaining_line();
        }
        trim_newline(input);

        if (input[0] == '\0') {
            snprintf(names[i], PLAYER_NAME_LENGTH, "Player %d", i + 1);
        } else {
            snprintf(names[i], PLAYER_NAME_LENGTH, "%s", input);
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

    for (int i = 0; i < game->player_count; i++) {
        game->players[i].color = colors[i % color_count];
        printf("Player %d %s assigned color: %s%s\n",
            game->players[i].id,
            game->players[i].name,
            civ_color_to_string(game->players[i].color),
            i >= color_count ? " (reused)" : "");
    }
}

static const char *game_mode_to_string(GameMode mode)
{
    return mode == GAME_MODE_DEBUG ? "debug" : "standard";
}

int main(int argc, char **argv)
{
    Game game;
    StartupOptions options = parse_startup_options(argc, argv);
    MapSize map_size = prompt_map_size();
    GameMode mode = prompt_game_mode();
    int player_count = prompt_player_count();
    char names[MAX_PLAYERS][PLAYER_NAME_LENGTH];
    RenderConfig render_config = {options.use_color};

    prompt_civilization_names(names, player_count);

    if (!game_init(&game, options.seed, map_size.width, map_size.height, player_count, mode)) {
        fprintf(stderr, "Failed to initialize game map.\n");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < player_count; i++) {
        snprintf(game.players[i].name, PLAYER_NAME_LENGTH, "%s", names[i]);
    }

    printf("\nSeed: %u\n", game.seed);
    printf("Map size: %s (%dx%d)\n", map_size_name(map_size), map_size.width, map_size.height);
    printf("Game mode: %s\n", game_mode_to_string(mode));
    printf("Human players: %d\n", player_count);
    assign_civilization_colors(&game);
    render_console_run(&game, render_config);
    if (render_config.use_color) {
        printf("\x1b[0m");
    }
    game_free(&game);

    return 0;
}
