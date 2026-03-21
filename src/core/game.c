
#include "game.h"
#include <stdlib.h>

struct game_context game_context;

void game_init(void) {
    /* TODO: change to a dynamic array */
    game_context.meshs = malloc(100 * sizeof(struct mesh));
    game_context.meshs_index = 0;
}

void game_deinit(void) {
    if (game_context.meshs)
        free(game_context.meshs);
}
