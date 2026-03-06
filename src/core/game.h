
#ifndef GAME_H
#define GAME_H

#include "renderer.h"
#include "soft_body.h"

struct game_context {
    struct ph_soft_body *soft_bodies;
    int soft_bodies_index;
    struct mesh *meshs;
    int meshs_index;
};

extern struct game_context game_context;

void game_init(void);
void game_deinit(void);

#endif // GAME_H
