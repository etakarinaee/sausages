#include "input.h"

#include <string.h>

struct input_context input_context;

static void char_callback(GLFWwindow *window, unsigned int codepoint) {
    (void) window;
    struct input_context *ctx = &input_context;

    if (codepoint < 0x80 && ctx->text_buffer_len + 1 < INPUT_TEXT_BUFFER_SIZE) {
        ctx->text_buffer[ctx->text_buffer_len++] = (char) codepoint;
    } else if (codepoint < 0x800 && ctx->text_buffer_len + 2 < INPUT_TEXT_BUFFER_SIZE) {
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0xC0 | (codepoint >> 6));
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000 && ctx->text_buffer_len + 3 < INPUT_TEXT_BUFFER_SIZE) {
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0xE0 | (codepoint >> 12));
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0x80 | ((codepoint >> 6) & 0x3F));
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0x80 | (codepoint & 0x3F));
    } else if (ctx->text_buffer_len + 4 < INPUT_TEXT_BUFFER_SIZE) {
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0xF0 | (codepoint >> 18));
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0x80 | ((codepoint >> 12) & 0x3F));
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0x80 | ((codepoint >> 6) & 0x3F));
        ctx->text_buffer[ctx->text_buffer_len++] = (char) (0x80 | (codepoint & 0x3F));
    }
    ctx->text_buffer[ctx->text_buffer_len] = '\0';
}

void input_init(struct input_context *ctx, GLFWwindow *window) {
    ctx->window = window;

    memset(ctx->key_previous, 0, sizeof(ctx->key_previous));
    memset(ctx->key_current, 0, sizeof(ctx->key_current));

    memset(ctx->mouse_previous, 0, sizeof(ctx->mouse_previous));
    memset(ctx->mouse_current, 0, sizeof(ctx->mouse_current));

    ctx->text_buffer_len = 0;
    ctx->text_buffer[0] = '\0';

    glfwSetCharCallback(window, char_callback);
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

const char *input_get_text(const struct input_context *ctx, int *len) {
    if (len) {
        *len = ctx->text_buffer_len;
    }

    return ctx->text_buffer;
}

void input_clear_text(struct input_context *ctx) {
    ctx->text_buffer_len = 0;
    ctx->text_buffer[0] = '\0';
}
