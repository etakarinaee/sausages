#ifndef INPUT_H
#define INPUT_H
#include <stdbool.h>
#include <GLFW/glfw3.h>

#define INPUT_TEXT_BUFFER_SIZE 256

struct input_context {
    GLFWwindow *window;

    int key_previous[GLFW_KEY_LAST + 1];
    int key_current[GLFW_KEY_LAST + 1];

    int mouse_previous[GLFW_MOUSE_BUTTON_LAST + 1];
    int mouse_current[GLFW_MOUSE_BUTTON_LAST + 1];

    char text_buffer[INPUT_TEXT_BUFFER_SIZE];
    int text_buffer_len;
};

extern struct input_context input_context;

void input_init(struct input_context *ctx, GLFWwindow *window);

void input_update(struct input_context *ctx);

// `input_*_just_*` api returns `true` only on the single frame where the state transitions, whereas the standard api
// returns `true` on every frame while the key is held down
bool input_key_down(const struct input_context *ctx, int key);

bool input_key_just_down(const struct input_context *ctx, int key);

bool input_key_just_up(const struct input_context *ctx, int key);

bool input_mouse_down(const struct input_context *ctx, int button);

bool input_mouse_just_down(const struct input_context *ctx, int button);

bool input_mouse_just_up(const struct input_context *ctx, int button);

void input_mouse_position(const struct input_context *ctx, double *x, double *y);

// returns the utf 8 text typed since the last call to `input_update`
// return result is valid until the next call to `input_update`
const char *input_get_text(const struct input_context *ctx, int *len);

#endif // INPUT_H
