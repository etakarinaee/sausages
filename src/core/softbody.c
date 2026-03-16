
// clang-format off
#include "cmath.h"
#include "collision.h"
#include "softbody.h"
#include "renderer.h"
// clang-format on

#include <math.h>
#include <stdlib.h>

struct softbody softbody_create_rect(struct vec2 pos, struct vec2 size,
                                     struct color3 color) {
    struct softbody b;

    int width = (int)size.x;
    int height = (int)size.y;
    int stride = width;

    float rest_len = 30.0f;

    b.point_radius = 15.0f;
    b.damping = 3.0f;
    b.stiffness = 600.0f;
    b.size = size;
    b.frame_angle = 0.0f;
    b.frame_pos = pos;
    b.update_pos = true;

    b.points_count = (int)(size.x * size.y);
    if (b.points_count < 2)
        return b;

    b.points = malloc(b.points_count * sizeof(struct softbody_point));

    int springs_horizontal = (width - 1) * height;
    int springs_vertical = width * (height - 1);
    int springs_diagonal = (width - 1) * (height - 1) * 2;

    b.springs_count = springs_horizontal + springs_vertical + springs_diagonal +
                      b.points_count; /* for frame */
    b.springs = malloc(b.springs_count * sizeof(struct spring));

    /* i know i know very reliable increase here */
    b.edges_count = 2 * (width - 1) + 2 * (height - 1);
    b.edges = malloc(b.edges_count *
                     sizeof(struct edge)); /* quote cheescake: i love edging */
    int edge_idx = 0;
    int point_idx = 0;

    /* point init */
    for (int yi = 0; yi < height; yi++) {
        for (int xi = 0; xi < width; xi++) {
            float x = (xi - (width - 1) * 0.5f) * rest_len;
            float y = (yi - (height - 1) * 0.5f) * rest_len;

            b.points[point_idx] = (struct softbody_point){
                .mass = 1,
                .pos = {.x = pos.x + x, .y = pos.y + y},
                .rest_pos = {.x = x, .y = y},
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
        b.springs[spring_idx++] = (struct spring){
            .rest_len = 0.0f,
            .start = i,
            .end = i, /* doesnt metter because translate from point */
            .end_frame = true,
        };

        /* vertical down */
        if (i + stride < b.points_count) {
            b.springs[spring_idx++] = (struct spring){
                .start = i,
                .end = i + stride,
                .rest_len = rest_len,
            };
        }

        /* horizontal right */
        if (x < width - 1) {
            b.springs[spring_idx++] = (struct spring){
                .start = i,
                .end = i + 1,
                .rest_len = rest_len,
            };
        }

        /* diagonal down */
        if (x < width - 1 && i + stride < b.points_count) {
            b.springs[spring_idx++] = (struct spring){
                .start = i,
                .end = i + stride + 1,
                .rest_len = rest_len * M_SQRT2,
            };
        }

        /* diagonal up */
        if (x < width - 1 && i - stride >= 0) {
            b.springs[spring_idx++] = (struct spring){
                .start = i,
                .end = i - stride + 1,
                .rest_len = rest_len * M_SQRT2,
            };
        }

        /* edges */
        if ((y == 0 || y == height - 1) && x < width - 1) {
            b.edges[edge_idx++] = (struct edge){
                .start = i,
                .end = i + 1,
            };
        }
        if ((x == 0 || x == width - 1) && i + stride < b.points_count) {
            b.edges[edge_idx++] = (struct edge){
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

static void apply_spring_forces(struct softbody *b, struct spring *s) {
    struct softbody_point *start = &b->points[s->start];

    if (s->end_frame) {
        struct vec2 target = math_vec2_add(
            b->frame_pos, math_vec2_rotate(start->rest_pos, b->frame_angle));

        struct vec2 delta = math_vec2_subtract(target, start->pos);
        float dist = math_vec2_length(delta);
        if (dist < 0.0001f)
            return;

        struct vec2 norm = math_vec2_scale(delta, 1.0f / dist);
        float f_spring = dist * b->stiffness * 0.7f;
        struct vec2 spring_f = math_vec2_scale(norm, f_spring);

        float rel_vel_along = -math_vec2_dot(start->vel, norm);
        struct vec2 damp_f =
            math_vec2_scale(norm, rel_vel_along * b->damping * 6.0f);

        start->force =
            math_vec2_add(start->force, math_vec2_add(spring_f, damp_f));
        return;
    }

    struct softbody_point *end = &b->points[s->end];
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

static float softbody_compute_angle(struct softbody *b) {
    struct vec2 sim_center = softbody_get_pos(b);

    float a00 = 0.0f, a01 = 0.0f, a10 = 0.0f, a11 = 0.0f;
    for (int i = 0; i < b->points_count; i++) {
        struct vec2 p = math_vec2_subtract(b->points[i].pos, sim_center);
        struct vec2 q =
            b->points[i].rest_pos; // original offsets, no center needed
        a00 += p.x * q.x;
        a01 += p.x * q.y;
        a10 += p.y * q.x;
        a11 += p.y * q.y;
    }

    return atan2f(a10 - a01, a00 + a11);
}

static void softbody_update_frame(struct softbody *b) {
    if (b->update_pos)
        b->frame_pos = softbody_get_pos(b);
    b->frame_angle = softbody_compute_angle(b);
}

static inline void softbody_update_point(struct softbody_point *p, float dt) {
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

void softbody_update_substep(struct softbody *b, float dt) {
    for (int i = 0; i < b->points_count; i++) {
        b->points[i].force = b->force; /* general outside force */
    }
    for (int i = 0; i < b->springs_count; i++) {
        apply_spring_forces(b, &b->springs[i]);
    }

    for (int i = 0; i < b->points_count; i++) {
        softbody_update_point(&b->points[i], dt);
    }
}

void softbody_update(struct softbody *b, float dt, struct color3 color) {
    const int substeps = 8;
    const float sub_dt = dt / substeps;
    for (int s = 0; s < substeps; s++) {
        softbody_update_frame(b);
        softbody_update_substep(b, sub_dt);
    }

    float vertices[b->points_count * 2];

    for (int i = 0; i < b->points_count; i++) {
        vertices[i * 2] = b->points[i].pos.x;
        vertices[i * 2 + 1] = b->points[i].pos.y;
    }

    renderer_update_mesh(&b->mesh, vertices, b->points_count, color);
    b->force = (struct vec2){0, 0};
    b->update_pos = true;
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

void softbody_check_coll(struct softbody *a, struct softbody *b) {
    for (int i = 0; i < a->points_count; i++) {
        struct softbody_point *a_p = &a->points[i];
        struct vec2 closest_point;
        float t = 0.0f;
        bool first_time = true;
        int edge_idx = -1;

        int intersections = 0;

        for (int j = 0; j < b->edges_count; j++) {
            struct edge *edge = &b->edges[j];
            struct softbody_point *start = &b->points[edge->start];
            struct softbody_point *end = &b->points[edge->end];

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
            struct edge *edge = &b->edges[edge_idx];
            struct softbody_point *start = &b->points[edge->start];
            struct softbody_point *end = &b->points[edge->end];

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

void softbody_apply_velocity(struct softbody *b, struct vec2 vel) {
    for (int i = 0; i < b->points_count; i++) {
        b->points[i].vel = math_vec2_add(b->points[i].vel, vel);
    }
}

void softbody_apply_force(struct softbody *b, struct vec2 force) {
    b->force = math_vec2_add(b->force, force);
}

void softbody_draw(struct softbody *b) {
    renderer_push_mesh(&render_context, b->mesh, (struct vec2){0, 0},
                       (struct vec2){1.0f, 1.0f});

    for (int i = 0; i < b->springs_count; i++) {
        struct vec2 start = b->points[b->springs[i].start].pos;
        struct vec2 end = b->points[b->springs[i].end].pos;
        renderer_push_line(&render_context, start, end, 1);
    }

    for (int i = 0; i < b->points_count; i++) {
        renderer_push_circle(&render_context, b->points[i].pos, 15.0f,
                             (struct color3){0.5f, 0.9f, 0.2f});
    }
}

void softbody_destroy(struct softbody *b) {
    if (b->points)
        free(b->points);
    if (b->springs)
        free(b->springs);
    if (b->edges)
        free(b->edges);
}

struct vec2 softbody_get_pos(struct softbody *b) {
    float x = 0.0f;
    float y = 0.0f;

    if (!b->update_pos)
        return b->frame_pos;
    for (int i = 0; i < b->points_count; i++) {
        x += b->points[i].pos.x;
        y += b->points[i].pos.y;
    }

    x /= b->points_count;
    y /= b->points_count;

    struct vec2 pos = (struct vec2){x, y};
    return pos;
}

void softbody_set_pos(struct softbody *b, struct vec2 pos) {
    b->frame_pos = pos;
    b->update_pos = false;
}
