
#include "renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float rectangle_vertices[] = {
     0.5f,  0.5f, 0.0f,     1.0f, 1.0f,     0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, 0.0f,     1.0f, 0.0f,     0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.0f,     0.0f, 0.0f,     0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,     0.0f, 1.0f,     0.0f, 0.0f, 1.0f,
};

unsigned int rectangle_indices[] = {
    0, 1, 3,
    1, 2, 3
};

static char* load_shader(const char* path) {
    FILE* file;
    char* buf;
    long size;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "failed to load shader: %s\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    buf = malloc(size + 1);
    fread(buf, 1, size, file);
    buf[size] = '\0';
    fclose(file);

    return buf;
}

static void buffers_init(struct render_context* ctx) {
    glGenVertexArrays(1, &ctx->vao);
    glBindVertexArray(ctx->vao);

    glGenBuffers(1, &ctx->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), rectangle_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ctx->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectangle_indices), rectangle_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

static int program_init(struct render_context* ctx) {
    int success;
    char info_log[512];

    char* vertex_str;
    char* fragment_str;
    GLuint vertex_id, fragment_id;

    /* Vertex Shader */
    vertex_str = load_shader("tri.vert");
    vertex_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_id, 1, (const GLchar* const*)&vertex_str, NULL);
    glCompileShader(vertex_id);

    glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &success);

    if(!success) {
        glGetShaderInfoLog(vertex_id, 512, NULL, info_log);
        fprintf(stderr, "Vertex Shader Compilation Failed: %s!\n", info_log);
        free((void*)vertex_str);
        return 1;
    } 

    /* Fragment Shader */
    fragment_str = load_shader("tri.frag");
    fragment_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_id, 1, (const GLchar* const*)&fragment_str, NULL);
    glCompileShader(fragment_id);

    glGetShaderiv(fragment_id, GL_COMPILE_STATUS, &success);

    if(!success) {
        glGetShaderInfoLog(fragment_id, 512, NULL, info_log);
        fprintf(stderr, "Vertex Shader Compilation Failed: %s!\n", info_log);
        free((void*)fragment_str);
        return 1;
    }

    ctx->program = glCreateProgram();
    glAttachShader(ctx->program, vertex_id);
    glAttachShader(ctx->program, fragment_id);
    glLinkProgram(ctx->program);

    glGetProgramiv(ctx->program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ctx->program, 512, NULL, info_log);
        fprintf(stderr, "failed to link shaders: %s\n", info_log);
        free((void*)vertex_str);
        free((void*)fragment_str);
        return 1;
    }

    glUseProgram(ctx->program);

    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);

    free((void*)vertex_str);
    free((void*)fragment_str);

    return 0;
}

int renderer_init(struct render_context* ctx) {
    buffers_init(ctx);
    program_init(ctx);

    /* TODO: replace with actualy dym array */
    ctx->quads = calloc(2, sizeof(struct quad_data));
    ctx->quads_count = 0;
    ctx->quads_capacity = 2;

    return 0;
}

void renderer_deinit(struct render_context *ctx) {
    if (ctx->quads) free(ctx->quads);
}

void renderer_push_quad(struct render_context *ctx, struct vec2 pos, float scale, float rotation) {
    struct quad_data data;

    /* TODO: error checking etc.. */
    if (ctx->quads_count + 1 > ctx->quads_capacity) {
        if (ctx->quads_capacity == 0) {
            ctx->quads_capacity = 2;
        } 
        else {
            ctx->quads_capacity *= 2;
        }

        ctx->quads = realloc(ctx->quads, ctx->quads_capacity * sizeof(struct quad_data));
    }
    ctx->quads_count++;

    data.pos = pos;
    data.scale = scale;
    data.rotation = rotation;

    ctx->quads[ctx->quads_count - 1] = data;
}

void renderer_draw(struct render_context *ctx) {
    struct quad_data* data;
    struct matrix m;
    GLuint uniform_matrix_loc;
    int i;

    glUseProgram(ctx->program);
    glBindVertexArray(ctx->vao);
    uniform_matrix_loc = glGetUniformLocation(ctx->program, "u_matrix");
    
    for (i = 0; i < ctx->quads_count; i++) {
        /* TODO: also suppor scale and rotations */
        data = &ctx->quads[i];
        math_matrix_translate(&m, data->pos.x, data->pos.y, 0.0f);

        glUniformMatrix4fv(uniform_matrix_loc, 1, GL_FALSE, m.m);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
}

void math_matrix_indentity(struct matrix *m) {
    memset(m, 0, sizeof(struct matrix));
    m->m[0] = 1.0f;
    m->m[5] = 1.0f;
    m->m[10] = 1.0f;
    m->m[15] = 1.0f;
}

void math_matrix_translate(struct matrix *m, float x, float y, float z) {
    memset(m, 0, sizeof(struct matrix));
    m->m[0] = 1.0f;
    m->m[5] = 1.0f;
    m->m[10] = 1.0f;
    m->m[15] = 1.0f;

    m->m[12] = x;
    m->m[13] = y;
    m->m[14] = z;
}

void math_matrix_scale(struct matrix *m, float x, float y, float z) {
    memset(m, 0, sizeof(struct matrix));
    m->m[0] = x;
    m->m[5] = y;
    m->m[10] = z;
    m->m[15] = 1.0f;
}

