#include "renderer.h"
#include "archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stbi/stb_image.h>

struct render_context ctx;
GLFWwindow* window;

float rectangle_vertices[] = {
    /* POS              COLOR */
    0.5f, 0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.0f, 1.0f,
};

unsigned int rectangle_indices[] = {
    0, 1, 3,
    1, 2, 3
};

static void buffers_init(struct render_context *ctx) {
    glGenVertexArrays(1, &ctx->vao);
    glBindVertexArray(ctx->vao);

    glGenBuffers(1, &ctx->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), rectangle_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ctx->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectangle_indices), rectangle_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

static int program_init(struct render_context *ctx) {
    int error = 0;
    int success;
    char info_log[512];
    unsigned long len;

    char *vertex_str = NULL;
    char *fragment_str = NULL;
    GLuint vertex_id = 0, fragment_id = 0;

    /* Vertex Shader */
    vertex_str = archive_read_alloc(SAUSAGES_DATA, "tri.vert", &len);
    vertex_id = glCreateShader(GL_VERTEX_SHADER);

    if (!vertex_str) {
        error = 1;
        goto end;
    }

    glShaderSource(vertex_id, 1, (const GLchar * const*) &vertex_str, NULL);
    glCompileShader(vertex_id);

    glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vertex_id, 512, NULL, info_log);
        fprintf(stderr, "Vertex Shader Compilation Failed: %s!\n", info_log);
        error = 1;
        goto end;
    }

    /* Fragment Shader */
    fragment_str = archive_read_alloc(SAUSAGES_DATA, "tri.frag", &len);
    fragment_id = glCreateShader(GL_FRAGMENT_SHADER);

    if (!fragment_str) {
        error = 1;
        goto end;
    }

    glShaderSource(fragment_id, 1, (const GLchar * const*) &fragment_str, NULL);
    glCompileShader(fragment_id);

    glGetShaderiv(fragment_id, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragment_id, 512, NULL, info_log);
        fprintf(stderr, "Fragment Shader Compilation Failed: %s!\n", info_log);
        error = 1;
        goto end;
    }

    ctx->program = glCreateProgram();
    glAttachShader(ctx->program, vertex_id);
    glAttachShader(ctx->program, fragment_id);
    glLinkProgram(ctx->program);

    glGetProgramiv(ctx->program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ctx->program, 512, NULL, info_log);
        fprintf(stderr, "failed to link shaders: %s\n", info_log);
        error = 1;
        goto end;
    }

    glUseProgram(ctx->program);

end:
    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);

    free(vertex_str);
    free(fragment_str);

    return error;
}

int renderer_init(struct render_context *ctx) {
    buffers_init(ctx);
    if (program_init(ctx)) {
        fprintf(stderr, "failed to init shaders!\n");
        return 1;
    }

    /* TODO: replace with actualy dym array */
    ctx->quads = calloc(2, sizeof(struct quad_data));
    ctx->quads_count = 0;
    ctx->quads_capacity = 2;

    return 0;
}

void renderer_deinit(struct render_context *ctx) {
    if (ctx->quads) free(ctx->quads);
}

void renderer_push_quad(struct render_context *ctx, struct vec2 pos, float scale, float rotation, struct color3 c) {
    struct quad_data data;
    struct quad_data *new_data;

    /* TODO: error checking etc.. */
    if (ctx->quads_count + 1 > ctx->quads_capacity) {
        if (ctx->quads_capacity == 0) {
            ctx->quads_capacity = 2;
        } else {
            ctx->quads_capacity *= 2;
        }

        new_data = realloc(ctx->quads, ctx->quads_capacity * sizeof(struct quad_data));
        if (!new_data) {
            fprintf(stderr, "out of memory!\n");
            return;
        }

        ctx->quads = new_data;
    }
    ctx->quads_count++;

    data.pos = pos;
    data.scale = scale;
    data.rotation = rotation;
    data.color = c;

    ctx->quads[ctx->quads_count - 1] = data;
}

void renderer_draw(struct render_context *ctx) {
    struct quad_data *data;
    struct matrix m;
    GLint uniform_matrix_loc;
    GLint uniform_color_loc;
    int i;

    glUseProgram(ctx->program);
    glBindVertexArray(ctx->vao);
    uniform_matrix_loc = glGetUniformLocation(ctx->program, "u_matrix");
    uniform_color_loc = glGetUniformLocation(ctx->program, "u_color");

    for (i = 0; i < ctx->quads_count; i++) {
        /* TODO: also suppor scale and rotations */
        data = &ctx->quads[i];
        math_matrix_translate(&m, data->pos.x, data->pos.y, 0.0f);

        glUniformMatrix4fv(uniform_matrix_loc, 1, GL_FALSE, m.m);
        glUniform3f(uniform_color_loc, data->color.r, data->color.g, data->color.b);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
}

texture_id renderer_load_texture(struct render_context *ctx, const char *path) {
    int width, height, channels;
    unsigned char* data;
    GLuint texture;
    GLint format;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(1);
    data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        fprintf(stderr, "failed to load texture: %s\n", path);
        return -1;
    }

    format = GL_RGB;

    if (channels == 4) {
        format = GL_RGBA;
    }
    else if (channels == 1) {
        format = GL_RED;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    return (texture_id)texture;
}

void math_matrix_identity(struct matrix *m) {
    memset(m, 0, sizeof(struct matrix));
    m->m[0] = 1.0f;
    m->m[5] = 1.0f;
    m->m[10] = 1.0f;
    m->m[15] = 1.0f;
}

void math_matrix_translate(struct matrix *m, float x, float y, float z) {
    math_matrix_identity(m);

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
