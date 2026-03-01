#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <luajit-2.1/lua.h>

#include "archive.h"
#include "audio.h"
#include "local.h"
#include "lua.h"
#include "renderer.h"

#ifdef SERVER
#define ENTRY SAUSAGES_ENTRY_SERVER
#else
#define ENTRY SAUSAGES_ENTRY_CLIENT
#endif

static lua_State *L;

static void resize_callback(GLFWwindow *window, const int width, const int height) {
    struct render_context *r = glfwGetWindowUserPointer(window);
    r->width = width;
    r->height = height;
    glViewport(0, 0, width, height);
}

int main(void) {
    FILE *test = fopen(SAUSAGES_DATA, "rb");
    if (!test) {
        fprintf(stderr, "game data not available\n");
        return EXIT_FAILURE;
    }
    fclose(test);

#ifndef SERVER
    if (!glfwInit()) {
        fprintf(stderr, "failed to init glfw\n");
        return EXIT_FAILURE;
    }

    GLFWwindow *window = glfwCreateWindow(1200, 800, "Sausages", NULL, NULL);
    if (!window) {
        fprintf(stderr, "failed to create window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    gladLoadGL();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    render_context = (struct render_context){.width = width, .height = height, .window = window};
    glViewport(0, 0, width, height);

    printf("OpenGL %s\n", (const char *) glGetString(GL_VERSION));

    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetWindowUserPointer(window, &render_context);

    // TODO: any calls to renderer in server would segfault
    renderer_init(&render_context);
    audio_init();
#endif

    L = lua_init(SAUSAGES_DATA, ENTRY);
    if (!L) {
        fprintf(stderr, "lua_init() failed\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    lua_call_init(L);

#ifdef SERVER
    double last_time = glfwGetTime();
    while (1) {
        const double current_time = glfwGetTime();
        const double delta_time = current_time - last_time;
        last_time = current_time;

        L = lua_reload(L, SAUSAGES_DATA, ENTRY);
        lua_call_update(L, delta_time);

        usleep(1000);
    }
#else
    glfwSwapInterval(0);

    double last_time = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        const double current_time = glfwGetTime();
        const double delta_time = current_time - last_time;
        last_time = current_time;

        L = lua_reload(L, SAUSAGES_DATA, ENTRY);
        lua_call_update(L, delta_time);

        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer_draw(&render_context);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // vsync is disabled because it causes some weird behavior when the window is minimized, so cap the fps
        // to 240 to avoid burning the cpu
        const double frame_end = glfwGetTime();
        const double elapsed = frame_end - current_time;
        if (elapsed < 1.0 / 240.0) {
            usleep((unsigned int) ((1.0 / 240.0 - elapsed) * 1e6));
        }
    }
#endif

    renderer_deinit(&render_context);
    lua_quit(L);
    glfwDestroyWindow(render_context.window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
