
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
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            double x;
            double y;
            glfwGetCursorPos(window, &x, &y);
            renderer_push_quad(&ctx, (struct vec2){x, y}, 1.0f, 0.0f);
        }

        renderer_draw(&ctx);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}


