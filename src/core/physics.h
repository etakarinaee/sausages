
#ifndef PHYSICS_H
#define PHYSICS_H

#include "cmath.h"

struct ph_soft_body_point {
    struct vec2 pos;
    struct vec2 vel;
    struct vec2 force;
    float mass;
};

struct ph_spring {
    struct ph_soft_body_point start;
    struct ph_soft_body_point end;
    float stiffness;
    float rest_len;
    float damping;
};

void ph_upadte_soft_body_point(struct ph_soft_body_point *p, float dt);
struct vec2 ph_get_spring_force(struct ph_spring *s);

#endif // PHYSICS_H
