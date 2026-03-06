
#ifndef GAME_H
#define GAME_H

#include "soft_body.h"

struct game_context {
    struct ph_soft_body *soft_bodies;
    int soft_bodies_index;
};

extern struct game_context game_context;

void game_init(void);
#endif // GAME_H
