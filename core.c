
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>

#include "renderer.h"

void resize_callback(GLFWwindow* window, int width, int height) {
    struct render_context* ctx = (struct render_context*)glfwGetWindowUserPointer(window);
    ctx->width = width;
    ctx->height = height;
    glViewport(0, 0, width, height);
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "failed to init glfw\n");
        return EXIT_FAILURE;
    }

    GLFWwindow* window = glfwCreateWindow(1200, 800, "Sausages", NULL, NULL);
    if (!window) {
        fprintf(stderr, "failed to create window\n");
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    gladLoadGL();

    struct render_context ctx = {0};
    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetWindowUserPointer(window, &ctx);

    renderer_init(&ctx);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double x; /* Not sure about the c89 std in this next lines here */
            double y;
            glfwGetCursorPos(window, &x, &y);
            struct vec2 pos;

            /* convert mouse pos from window coords to 0 to 1 by dividing with width 
                than convert to -1 to 1 by multiply 2 and then subtract -1
            */
            pos = (struct vec2){ (x / ctx.width) * 2.0f - 1.0f, -((y / ctx.height) * 2.0f - 1.0f) };

            /* printf("X: %f Y: %f\n", pos.x, pos.y); */

            renderer_push_quad(&ctx, pos, 1.0f, 0.0f);
        }

        renderer_draw(&ctx);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    renderer_deinit(&ctx);
    glfwTerminate();

    return 0;
}


