
#include "game.h"
#include "soft_body.h"
#include <stdlib.h>

struct game_context game_context;

void game_init(void) {
    /* TODO: change to a dynamic array */
    game_context.soft_bodies = malloc(100 * sizeof(struct ph_soft_body));
    game_context.soft_bodies_index = 0;
}
