#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stddef.h>
#include <math.h>

#define CORE_RENDERER_QUAD_NO_TEXTURE (-1)

struct vec2 {
    float x;
    float y;
};

struct color3 {
    float r;
    float g;
    float b;
};

struct matrix {
    float m[16];
};

typedef GLint texture_id;

struct quad_data {
    texture_id tex;
    float scale;
    float rotation;
    struct vec2 pos;
    struct color3 color;
};

struct render_context {
    int width;
    int height;
    GLFWwindow *window;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    GLuint quad_program;
    GLuint tex_program;

    struct quad_data *quads;
    size_t quads_count;
    size_t quads_capacity;
};

extern struct render_context ctx;

int renderer_init(struct render_context *r);
void renderer_deinit(const struct render_context *r);
void renderer_push_quad(struct render_context *r, struct vec2 pos, float scale, float rotation, struct color3 color, texture_id tex);
void renderer_draw(struct render_context *r);

texture_id renderer_load_texture(const char *path);

/* Math */
#define RAD2DEG(x) (x * 180.0f / M_PI)
#define DEG2RAD(x) (x * M_PI / 180.0f)

void math_matrix_identity(struct matrix *m);
void math_matrix_translate(struct matrix *m, float x, float y, float z);
void math_matrix_scale(struct matrix *m, float x, float y, float z);

/* Angle in degrees */
void math_matrix_rotate_2d(struct matrix* m, float angle);
void math_matrix_mul(struct matrix* out, struct matrix* a, struct matrix* b);

#endif
