#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include <luajit-2.1/lua.h>

#include "archive.h"
#include "lua.h"
#include "renderer.h"

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

    ctx = (struct render_context){.width = width, .height = height, .window = window};
    glViewport(0, 0, width, height);

    printf("OpenGL %s\n", (const char *) glGetString(GL_VERSION));

    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetWindowUserPointer(window, &ctx);

    L = lua_init(SAUSAGES_DATA, SAUSAGES_ENTRY);
    if (!L) {
        fprintf(stderr, "lua_init() failed\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    lua_call_init(L);

    renderer_init(&ctx);

    const texture_id tex = renderer_load_texture("../test.png");
    double last_time = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        const double current_time = glfwGetTime();
        const double delta_time = current_time - last_time;
        last_time = current_time;

        L = lua_reload(L, SAUSAGES_DATA, SAUSAGES_ENTRY);
        lua_call_update(L, delta_time);

        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);

            const struct vec2 pos = {
                .x = (float) (x / ctx.width * 2.0 - 1.0),
                .y = (float) -(y / ctx.height * 2.0 - 1.0)
            };
            const struct color3 color = {.r = 1.0f, .g = 1.0f, .b = 1.0f};

            renderer_push_quad(&ctx, pos, 1.0f, 0.0f, color, tex);
        }

        renderer_draw(&ctx);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    renderer_deinit(&ctx);
    lua_quit(L);
    glfwTerminate();

    return EXIT_SUCCESS;
}
