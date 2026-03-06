
#ifndef SOFT_BODY_H
#define SOFT_BODY_H

#include "cmath.h"

struct ph_soft_body_point {
    struct vec2 pos;
    struct vec2 vel;
    struct vec2 force;
    float mass;
};

struct ph_spring {
    /* handles to ph_soft_body_point */
    int start;
    int end;
    float rest_len;
};

struct ph_soft_body {
    int points_count;
    int springs_count;
    struct ph_soft_body_point *points;
    struct ph_spring *springs;

    float stiffness;
    float damping;
};

struct ph_soft_body ph_soft_body_create_rect(struct vec2 pos, struct vec2 size);
void ph_soft_body_update(struct ph_soft_body *b, float dt);
void ph_soft_body_draw(struct ph_soft_body *b);
void ph_soft_body_destroy(struct ph_soft_body *b);

#endif // SOFT_BODY_H
