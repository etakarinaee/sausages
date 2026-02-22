#include <stdio.h>
#include <stdlib.h>

#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>

#include "renderer.h"
#include <GLFW/glfw3.h> /* after renderer because renderer includes glad which muss be included after glfw */

static struct vec2 check_vec2(lua_State *L, const int idx) {
    struct vec2 v;
    luaL_checktype(L, idx, LUA_TTABLE);

    lua_rawgeti(L, idx, 1);
    v.x = (float) luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, idx, 2);
    v.y = (float) luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return v;
}

static struct color3 check_color3(lua_State *L, const int idx) {
    struct color3 c;
    luaL_checktype(L, idx, LUA_TTABLE);

    lua_rawgeti(L, idx, 1);
    c.r = (float) luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, idx, 2);
    c.g = (float) luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, idx, 3);
    c.b = (float) luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return c;
}

static int quit(lua_State *L) {
    (void) L;
    exit(0);
}

static int print(lua_State *L) {
    printf("%s\n", luaL_checkstring(L, 1));

    return 0;
}

static int get_screen_dimensions(lua_State *L) {
    int width, height;
    glfwGetWindowSize(ctx.window, &width, &height);
    
    lua_newtable(L);
    lua_pushinteger(L, width);
    lua_setfield(L, -2, "width");

    lua_pushinteger(L, height);
    lua_setfield(L, -2, "height");

    return 1;
}

static int push_quad(lua_State *L) {
    const struct vec2 pos = check_vec2(L, 1);
    const struct color3 color = check_color3(L, 2);
    const texture_id tex = luaL_checkint(L, 3);

    renderer_push_quad(&ctx, pos, 1.0f, 0.0f, color, tex);

    return 0;
}

static int load_texture(lua_State *L) {
    const texture_id tex = renderer_load_texture(luaL_checkstring(L, 1));
    lua_pushinteger(L, tex);
    return 1;
}

static int key_pressed(lua_State *L) {
    if (glfwGetKey(ctx.window, luaL_checkint(L, 1)) == GLFW_PRESS) {
        lua_pushinteger(L, 1);
        return 1;
    }
    return 0;
}

static int key_down(lua_State *L) {
    if (glfwGetKey(ctx.window, luaL_checkint(L, 1)) != GLFW_RELEASE) {
        lua_pushinteger(L, 1);
        return 1;
    }
    return 0;
}

static int mouse_pressed(lua_State *L) {
    if (glfwGetMouseButton(ctx.window, luaL_checkint(L, 1)) == GLFW_PRESS) {
        lua_pushinteger(L, 1);
        return 1;
    }
    return 0;
}

static int mouse_down(lua_State *L) {
    if (glfwGetMouseButton(ctx.window, luaL_checkint(L, 1)) != GLFW_RELEASE) {
        lua_pushinteger(L, 1);
        return 1;
    }
    return 0;
}

static int mouse_pos(lua_State *L) {
    double x;
    double y;
    glfwGetCursorPos(ctx.window, &x, &y);

    lua_newtable(L);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");

    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");

    return 1;
}

static const luaL_Reg api[] = {
    {"quit", quit},
    {"print", print},
    {"get_screen_dimensions", get_screen_dimensions},

    /* Render */
    {"push_quad", push_quad},
    {"load_texture", load_texture},

    /* Input */
    {"key_pressed", key_pressed},
    {"key_down", key_down},
    {"mouse_pressed", mouse_pressed},
    {"mouse_down", mouse_down},
    {"mouse_pos", mouse_pos},
    {NULL, NULL},
};

static void keys_init(lua_State *L) {
    int i;
    char k[2];
    k[1] = '\0';

    lua_newtable(L);

    for (i = 'A'; i <= 'Z'; i++) {
        lua_pushinteger(L, i);
        k[0] = (char) (i + 'a' - 'A');
        lua_setfield(L, -2, k);
    }

    for (i = 'A'; i <= 'Z'; i++) {
        lua_pushinteger(L, i);
        k[0] = (char) i;
        lua_setfield(L, -2, k);
    }

    for (i = '0'; i <= '9'; i++) {
        lua_pushinteger(L, i);
        k[0] = (char) i;
        lua_setfield(L, -2, k);
    }

    lua_pushinteger(L, ' ');
    lua_setfield(L, -2, "space");

    lua_pushinteger(L, '\r');
    lua_setfield(L, -2, "return");

    lua_pushinteger(L, '\n');
    lua_setfield(L, -2, "enter");

    lua_pushinteger(L, '\t');
    lua_setfield(L, -2, "tab");

    lua_pushinteger(L, 27);
    lua_setfield(L, -2, "escape");

    lua_pushinteger(L, 127);
    lua_setfield(L, -2, "backspace");

    lua_setglobal(L, "key");
}

static const char *mouse_names[] = {
    "left",
    "right",
    "middle",
    "button_4",
    "button_5",
    "button_6",
    "button_7",
    "button_8",
};

static void mouse_init(lua_State *L) {
    lua_newtable(L);

    for (int i = 0; i < 8; i++) {
        lua_pushinteger(L, i);
        lua_setfield(L, -2, mouse_names[i]);
    }

    lua_setglobal(L, "mouse");
}

void lua_api_init(lua_State *L) {
    lua_newtable(L);
    for (const luaL_Reg *f = api; f->name; f++) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }

    lua_setglobal(L, "core");

    keys_init(L);
    mouse_init(L);
}
