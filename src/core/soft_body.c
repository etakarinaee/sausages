
// clang-format off
#include "cmath.h"
#include "collision.h"
#include "soft_body.h"
#include "renderer.h"
// clang-format off

#include <math.h>
#include <stdlib.h>

/* high because of stubsteps */
#define PH_GRAVITY_VEC ((struct vec2){0.0f, -900.0f})

struct ph_soft_body ph_soft_body_create_rect(struct vec2 pos,
                                             struct vec2 size) {
    struct ph_soft_body b;

    int width = (int)size.x;
    int height = (int)size.y;
    int stride = width;

    float rest_len = 30.0f;

    b.point_radius = 15.0f;
    b.damping = 3.0f;
    b.stiffness = 600.0f;

    b.points_count = (int)(size.x * size.y);
    if (b.points_count < 2)
        return b;

    b.points = malloc(b.points_count * sizeof(struct ph_soft_body_point));

    int springs_horizontal = (width - 1) * height;
    int springs_vertical = width * (height - 1);
    int springs_diagonal = (width - 1) * (height - 1) * 2;

    b.springs_count = springs_horizontal + springs_vertical + springs_diagonal;
    b.springs = malloc(b.springs_count * sizeof(struct ph_spring));

    float half_x = size.x * 0.5f;
    float half_y = size.y * 0.5f;
    int point_idx = 0;

    /* point init */
    for (float x = -half_x; x < half_x; x += 1.0f) {
        for (float y = -half_y; y < half_y; y += 1.0f) {
            b.points[point_idx] = (struct ph_soft_body_point){
                .mass = 1,
                .pos =
                    {
                        .x = pos.x + x * rest_len,
                        .y = pos.y + y * rest_len,
                    },
            };

            point_idx++;
        }
    }

    int spring_idx = 0;

    /* spring init */
    for (int i = 0; i < b.points_count; i++) {
        int x = i % width;

        /* vertical down */
        if (i + stride < b.points_count) {
            b.springs[spring_idx++] = (struct ph_spring){
                .start = i,
                .end = i + stride,
                .rest_len = rest_len,
            };
        }

        /* horizontal right */
        if (x < width - 1) {
            b.springs[spring_idx++] = (struct ph_spring){
                .start = i,
                .end = i + 1,
                .rest_len = rest_len,
            };
        }

        /* diagonal down */
        if (x < width - 1 && i + stride < b.points_count) {
            b.springs[spring_idx++] = (struct ph_spring){
                .start = i,
                .end = i + stride + 1,
                .rest_len = rest_len * M_SQRT2,
            };
        }

        /* diagonal up */
        if (x < width - 1 && i - stride >= 0) {
            b.springs[spring_idx++] = (struct ph_spring){
                .start = i,
                .end = i - stride + 1,
                .rest_len = rest_len * M_SQRT2,
            };
        }
    }

    return b;
}

static void ph_apply_spring_forces(struct ph_soft_body *b,
                                   struct ph_spring *s) {
    struct ph_soft_body_point *start = &b->points[s->start];
    struct ph_soft_body_point *end = &b->points[s->end];

    struct vec2 delta = math_vec2_subtract(end->pos, start->pos);
    float dist = math_vec2_length(delta);
    if (dist < 0.0001f)
        return;

    struct vec2 norm = math_vec2_scale(delta, 1.0f / dist);

    float f_spring = (dist - s->rest_len) * b->stiffness;
    struct vec2 spring_f = math_vec2_scale(norm, f_spring);

    float rel_vel_along =
        math_vec2_dot(math_vec2_subtract(end->vel, start->vel), norm);
    struct vec2 damp_f = math_vec2_scale(norm, rel_vel_along * b->damping);

    struct vec2 total = math_vec2_add(spring_f, damp_f);
    start->force = math_vec2_add(start->force, total);
    end->force = math_vec2_subtract(end->force, total);
}

static void ph_soft_body_point_self_collision(struct ph_soft_body *b, int idx,
                                              struct ph_soft_body_point *p) {
    for (int i = 0; i < b->points_count; i++) {
        if (i == idx)
            continue;

        struct ph_soft_body_point *other = &b->points[i];

        struct vec2 delta = math_vec2_subtract(p->pos, other->pos);
        float dist = math_vec2_length(delta);

        if (dist < b->point_radius * 2.0f && dist > 0.0001f) {
            struct vec2 norm = math_vec2_scale(delta, 1.0f / dist);

            struct vec2 rel_vel = math_vec2_subtract(p->vel, other->vel);
            float vel_along_norm = math_vec2_dot(rel_vel, norm);

            if (vel_along_norm < 0.0f) {
                float restitution = 0.3f;
                float impulse = -(1.0f + restitution) * vel_along_norm /
                                (1.0f / p->mass + 1.0f / other->mass);

                struct vec2 impulse_vec = math_vec2_scale(norm, impulse);
                p->vel = math_vec2_add(
                    p->vel, math_vec2_scale(impulse_vec, 1.0f / p->mass));
                other->vel = math_vec2_subtract(
                    other->vel,
                    math_vec2_scale(impulse_vec, 1.0f / other->mass));
            }
        }
    }
}

void ph_soft_body_update_substep(struct ph_soft_body *b, float dt) {
    for (int i = 0; i < b->points_count; i++) {
        b->points[i].force = PH_GRAVITY_VEC;
    }
    for (int i = 0; i < b->springs_count; i++) {
        ph_apply_spring_forces(b, &b->springs[i]);
    }

    for (int i = 0; i < b->points_count; i++) {
        struct ph_soft_body_point *p = &b->points[i];
        p->vel = math_vec2_add(p->vel, math_vec2_scale(p->force, dt / p->mass));
        p->pos = math_vec2_add(p->pos, math_vec2_scale(p->vel, dt));

        if (coll_check_point_rect(p->pos, (struct vec2){-400, -50},
                                  (struct vec2){800, 100})) {
            p->pos.y = 50.0f;
            p->vel.y *= -0.3f;
        }

        ph_soft_body_point_self_collision(b, i, p);
        p->vel = math_vec2_scale(p->vel, 0.999f);
    }
}

void ph_soft_body_update(struct ph_soft_body *b, float dt) {
    const int substeps = 8;
    const float sub_dt = dt / substeps;
    for (int s = 0; s < substeps; s++) {
        ph_soft_body_update_substep(b, sub_dt);
    }
}

void ph_soft_body_apply_velocity(struct ph_soft_body *b, struct vec2 vel) {
     for (int i = 0; i < b->points_count; i++) {
         b->points[i].vel = math_vec2_add(b->points[i].vel, vel);
     }   
}

void ph_soft_body_draw(struct ph_soft_body *b) {
    for (int i = 0; i < b->points_count; i++) {
        renderer_push_circle(&render_context, b->points[i].pos, b->point_radius,
                             (struct color3){1.0f, 0.0f, 0.0f});
    }
}

void ph_soft_body_destroy(struct ph_soft_body *b) {
    if (b->points)
        free(b->points);
    if (b->springs)
        free(b->springs);
}
