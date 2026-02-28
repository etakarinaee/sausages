#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <luajit-2.1/lua.h>

#include "archive.h"
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

    if (!glfwInit()) {
        fprintf(stderr, "failed to init glfw\n");
        return EXIT_FAILURE;
    }

    GLFWwindow* window = glfwCreateWindow(1200, 800, "Sausages", NULL, NULL);
    if (!window) {
        fprintf(stderr, "failed to create window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    gladLoadGL();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    ctx = (struct render_context){.width = width, .height = height, .window = window};
    glViewport(0, 0, width, height);

    printf("OpenGL %s\n", (const char *) glGetString(GL_VERSION));

    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetWindowUserPointer(window, &ctx);

    L = lua_init(SAUSAGES_DATA, ENTRY);
    if (!L) {
        fprintf(stderr, "lua_init() failed\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    lua_call_init(L);

    renderer_init(&ctx);

    font_id id = renderer_load_font(&ctx, "../AdwaitaSans-Regular.ttf");

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
    double last_time = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        const double current_time = glfwGetTime();
        const double delta_time = current_time - last_time;
        last_time = current_time;

        L = lua_reload(L, SAUSAGES_DATA, ENTRY);
        lua_call_update(L, delta_time);

        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer_push_text(&ctx, (struct vec2){0, 0}, 2.0f, (struct color3){1.0f, 1.0f, 1.0f}, id, "Hello World");
        renderer_draw(&ctx);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
#endif

    renderer_deinit(&ctx);
    lua_quit(L);
    glfwTerminate();

    return EXIT_SUCCESS;
}
