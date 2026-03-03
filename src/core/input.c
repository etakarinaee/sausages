#include "input.h"

#include <string.h>

struct input_context input_context;

void input_init(struct input_context *ctx, GLFWwindow *window) {
    ctx->window = window;

    memset(ctx->key_previous, 0, sizeof(ctx->key_previous));
    memset(ctx->key_current, 0, sizeof(ctx->key_current));

    memset(ctx->mouse_previous, 0, sizeof(ctx->mouse_previous));
    memset(ctx->mouse_current, 0, sizeof(ctx->mouse_current));
}

void input_update(struct input_context *ctx) {
    memcpy(ctx->key_previous, ctx->key_current, sizeof(ctx->key_current));
    memcpy(ctx->mouse_previous, ctx->mouse_current, sizeof(ctx->mouse_current));

    for (int i = 0; i <= GLFW_KEY_LAST; i++) {
        ctx->key_current[i] = glfwGetKey(ctx->window, i);
    }

    for (int i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++) {
        ctx->mouse_current[i] = glfwGetMouseButton(ctx->window, i);
    }
}

bool input_key_down(const struct input_context *ctx, const int key) {
    if (key < 0 || key >= GLFW_KEY_LAST) {
        return false;
    }

    return ctx->key_current[key] == GLFW_PRESS;
}

bool input_key_just_down(const struct input_context *ctx, const int key) {
    if (key < 0 || key > GLFW_KEY_LAST) {
        return false;
    }

    return ctx->key_current[key] == GLFW_PRESS && ctx->key_previous[key] == GLFW_RELEASE;
}

bool input_key_just_up(const struct input_context *ctx, const int key) {
    if (key < 0 || key > GLFW_KEY_LAST) {
        return false;
    }

    return ctx->key_current[key] == GLFW_RELEASE && ctx->key_previous[key] == GLFW_PRESS;
}

bool input_mouse_down(const struct input_context *ctx, const int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
        return false;
    }

    return ctx->mouse_current[button] == GLFW_PRESS;
}

bool input_mouse_just_down(const struct input_context *ctx, const int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
        return false;
    }

    return ctx->mouse_current[button] == GLFW_PRESS && ctx->mouse_previous[button] == GLFW_RELEASE;
}

bool input_mouse_just_up(const struct input_context *ctx, const int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
        return false;
    }

    return ctx->mouse_current[button] == GLFW_RELEASE && ctx->mouse_previous[button] == GLFW_PRESS;
}

void input_mouse_position(const struct input_context *ctx, double *x, double *y) {
    glfwGetCursorPos(ctx->window, x, y);
}
