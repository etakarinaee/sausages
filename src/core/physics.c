
#include "physics.h"
#include "cmath.h"

#define PH_GRAVITY_VEC ((struct vec2){0.0f, -9.81f})

void ph_upadte_soft_body_point(struct ph_soft_body_point *p, float dt) {
    p->force = (struct vec2){0, 0};

    p->force = math_vec2_add(p->force, PH_GRAVITY_VEC);
    p->vel = math_vec2_add(p->vel, math_vec2_scale(math_vec2_scale(p->force, dt), 1.0f / p->mass));
    p->pos = math_vec2_add(p->pos, math_vec2_scale(p->vel, dt));
}

struct vec2 ph_get_spring_force(struct ph_spring *s) {
    struct vec2 norm = math_vec2_norm(math_vec2_subtract(s->end.pos, s->end.pos));
    struct vec2 f_spring = math_vec2_scale(norm, (math_vec2_distance(s->end.pos, s->start.pos) - s->rest_len) * s->stiffness);
    struct vec2 f_damp = math_vec2_scale(math_vec2_subtract(s->end.vel, s->start.vel), math_vec2_dot(s->end.pos, s->start.pos) * s->damping);

    return math_vec2_add(f_spring, f_damp);
}

