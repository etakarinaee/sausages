
#ifndef SOFT_BODY_H
#define SOFT_BODY_H

#include "cmath.h"
#include "renderer.h"

enum {
    SOFT_BODY_POS,
    SOFT_BODY_FRAME,
};

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
    bool end_frame; /* true if end handle is a frame point */
};

struct ph_edge {
    int start;
    int end;
};

struct ph_soft_body {
    int points_count;
    int springs_count;
    int edges_count;
    struct ph_soft_body_point *points;
    struct ph_spring *springs;
    struct ph_edge *edges;
    struct ph_soft_body_point *frame_points;

    float point_radius;
    float stiffness;
    float damping;

    struct vec2 frame_pos;
    struct vec2 pos;
    bool update_frame_pos; /* true when position need to be pulled again */
    bool update_pos;

    struct vec2 force; /* acumalive outside force acting on the hole object */

    struct vec2 size;
    struct mesh mesh;
};

struct ph_soft_body ph_soft_body_create_rect(struct vec2 pos, struct vec2 size,
                                             struct color3 color);
void ph_soft_body_update(struct ph_soft_body *b, float dt, struct color3 color);
void ph_soft_body_check_coll(struct ph_soft_body *a, struct ph_soft_body *b);
void ph_soft_body_apply_velocity(struct ph_soft_body *b, struct vec2 vel);
void ph_soft_body_apply_force(struct ph_soft_body *b, struct vec2 force);
void ph_soft_body_draw(struct ph_soft_body *b);
void ph_soft_body_destroy(struct ph_soft_body *b);

struct vec2 ph_soft_body_get_pos(struct ph_soft_body *b, int type);
void ph_soft_body_set_pos(struct ph_soft_body *b, struct vec2 pos, int type);

#endif // SOFT_BODY_H
