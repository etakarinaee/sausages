
// clang-format off
#include "cmath.h"
#include "collision.h"
#include "soft_body.h"
#include "renderer.h"
// clang-format on

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct ph_soft_body ph_soft_body_create_rect(struct vec2 pos, struct vec2 size,
                                             struct color3 color) {
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

    b.frame_points = malloc(b.points_count * sizeof(struct ph_soft_body_point));

    int springs_horizontal = (width - 1) * height;
    int springs_vertical = width * (height - 1);
    int springs_diagonal = (width - 1) * (height - 1) * 2;

    b.springs_count = springs_horizontal + springs_vertical + springs_diagonal +
                      b.points_count; /* for frame */
    b.springs = malloc(b.springs_count * sizeof(struct ph_spring));

    /* i know i know very reliable increase here */
    b.edges_count = 2 * (width - 1) + 2 * (height - 1);
    b.edges =
        malloc(b.edges_count *
               sizeof(struct ph_edge)); /* quote cheescake: i love edging */
    int edge_idx = 0;

    float half_x = size.x * 0.5f;
    float half_y = size.y * 0.5f;
    int point_idx = 0;

    /* point init */
    for (float y = -half_y; y < half_y; y += 1.0f) {
        for (float x = -half_x; x < half_x; x += 1.0f) {
            b.points[point_idx] = (struct ph_soft_body_point){
                .mass = 1,
                .pos =
                    {
                        .x = pos.x + x * rest_len,
                        .y = pos.y + y * rest_len,
                    },
            };

            b.frame_points[point_idx] = (struct ph_soft_body_point){
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
        int y = i / width;

        /* attach spring from point to frame point */
        b.springs[spring_idx++] = (struct ph_spring){
            .rest_len = 0.0f,
            .start = i,
            .end = i,
            .end_frame = true,
        };

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

        /* edges */
        if ((y == 0 || y == height - 1) && x < width - 1) {
            b.edges[edge_idx++] = (struct ph_edge){
                .start = i,
                .end = i + 1,
            };
        }
        if ((x == 0 || x == width - 1) && i + stride < b.points_count) {
            b.edges[edge_idx++] = (struct ph_edge){
                .start = i,
                .end = i + stride,
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
    b.mesh = renderer_create_mesh(vertices, b.points_count, indices,
                                  indices_count, color);
    return b;
}

static void ph_apply_spring_forces(struct ph_soft_body *b,
                                   struct ph_spring *s) {
    struct ph_soft_body_point *start = &b->points[s->start];

    struct ph_soft_body_point *end =
        s->end_frame ? &b->frame_points[s->end] : &b->points[s->end];

    struct vec2 delta = math_vec2_subtract(end->pos, start->pos);
    float dist = math_vec2_length(delta);
    if (dist < 0.0001f)
        return;

    struct vec2 norm = math_vec2_scale(delta, 1.0f / dist);

    float stiff_coef =
        s->end_frame ? 0.7f : 1.0f; /* not as strong springs for frame */
    float damp_coef = s->end_frame ? 6.0f : 1.0f;
    float f_spring = (dist - s->rest_len) * b->stiffness * stiff_coef;
    struct vec2 spring_f = math_vec2_scale(norm, f_spring);

    float rel_vel_along =
        math_vec2_dot(math_vec2_subtract(end->vel, start->vel), norm);
    struct vec2 damp_f =
        math_vec2_scale(norm, rel_vel_along * b->damping * damp_coef);

    struct vec2 total = math_vec2_add(spring_f, damp_f);
    start->force = math_vec2_add(math_vec2_add(b->force, start->force), total);
    if (!s->end_frame)
        end->force = math_vec2_subtract(end->force, total);
}

static float ph_soft_body_compute_angle(struct ph_soft_body *b) {
    struct vec2 frame_center = ph_soft_body_get_pos(b, SOFT_BODY_FRAME);
    struct vec2 sim_center = ph_soft_body_get_pos(b, SOFT_BODY_POS);

    float a00 = 0.0f, a01 = 0.0f, a10 = 0.0f, a11 = 0.0f;
    for (int i = 0; i < b->points_count; i++) {
        struct vec2 p = math_vec2_subtract(b->points[i].pos, sim_center);
        struct vec2 q =
            math_vec2_subtract(b->frame_points[i].pos, frame_center);
        a00 += p.x * q.x;
        a01 += p.x * q.y;
        a10 += p.y * q.x;
        a11 += p.y * q.y;
    }

    float angle = atan2f(a10 - a01, a00 + a11);
    return angle;
}

static float compute_moment(struct ph_soft_body_point *p,
                            struct vec2 frame_center) {
    float distance = math_vec2_distance(frame_center, p->pos);
    float angle = math_vec2_angle(p->pos, frame_center);
    float moment = math_vec2_length(p->force) * distance * sinf(angle);
    return moment;
}

static float get_angle(struct ph_soft_body *b) {
    // rotational engery
    struct vec2 frame_center = ph_soft_body_get_pos(b, SOFT_BODY_FRAME);
    float moment = 0.0f;
    for (int i = 0; i < b->points_count; i++) {
        moment += compute_moment(&b->points[i], frame_center);
    }
    moment /= b->points_count;

    // mass
    float mass = 0.0f;
    for (int i = 0; i < b->points_count; i++) {
        mass += b->points[i].mass;
    }

    // printf("Moment %.4f\n", moment);
    // printf("Mass: %.4f\n", mass);

    return moment / mass;
}

static void ph_soft_body_transform_frame(struct ph_soft_body *b) {
    float angle = ph_soft_body_compute_angle(b);
    struct vec2 frame_center = ph_soft_body_get_pos(b, SOFT_BODY_FRAME);
    struct vec2 sim_center = ph_soft_body_get_pos(b, SOFT_BODY_POS);

    for (int i = 0; i < b->points_count; i++) {
        struct vec2 *p = &b->frame_points[i].pos;
        struct vec2 delta = math_vec2_subtract(*p, frame_center);

        float angle1 = get_angle(b);
        // printf("Angle: %.4f\n", angle1);

        struct matrix m;
        math_matrix_rotate_2d(&m, angle);
        delta = math_vec2_mul_matrix(delta, &m);

        *p = math_vec2_add(sim_center, delta);
    }
}

static inline void ph_soft_body_update_point(struct ph_soft_body_point *p,
                                             float dt) {
    p->vel = math_vec2_add(p->vel, math_vec2_scale(p->force, dt / p->mass));
    p->pos = math_vec2_add(p->pos, math_vec2_scale(p->vel, dt));

    /* TODO: make this not hardcoded */
    if (coll_check_point_rect(p->pos, (struct vec2){-400, -50},
                              (struct vec2){800, 100})) {
        p->pos.y = 50.0f;
        p->vel.y *= -0.3f;
    }

    p->vel = math_vec2_scale(p->vel, 0.999f);
}

void ph_soft_body_update_substep(struct ph_soft_body *b, float dt) {
    for (int i = 0; i < b->points_count; i++) {
        b->points[i].force =
            (struct vec2){0.0f, 0.0f}; /* general outside force */
    }
    for (int i = 0; i < b->springs_count; i++) {
        ph_apply_spring_forces(b, &b->springs[i]);
    }

    for (int i = 0; i < b->points_count; i++) {
        ph_soft_body_update_point(&b->points[i], dt);
    }
}

void ph_soft_body_update(struct ph_soft_body *b, float dt,
                         struct color3 color) {
    const int substeps = 8;
    const float sub_dt = dt / substeps;
    for (int s = 0; s < substeps; s++) {
        ph_soft_body_update_substep(b, sub_dt);
    }

    ph_soft_body_transform_frame(b);
    b->update_pos = true;
    b->update_frame_pos = true;

    float vertices[b->points_count * 2];

    for (int i = 0; i < b->points_count; i++) {
        vertices[i * 2] = b->points[i].pos.x;
        vertices[i * 2 + 1] = b->points[i].pos.y;
    }

    renderer_update_mesh(&b->mesh, vertices, b->points_count, color);
    b->force = (struct vec2){0, 0};
}

static struct vec2 closest_point_on_line(struct vec2 start, struct vec2 end,
                                         struct vec2 p, float *out_t) {
    struct vec2 edge_delta = math_vec2_subtract(end, start);
    struct vec2 delta = math_vec2_subtract(p, start);

    float t = math_vec2_dot(delta, edge_delta) /
              math_vec2_dot(edge_delta, edge_delta);
    t = math_clamp(t, 0.0f, 1.0f);

    if (out_t)
        *out_t = t;

    return math_vec2_add(start, math_vec2_scale(edge_delta, t));
}

void ph_soft_body_check_coll(struct ph_soft_body *a, struct ph_soft_body *b) {
    for (int i = 0; i < a->points_count; i++) {
        struct ph_soft_body_point *a_p = &a->points[i];
        struct vec2 closest_point;
        float t = 0.0f;
        bool first_time = true;
        int edge_idx = -1;

        int intersections = 0;

        for (int j = 0; j < b->edges_count; j++) {
            struct ph_edge *edge = &b->edges[j];
            struct ph_soft_body_point *start = &b->points[edge->start];
            struct ph_soft_body_point *end = &b->points[edge->end];

            if ((start->pos.y > a_p->pos.y) != (end->pos.y > a_p->pos.y) &&
                a_p->pos.x < (end->pos.x - start->pos.x) *
                                     (a_p->pos.y - start->pos.y) /
                                     (end->pos.y - start->pos.y) +
                                 start->pos.x) {
                intersections++;
            }

            float out_t = 0.0f;
            struct vec2 p_on_line =
                closest_point_on_line(start->pos, end->pos, a_p->pos, &out_t);
            if (first_time || math_vec2_distance(a_p->pos, closest_point) >
                                  math_vec2_distance(a_p->pos, p_on_line)) {
                closest_point = p_on_line;
                t = out_t;
                edge_idx = j;
            }

            first_time = false;
        }

        if (intersections % 2 == 1) {
            struct ph_edge *edge = &b->edges[edge_idx];
            struct ph_soft_body_point *start = &b->points[edge->start];
            struct ph_soft_body_point *end = &b->points[edge->end];

            struct vec2 push = math_vec2_subtract(closest_point, a_p->pos);
            struct vec2 normal = math_vec2_norm(push);
            const float e = 1.0f;
            float vn = math_vec2_dot(a_p->vel, normal);
            struct vec2 v_normal = math_vec2_scale(normal, vn);
            struct vec2 v_tangent = math_vec2_subtract(a_p->vel, v_normal);

            a_p->pos = math_vec2_add(a_p->pos, push);
            end->pos = math_vec2_subtract(end->pos, math_vec2_scale(push, t));
            start->pos =
                math_vec2_add(start->pos, math_vec2_scale(push, 1.0f - t));

            struct vec2 v_new =
                math_vec2_subtract(v_tangent, math_vec2_scale(v_normal, e));
            a_p->vel = v_new;

            struct vec2 v_edge = math_vec2_scale(push, 1 - t);
            a_p->vel = math_vec2_subtract(a_p->vel, v_edge);

            start->vel =
                math_vec2_add(start->vel, math_vec2_scale(v_edge, 1.0f - t));
            end->vel = math_vec2_add(end->vel, math_vec2_scale(v_edge, t));
        }
    }
}

void ph_soft_body_apply_velocity(struct ph_soft_body *b, struct vec2 vel) {
    for (int i = 0; i < b->points_count; i++) {
        b->points[i].vel = math_vec2_add(b->points[i].vel, vel);
    }
}

void ph_soft_body_apply_force(struct ph_soft_body *b, struct vec2 force) {
    b->force = math_vec2_add(b->force, force);
}

void ph_soft_body_draw(struct ph_soft_body *b) {
    renderer_push_mesh(&render_context, b->mesh, (struct vec2){0, 0},
                       (struct vec2){1.0f, 1.0f});

    /*
        for (int i = 0; i < b->points_count; i++) {
            renderer_push_circle(&render_context, b->points[i].pos, 15.0f,
                                 (struct color3){0.5f, 0.9f, 0.2f});
            renderer_push_circle(&render_context, b->frame_points[i].pos, 15.0f,
                                 (struct color3){0.9f, 0.9f, 0.2f});
        }
        */
}

void ph_soft_body_destroy(struct ph_soft_body *b) {
    if (b->points)
        free(b->points);
    if (b->springs)
        free(b->springs);
    if (b->edges)
        free(b->edges);
    if (b->frame_points)
        free(b->frame_points);
}

struct vec2 ph_soft_body_get_pos(struct ph_soft_body *b, int type) {
    float x = 0.0f;
    float y = 0.0f;

    switch (type) {
    case SOFT_BODY_POS:
        if (!b->update_pos)
            return b->pos;
        for (int i = 0; i < b->points_count; i++) {
            x += b->points[i].pos.x;
            y += b->points[i].pos.y;
        }
        break;
    case SOFT_BODY_FRAME:
        if (!b->update_frame_pos)
            return b->frame_pos;
        for (int i = 0; i < b->points_count; i++) {
            x += b->frame_points[i].pos.x;
            y += b->frame_points[i].pos.y;
        }
        break;
    }

    struct vec2 pos = type == SOFT_BODY_POS ? b->pos : b->frame_pos;

    x /= b->points_count;
    y /= b->points_count;

    pos = (struct vec2){x, y};
    struct vec2 *change = type == SOFT_BODY_POS ? &b->pos : &b->frame_pos;
    bool *update =
        type == SOFT_BODY_POS ? &b->update_pos : &b->update_frame_pos;
    *change = pos;
    *update = false;

    return pos;
}

static void update_pos(struct ph_soft_body *b, int type) {
    struct vec2 prev_center = ph_soft_body_get_pos(b, type);
    struct vec2 center = type == SOFT_BODY_POS ? b->pos : b->frame_pos;

    for (int i = 0; i < b->points_count; i++) {
        struct ph_soft_body_point *p =
            type == SOFT_BODY_POS ? &b->points[i] : &b->frame_points[i];
        struct vec2 delta = math_vec2_subtract(p->pos, prev_center);
        p->pos = math_vec2_add(center, math_vec2_scale(delta, -1));
    }
}

void ph_soft_body_set_pos(struct ph_soft_body *b, struct vec2 pos, int type) {
    switch (type) {
    case SOFT_BODY_POS:
        b->pos = pos;
        b->update_pos = false;
        update_pos(b, SOFT_BODY_POS);
        break;
    case SOFT_BODY_FRAME:
        b->frame_pos = pos;
        b->update_frame_pos = false;
        update_pos(b, SOFT_BODY_FRAME);
        break;
    }
}
