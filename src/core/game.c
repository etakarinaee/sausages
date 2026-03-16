
#include "game.h"
#include "softbody.h"
#include <stdlib.h>

struct game_context game_context;

void game_init(void) {
    /* TODO: change to a dynamic array */
    game_context.soft_bodies = malloc(100 * sizeof(struct softbody));
    game_context.soft_bodies_index = 0;

    /* TODO: change to a dynamic array */
    game_context.meshs = malloc(100 * sizeof(struct mesh));
    game_context.meshs_index = 0;
}

void game_deinit(void) {
    if (game_context.soft_bodies) {
        for (int i = 0; i < game_context.soft_bodies_index; i++) {
            softbody_destroy(&game_context.soft_bodies[i]);
        }
        free(game_context.soft_bodies);
    }
    if (game_context.meshs)
        free(game_context.meshs);
}
