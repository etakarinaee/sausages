
#ifndef SOFTBODY_H
#define SOFTBODY_H

#include "cmath.h"
#include "renderer.h"

enum {
    SOFTBODY_NORMAL,
    SOFTBODY_EDGE_STATIC, /* only endpoints (left and right) static */
    SOFTBODY_STATIC,      /* everythign static */
};

struct softbody_point {
    struct vec2 pos;
    struct vec2 vel;
    struct vec2 force;
    struct vec2 rest_pos; /* offset from center at creation */
    float mass;
    bool is_static; /* not moveable */
};

struct spring {
    /* handles to softbody_point */
    int start;
    int end;
    float rest_len;
    bool end_frame; /* true if end handle is a frame point */
};

struct edge {
    int start;
    int end;
};

struct softbody {
    float stiffness;
    float damping;
    float point_radius;

    int points_count;
    int springs_count;
    int edges_count;
    struct softbody_point *points;
    struct spring *springs;
    struct edge *edges;
    struct softbody_point *frame_points;

    float frame_angle;
    /* never use these directly they are only for caching the position */
    struct vec2 frame_pos;
    bool update_pos;

    struct vec2 force; /* acumalitive outside force acting on the hole object */
    struct vec2 size;
    struct mesh mesh;
};

struct softbody softbody_create_rect(struct vec2 pos, struct vec2 size,
                                     struct color3 color, int type);
void softbody_update(struct softbody *b, float dt, struct color3 color);
void softbody_check_coll(struct softbody *a, struct softbody *b);
void softbody_apply_velocity(struct softbody *b, struct vec2 vel);
void softbody_apply_force(struct softbody *b, struct vec2 force);
void softbody_draw(struct softbody *b);
void softbody_destroy(struct softbody *b);

struct vec2 softbody_get_pos(struct softbody *b);
void softbody_set_pos(struct softbody *b, struct vec2 pos);

#endif // softbody_H
