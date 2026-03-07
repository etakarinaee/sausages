
// clang-format off
#include "cmath.h"
#include "collision.h"
#include "soft_body.h"
#include "renderer.h"
// clang-format off

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/* high because of stubsteps */
#define PH_GRAVITY_VEC ((struct vec2){0.0f, -900.0f})

struct ph_soft_body ph_soft_body_create_rect(struct vec2 pos,
                                             struct vec2 size, struct color3 color) {
    struct ph_soft_body b;

    int width = (int)size.x;
    int height = (int)size.y;
    int stride = width;

    float rest_len = 30.0f;

    b.point_radius = 15.0f;
    b.damping = 3.0f;
    b.stiffness = 600.0f;
    b.size = size;

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

            if ((int)x == (int)-half_x || (int)x == (int)half_x - 1 ||
                (int)y == (int)-half_y || (int)y == (int)half_y -1) {
                 b.points[point_idx].is_outer = true;               
            }
            
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

    float vertices[b.points_count * 2];
    int indices_count = (width - 1) * (height - 1) * 6;
    uint32_t indices[indices_count];
    int indices_idx = 0;

    for (int i = 0; i < b.points_count; i++) {

        vertices[i * 2] = b.points[i].pos.x;
        vertices[i * 2 + 1] = b.points[i].pos.y;

        int x = i % width;

        if (x < width - 1 && i + stride < b.points_count) {

            // triangle 1
            indices[indices_idx++] = i;
            indices[indices_idx++] = i + 1;
            indices[indices_idx++] = i + stride;

            // triangle 2
            indices[indices_idx++] = i + 1;
            indices[indices_idx++] = i + stride + 1;
            indices[indices_idx++] = i + stride;
        }
    }     
    b.mesh = renderer_create_mesh(vertices, b.points_count, indices, indices_count, color);
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

void ph_soft_body_update(struct ph_soft_body *b, float dt, struct color3 color) {
    const int substeps = 8;
    const float sub_dt = dt / substeps;
    for (int s = 0; s < substeps; s++) {
        ph_soft_body_update_substep(b, sub_dt);
    }

    
    float vertices[b->points_count * 2];

    for (int i = 0; i < b->points_count; i++) {
        vertices[i * 2] = b->points[i].pos.x;
        vertices[i * 2 + 1] = b->points[i].pos.y;
    }

    renderer_update_mesh(&b->mesh, vertices, b->points_count, color);
}

static struct vec2 closest_point_on_line(struct vec2 start, struct vec2 end, struct vec2 p, float *out_t) {
    struct vec2 edge_delta = math_vec2_subtract(end, start);
    struct vec2 delta = math_vec2_subtract(p, start);

    float t = math_vec2_dot(delta, edge_delta) / math_vec2_dot(edge_delta, edge_delta);
    t = math_clamp(t, 0.0f, 1.0f);

    if (out_t) *out_t = t;

    return math_vec2_add(start, math_vec2_scale(edge_delta, t));
}

void ph_soft_body_check_coll(struct ph_soft_body *a, struct ph_soft_body *b) {
    for (int i = 0; i < a->points_count; i++) {
        struct ph_soft_body_point *a_p = &a->points[i];
        if (!a_p->is_outer) continue;
                
        struct vec2 edge_start;
        int edge_start_idx;
        struct vec2 edge_end;
        int edge_end_idx;
        int edge_point_count = 0;
        struct vec2 closest_point;
        float t = 0.0f;
        bool first_time = true;

        int intersections = 0;

        for (int j = 0; j < b->points_count; j++) {
            struct ph_soft_body_point *p = &b->points[j];
            if (p->is_outer) {
                struct vec2 *edge_p = edge_point_count == 0 ? &edge_start : &edge_end;
                int *edge_idx = edge_point_count == 0 ? &edge_start_idx : &edge_end_idx;
                
                *edge_p = p->pos;
                *edge_idx = j;
                edge_point_count++;    
            }

            /* found edge */
            if (edge_point_count == 2) {
                edge_point_count = 0;

                struct vec2 out_p = a_p->pos;
                out_p.x += b->size.x + 2;  /* +2 for extra safety */

                if ((edge_start.y > a_p->pos.y) != (edge_end.y > a_p->pos.y) &&
                    a_p->pos.x < (edge_end.x - edge_start.x) * (a_p->pos.y - edge_start.y) / (edge_end.y - edge_start.y) + edge_start.x) {
                    intersections++;
                }

                float out_t = 0.0f;
                struct vec2 p_on_line = closest_point_on_line(edge_start, edge_end, a_p->pos, &out_t);
                if (first_time || math_vec2_distance(a_p->pos, closest_point) > math_vec2_distance(a_p->pos, p_on_line)) {
                    closest_point = p_on_line;
                    t = out_t;
                }

                first_time = false;
            }
        }

        if (intersections % 2 == 1) {
            struct vec2 push = math_vec2_subtract(closest_point, a_p->pos);
            struct vec2 normal = math_vec2_norm(push);
            const float e = 1.0f;
            float vn = math_vec2_dot(a_p->vel, normal);
            struct vec2 v_normal = math_vec2_scale(normal, vn);
            struct vec2 v_tangent = math_vec2_subtract(a_p->vel, v_normal);
            
            a_p->pos = math_vec2_add(a_p->pos, push);
            b->points[edge_end_idx].pos = math_vec2_subtract(b->points[edge_end_idx].pos, math_vec2_scale(push, t));
            b->points[edge_start_idx].pos = math_vec2_add(b->points[edge_start_idx].pos,  math_vec2_scale(push, 1.0f - t));

            struct vec2 v_new = math_vec2_subtract(v_tangent, math_vec2_scale(v_normal, e));
            a_p->vel = v_new;

            struct vec2 v_edge = math_vec2_scale(push, 1 - t);
            a_p->vel = math_vec2_subtract(a_p->vel, v_edge);
            
            b->points[edge_start_idx].vel = math_vec2_add(b->points[edge_start_idx].vel, math_vec2_scale(v_edge, 1.0f - t));
            b->points[edge_end_idx].vel = math_vec2_add(b->points[edge_end_idx].vel, math_vec2_scale(v_edge, t));
         }
    }
}

void ph_soft_body_apply_velocity(struct ph_soft_body *b, struct vec2 vel) {
     for (int i = 0; i < b->points_count; i++) {
         b->points[i].vel = math_vec2_add(b->points[i].vel, vel);
     }   
}

void ph_soft_body_draw(struct ph_soft_body *b) {
    renderer_push_mesh(&render_context, b->mesh, (struct vec2){0, 0}, (struct vec2){1.0f, 1.0f});
}

void ph_soft_body_destroy(struct ph_soft_body *b) {
    if (b->points)
        free(b->points);
    if (b->springs)
        free(b->springs);
}
