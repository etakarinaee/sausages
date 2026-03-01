
#include "cmath.h"
#include "renderer.h"

#include <math.h>

struct vec2 math_vec2_add(struct vec2 a, struct vec2 b) {
    return (struct vec2){a.x + b.x, a.y + b.y};
}

struct vec2 math_vec2_subtract(struct vec2 a, struct vec2 b) {
    return (struct vec2){a.x - b.x, a.y - b.y};
}

struct vec2 math_vec2_scale(struct vec2 v, float scalar) {
    return (struct vec2){v.x * scalar, v.y * scalar};
}

float math_vec2_length(struct vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float math_vec2_distance(struct vec2 a, struct vec2 b) {
    struct vec2 delta = math_vec2_subtract(a, b);
    return math_vec2_length(delta);
}

float math_vec2_dot(struct vec2 a, struct vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float math_vec2_angle_cos(struct vec2 a, struct vec2 b) {
    return math_vec2_dot(a, b) / math_vec2_length(a) * math_vec2_length(b);
}

float math_vec2_angle(struct vec2 a, struct vec2 b) {
    return acosf(math_vec2_angle_cos(a, b));
}

struct vec2i math_vec2_to_vec2i(struct vec2 v) {
    return (struct vec2i){
        (int)v.x,
        (int)v.y
    };
}

struct vec2 math_vec2i_to_vec2(struct vec2i v) {
    return (struct vec2){
        (float)v.x,
        (float)v.y,
    };
}

void math_matrix_identity(struct matrix *m) {
    *m = (struct matrix){ .m = {
        [0] = 1.0f, [5] = 1.0f, [10] = 1.0f, [15] = 1.0f
    }};
}

void math_matrix_translate(struct matrix *m, const float x, const float y, const float z) {
    math_matrix_identity(m);
    m->m[12] = x;
    m->m[13] = y;
    m->m[14] = z;
}

void math_matrix_scale(struct matrix *m, const float x, const float y, const float z) {
    *m = (struct matrix){ .m = {
        [0] = x, [5] = y, [10] = z, [15] = 1.0f
    }};
}

void math_matrix_rotate_2d(struct matrix *m, float angle) {
    float theta = DEG2RAD(angle);

    *m = (struct matrix){ .m = {
        [0] = cos(theta), [1] = -sin(theta), 
        [4] = sin(theta), [5] = cos(theta),
        [10] = 1.0f, [15] = 1.0f,
    }};
}

/* TODO: probaly optimize or smth */
void math_matrix_mul(struct matrix *out, struct matrix *a, struct matrix *b) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a->m[k * 4 + r] * b->m[c * 4 + k];
            }
            out->m[c * 4 + r] = sum;
        }
    }
}

void math_matrix_orthographic(struct matrix *m, float left, float right, float bottom, float top, float near, float far) {
    *m = (struct matrix){ .m = {
        [0] = 2.0f / (right - left), [5] = 2.0f / (top - bottom),
        [10] = 2.0f / (near - far), [15] = 1.0f,
        [12] = (left + right) / (left - right),
        [13] = (bottom + top) / (bottom - top),
        [14] = (near + far) / (near - far),
    }};
}

void math_matrix_get_orthographic(struct render_context* r, struct matrix *m) {
    float half_w = (r->width / r->camera.zoom) * 0.5f;
    float half_h = (r->height / r->camera.zoom) * 0.5f;

    math_matrix_orthographic(m, -half_w, half_w, -half_h, half_h, -1.0f, 1.0f);
}

