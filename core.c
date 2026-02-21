#include <stdio.h>
#include <stdbool.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdlib.h>

#include "core_archive.h"

/* where the game data is stored */
#define SAUSAGES_DATA "sausages.arc"

GLuint tri_program;
GLuint vao, vbo;

GLuint compile_shader(GLenum type, const char *s) {
    int success;
    /* infoLog */
    char buf[512];
    GLuint shader;

    shader = glCreateShader(type);

    {
        const GLsizei count = 1;
        const GLint *length = NULL;

        glShaderSource(shader, count, &s, length);
    }

    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLsizei *length = NULL;

        glGetShaderInfoLog(shader, sizeof buf, length, buf);
        fprintf(stderr, "\n%s\n", buf);
        exit(1);
    }

    return shader;
}

GLuint create_shader_program(void) {
    GLuint vertex_shader, fragment_shader;
    GLuint program;
    int success;
    unsigned long len;
    /* infoLog */
    char buf[512];
    char *tri_vertex, *tri_fragment;
    
    tri_vertex = archive_read_alloc(SAUSAGES_DATA, "tri.vert", &len);
    if (!tri_vertex) {
        exit(1);
    }

    tri_fragment = archive_read_alloc(SAUSAGES_DATA, "tri.frag", &len);
    if (!tri_fragment) {
        exit(1);
    }

    vertex_shader = compile_shader(GL_VERTEX_SHADER, tri_vertex);
    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, tri_fragment);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLsizei *length = NULL;

        glGetProgramInfoLog(program, sizeof buf, length, buf);
        fprintf(stderr, "\n%s\n", buf);
        exit(1);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    free(tri_vertex);
    free(tri_fragment);

    return program;
}

void quit(void) {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(tri_program);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(tri_program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

int main(int argc, char **argv) {
    GLenum err;
    FILE *test;

    /* check for game data before doing anything */
    test = fopen(SAUSAGES_DATA, "rb");
    if (!test) {
        fprintf(stderr, "game data not available\n");
        return 1;
    }
    fclose(test);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Sausages");

    glewExperimental = true;
    err = glewInit();
    if (err != GLEW_OK) {
        /* TODO: Cannot print value of type const GLubyte * that implies specifier %p with format specifier %s that implies type const char * */
        fprintf(stderr, "glewInit: %s\n", glewGetErrorString(err));
        return -1;
    }

    /* TODO: Cannot print value of type const GLubyte * that implies specifier %p with format specifier %s that implies type const char * */
    printf("OpenGL %s\n", glGetString(GL_VERSION));

    glClearColor(0.f, 0.f, 0.f, 0.f);
    tri_program = create_shader_program();

    const float vertices[] = {
        -.5f, -.5f, 1.f, 0.f, 0.f,
        .5f, -.5f, 0.f, 1.f, 0.f,
        0.f, .5f, 0.f, 0.f, 1.f,
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);

    /* TODO: make this more readable */

    /* position at location 0 */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /* color at location 1 */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    atexit(quit);

    glutMainLoop();

    return 0;
}

