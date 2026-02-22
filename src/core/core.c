#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include <luajit-2.1/lua.h>

#include "archive.h"
#include "lua.h"

#include "renderer.h"

static lua_State *L;

void resize_callback(GLFWwindow *window, int width, int height) {
    struct render_context *ctx = glfwGetWindowUserPointer(window);
    ctx->width = width;
    ctx->height = height;
    glViewport(0, 0, width, height);
}

int main(void) {
    FILE *test;
    int width, height;
    double current_time, last_time;
    double delta_time;
    texture_id tex;

    /* check for game data before doing anything */
    test = fopen(SAUSAGES_DATA, "rb");
    if (!test) {
        fprintf(stderr, "game data not available\n");
        return 1;
    }
    fclose(test);

    if (!glfwInit()) {
        fprintf(stderr, "failed to init glfw\n");
        return EXIT_FAILURE;
    }

    window = glfwCreateWindow(1200, 800, "Sausages", NULL, NULL);
    if (!window) {
        fprintf(stderr, "failed to create window\n");
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    gladLoadGL();

    glfwGetFramebufferSize(window, &width, &height);

    ctx.width = width;
    ctx.height = height;

    glViewport(0, 0, width, height);

    printf("OpenGL %s\n", glGetString(GL_VERSION));

    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetWindowUserPointer(window, &ctx);

    /* Init Lua */
    L = lua_init(SAUSAGES_DATA, SAUSAGES_ENTRY);
    if (!L) {
        fprintf(stderr, "lua_init()\n");

        return 1;
    }
    lua_call_init(L);

    renderer_init(&ctx);

    tex = renderer_load_texture("../test.png");

    while (!glfwWindowShouldClose(window)) {
        current_time = glfwGetTime();

        delta_time = current_time - last_time;
        last_time = current_time;

        L = lua_reload(L, SAUSAGES_DATA, SAUSAGES_ENTRY);
        lua_call_update(L, delta_time);

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            struct vec2 pos;
            struct color3 color;
            double x; /* Not sure about the c89 std in this next lines here */
            double y;
            glfwGetCursorPos(window, &x, &y);

            /* convert mouse pos from window coords to 0 to 1 by dividing with width 
                than convert to -1 to 1 by multiply 2 and then subtract -1 */
            pos.x = (x / ctx.width) * 2.0f - 1.0f;
            pos.y = -((y / ctx.height) * 2.0f - 1.0f);

            color.r = 1.0f;
            color.g = 1.0f;
            color.b = 1.0f;

            renderer_push_quad(&ctx, pos, 1.0f, 0.0f, color, tex);
        }

        renderer_draw(&ctx);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    renderer_deinit(&ctx);
    glfwTerminate();

    lua_quit(L);

    return 0;
}
