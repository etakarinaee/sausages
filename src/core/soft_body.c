
#include "soft_body.h"
#include "cmath.h"
#include "renderer.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define PH_GRAVITY_VEC ((struct vec2){0.0f, -100.0f})

struct ph_soft_body ph_soft_body_create_rect(struct vec2 pos, struct vec2 size) {
    struct ph_soft_body b;

    int width = (int)size.x;
    int height = (int)size.y;
    int stride = width;

    float rest_len = 30.0f;

    b.damping = 0.9f;
    b.stiffness = 1.0f;

    b.points_count = (int)(size.x * size.y);
    if (b.points_count < 2 || b.points_count % 2 == 1) return b;

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
                .pos = {
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

static float ph_get_spring_force(struct ph_soft_body *b, struct ph_spring *s) {
    struct ph_soft_body_point start = b->points[s->start];
    struct ph_soft_body_point end  = b->points[s->end];

    struct vec2 delta = math_vec2_subtract(end.pos, start.pos);
    float dist = math_vec2_length(delta);

    if (dist < 0.0001f) {
        return 0.0f;
    }

    struct vec2 norm = math_vec2_scale(delta, 1.0f / dist);
    float f_spring = (dist - s->rest_len) * b->stiffness;
    float f_damp = math_vec2_dot(norm, math_vec2_subtract(end.vel, start.vel)) * b->damping;

    return f_spring + f_damp;
}

static void ph_soft_body_update_point(struct ph_soft_body *b, int idx, struct ph_soft_body_point *p, float dt) {
    p->force = (struct vec2){0, 0};

    p->force = math_vec2_add(p->force, PH_GRAVITY_VEC);

    /* TODO: optimize this by creation a spring children table in point or smth */
    for (int i = 0; i < b->springs_count; i++) {
        if (idx == b->springs[i].start || idx == b->springs[i].end) {
            struct ph_spring *s = &b->springs[i];
            float spring_force = ph_get_spring_force(b, s);

            struct ph_soft_body_point start = b->points[s->start];
            struct ph_soft_body_point end = b->points[s->end];

            struct vec2 spring_f;
            if (idx == b->springs[i].end) {
                spring_f = math_vec2_scale(math_vec2_norm(math_vec2_subtract(start.pos, end.pos)), spring_force);
            }
            else {
                spring_f = math_vec2_scale(math_vec2_norm(math_vec2_subtract(end.pos, start.pos)), spring_force);
            }
            p->force = math_vec2_add(p->force, spring_f);
        }
    }

    p->vel = math_vec2_add(p->vel, math_vec2_scale(math_vec2_scale(p->force, dt), 1.0f / p->mass));
    p->pos = math_vec2_add(p->pos, math_vec2_scale(p->vel, dt));
}

void ph_soft_body_update(struct ph_soft_body *b, float dt) {
    for (int i = 0; i < b->points_count; i++) {
        ph_soft_body_update_point(b, i, &b->points[i], dt);
    }
}

void ph_soft_body_draw(struct ph_soft_body *b) {
    for (int i = 0; i < b->points_count; i++) {
        renderer_push_circle(&render_context, b->points[i].pos, 15.0f, (struct color3){1.0f, 0.0f, 0.0f});
    }
}

void ph_soft_body_destroy(struct ph_soft_body *b) {
    if (b->points) free(b->points);
    if (b->springs) free(b->springs);
}


