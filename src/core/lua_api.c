#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>

#include "renderer.h"
#include <GLFW/glfw3.h> /* after renderer because renderer includes glad which muss be included after glfw */

#include "archive.h"
#include "cmath.h"
#include "collision.h"
#include "game.h"
#include "input.h"
#include "local.h"
#include "net.h"
#include "soft_body.h"
#include "ui.h"
#include "voice.h"

// metatables
#define SERVER_MT "net_server"
#define CLIENT_MT "net_client"

////////////////
/* etc */
////////////////

static int l_read_stdin(lua_State *L) {
    const int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    char buf[1024];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);

    fcntl(STDIN_FILENO, F_SETFL, flags);

    if (n <= 0) {
        return 0;
    }

    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
        n--;
    }
    if (n == 0) {
        return 0;
    }

    lua_pushlstring(L, buf, (size_t)n);
    return 1;
}

static struct vec2 check_vec2(lua_State *L, const int idx) {
    struct vec2 v;
    luaL_checktype(L, idx, LUA_TTABLE);

    lua_rawgeti(L, idx, 1);
    v.x = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, idx, 2);
    v.y = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return v;
}

static struct vec2i check_vec2i(lua_State *L, const int idx) {
    struct vec2i v;
    luaL_checktype(L, idx, LUA_TTABLE);

    lua_rawgeti(L, idx, 1);
    v.x = (float)luaL_checkint(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, idx, 2);
    v.y = (float)luaL_checkint(L, -1);
    lua_pop(L, 1);

    return v;
}

static struct color3 check_color3(lua_State *L, const int idx) {
    struct color3 c;
    luaL_checktype(L, idx, LUA_TTABLE);

    lua_rawgeti(L, idx, 1);
    c.r = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, idx, 2);
    c.g = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, idx, 3);
    c.b = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return c;
}

static int l_quit(lua_State *L) {
    (void)L;
    exit(0);
}

static int l_print(lua_State *L) {
    printf("%s\n", luaL_checkstring(L, 1));

    return 0;
}

////////////////
/* rendering */
////////////////

static int l_get_screen_dimensions(lua_State *L) {
    int width, height;
    glfwGetWindowSize(render_context.window, &width, &height);

    lua_newtable(L);
    lua_pushinteger(L, width);
    lua_setfield(L, -2, "width");

    lua_pushinteger(L, height);
    lua_setfield(L, -2, "height");

    return 1;
}

static int l_push_rect(lua_State *L) {
    const struct vec2 pos = check_vec2(L, 1);
    const struct vec2 scale = check_vec2(L, 2);
    const struct color3 color = check_color3(L, 3);

    renderer_push_rect(&render_context, pos, scale, 0.0f, color, ANCHOR_CENTER);

    return 0;
}

static int l_push_rect_ex(lua_State *L) {
    const struct vec2 pos = check_vec2(L, 1);
    const struct vec2 scale = check_vec2(L, 2);
    const float rotation = luaL_checknumber(L, 3);
    const struct color3 color = check_color3(L, 4);
    const int anchor = luaL_checkint(L, 5);

    renderer_push_rect(&render_context, pos, scale, rotation, color, anchor);

    return 0;
}

static int l_push_circle(lua_State *L) {
    const struct vec2 pos = check_vec2(L, 1);
    const float radius = luaL_checknumber(L, 2);
    const struct color3 color = check_color3(L, 3);

    renderer_push_circle(&render_context, pos, radius, color);

    return 0;
}

static int l_push_texture(lua_State *L) {
    const struct vec2 pos = check_vec2(L, 1);
    const struct vec2 scale = check_vec2(L, 2);
    const texture_id tex = luaL_checkint(L, 3);

    renderer_push_texture(&render_context, pos, scale, 0.0f, tex,
                          ANCHOR_CENTER);

    return 0;
}

static int l_push_texture_ex(lua_State *L) {
    const struct vec2 pos = check_vec2(L, 1);
    const struct vec2 scale = check_vec2(L, 2);
    const float rotation = luaL_checknumber(L, 3);
    const texture_id tex = luaL_checkint(L, 4);
    const int anchor = luaL_checkint(L, 5);

    renderer_push_texture(&render_context, pos, scale, rotation, tex, anchor);

    return 0;
}

static int l_push_text(lua_State *L) {
    const int font = luaL_checkint(L, 1);
    const char *text = luaL_checkstring(L, 2);
    const struct vec2 pos = check_vec2(L, 3);
    float scale = luaL_checknumber(L, 4);
    const struct color3 color = check_color3(L, 5);

    renderer_push_text(&render_context, pos, scale, color, font, text,
                       ANCHOR_BOTTOM_LEFT);

    return 0;
}

static int l_push_text_ex(lua_State *L) {
    const int font = luaL_checkint(L, 1);
    const char *text = luaL_checkstring(L, 2);
    const struct vec2 pos = check_vec2(L, 3);
    float scale = luaL_checknumber(L, 4);
    const struct color3 color = check_color3(L, 5);
    const int anchor = luaL_checkint(L, 6);

    renderer_push_text(&render_context, pos, scale, color, font, text, anchor);

    return 0;
}

static int l_push_mesh(lua_State *L) {
    const int handle = luaL_checkinteger(L, 1);
    const struct vec2 pos = check_vec2(L, 2);
    const struct vec2 scale = check_vec2(L, 3);

    renderer_push_mesh(&render_context, game_context.meshs[handle], pos, scale);
    return 0;
}

static int l_create_mesh(lua_State *L) {
    const int vertex_count = luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    float vertices[vertex_count];

    for (int i = 0; i < vertex_count; i++) {
        lua_rawgeti(L, 2, 1);
        vertices[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    const struct color3 color = check_color3(L, 3);
    /*
         TODO: implement ebo generation
        int handle = game_context.meshs_index;
        game_context.meshs[game_context.meshs_index++] =
            renderer_create_mesh(vertices, vertex_count, color);
        lua_pushinteger(L, handle);
        */
    return 1;
}

static int l_update_mesh(lua_State *L) {
    const int handle = luaL_checkinteger(L, 1);
    const int vertex_count = luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);

    float vertices[vertex_count];

    for (int i = 0; i < vertex_count; i++) {
        lua_rawgeti(L, 3, 1);
        vertices[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    const struct color3 color = check_color3(L, 4);

    renderer_update_mesh(&game_context.meshs[handle], vertices, vertex_count,
                         color);

    return 0;
}

static int l_load_texture(lua_State *L) {
    const texture_id tex = renderer_load_texture(luaL_checkstring(L, 1));
    lua_pushinteger(L, tex);
    return 1;
}

static int l_load_font(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    const int font_size = luaL_checkint(L, 2);
    const struct vec2i range = check_vec2i(L, 3);

    const font_id font =
        renderer_load_font(&render_context, text, font_size, range);
    lua_pushinteger(L, font);
    return 1;
}

// softbody
static int l_create_softbody(lua_State *L) {
    const struct vec2 pos = check_vec2(L, 1);
    const struct vec2 size = check_vec2(L, 2);

    int handle = game_context.soft_bodies_index;
    game_context.soft_bodies[game_context.soft_bodies_index++] =
        ph_soft_body_create_rect(pos, size);
    lua_pushinteger(L, handle);
    return 1;
}

static int l_update_softbody(lua_State *L) {
    const int handle = luaL_checkinteger(L, 1);
    const float dt = luaL_checknumber(L, 2);

    ph_soft_body_update(&game_context.soft_bodies[handle], dt);

    return 0;
}

static int l_softbody_apply_velocity(lua_State *L) {
    const int handle = luaL_checkinteger(L, 1);
    const struct vec2 vel = check_vec2(L, 2);

    ph_soft_body_apply_velocity(&game_context.soft_bodies[handle], vel);

    return 0;
}

static int l_draw_softbody(lua_State *L) {
    const int handle = luaL_checknumber(L, 1);

    ph_soft_body_draw(&game_context.soft_bodies[handle]);

    return 0;
}

static int l_destroy_softbody(lua_State *L) {
    const int handle = luaL_checknumber(L, 1);

    ph_soft_body_destroy(&game_context.soft_bodies[handle]);

    return 0;
}

////////////////
/* ui */
////////////////

static int l_button(lua_State *L) {
    const int font = luaL_checkint(L, 1);
    const char *text = luaL_checkstring(L, 2);
    const struct vec2 pos = check_vec2(L, 3);
    const struct vec2i size = check_vec2i(L, 4);

    bool clicked = ui_button(&render_context, font, text, pos, size);
    if (clicked) {
        lua_pushinteger(L, clicked);
        return 1;
    }

    return 0;
}

static int l_screen_to_world(lua_State *L) {
    const struct vec2 screen_pos = check_vec2(L, 1);

    int width, height;
    glfwGetWindowSize(render_context.window, &width, &height);

    lua_newtable(L);
    lua_pushnumber(L, screen_pos.x - width / 2.0f);
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, height / 2.0f - screen_pos.y);
    lua_rawseti(L, -2, 2);

    return 1;
}

////////////////
/* collision */
////////////////

static int l_check_point_circle(lua_State *L) {
    const struct vec2 p = check_vec2(L, 1);
    const struct vec2 center = check_vec2(L, 2);
    const float radius = (float)luaL_checknumber(L, 3);

    bool in = coll_check_point_circle(p, center, radius);
    if (in) {
        lua_pushinteger(L, in);
        return 1;
    }

    return 0;
}

static int l_check_point_rect(lua_State *L) {
    const struct vec2 p = check_vec2(L, 1);
    const struct vec2 pos = check_vec2(L, 2);
    const struct vec2 size = check_vec2(L, 3);

    bool in = coll_check_point_rect(p, pos, size);
    if (in) {
        lua_pushinteger(L, in);
        return 1;
    }

    return 0;
}

////////////////
/* input */
////////////////

static int l_key_down(lua_State *L) {
    if (input_key_down(&input_context, luaL_checkint(L, 1))) {
        lua_pushinteger(L, 1);

        return 1;
    }

    return 0;
}

static int l_key_just_down(lua_State *L) {
    if (input_key_just_down(&input_context, luaL_checkint(L, 1))) {
        lua_pushinteger(L, 1);

        return 1;
    }

    return 0;
}

static int l_key_just_up(lua_State *L) {
    if (input_key_just_up(&input_context, luaL_checkint(L, 1))) {
        lua_pushinteger(L, 1);

        return 1;
    }

    return 0;
}

static int l_mouse_down(lua_State *L) {
    if (input_mouse_down(&input_context, luaL_checkint(L, 1))) {
        lua_pushinteger(L, 1);

        return 1;
    }

    return 0;
}

static int l_mouse_just_down(lua_State *L) {
    if (input_mouse_just_down(&input_context, luaL_checkint(L, 1))) {
        lua_pushinteger(L, 1);

        return 1;
    }

    return 0;
}

static int l_mouse_just_up(lua_State *L) {
    if (input_mouse_just_up(&input_context, luaL_checkint(L, 1))) {
        lua_pushinteger(L, 1);
        return 1;
    }

    return 0;
}

static int l_mouse_position(lua_State *L) {
    double x;
    double y;
    input_mouse_position(&input_context, &x, &y);

    lua_newtable(L);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");

    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");

    return 1;
}

static int l_get_text_input(lua_State *L) {
    int len;
    const char *text = input_get_text(&input_context, &len);

    if (len > 0) {
        lua_pushlstring(L, text, len);

        return 1;
    }

    return 0;
}

////////////////
/* networking */
////////////////

static int push_event(lua_State *L, const struct net_event *event) {
    if (event->type == NET_EVENT_NONE) {
        lua_pushnil(L);

        return 1;
    }

    lua_newtable(L);
    lua_pushinteger(L, event->type);
    lua_setfield(L, -2, "type");

    lua_pushinteger(L, event->client_id);
    lua_setfield(L, -2, "id");

    if (event->type == NET_EVENT_DATA && event->len > 0) {
        lua_pushlstring(L, (const char *)event->data, event->len);
        lua_setfield(L, -2, "data");
    }

    return 1;
}

// server

static int l_server_new(lua_State *L) {
    const char *ip = luaL_checkstring(L, 1);
    const uint16_t port = (uint16_t)luaL_checkint(L, 2);
    const uint32_t n = (uint32_t)luaL_optint(L, 3, 32);

    struct net_server *server = net_server_create(ip, port, n);
    if (!server) {
        luaL_error(L, "core.server.new: failed on port %d", port);
    }

    struct net_server **sp = lua_newuserdata(L, sizeof(*sp));
    *sp = server;

    luaL_getmetatable(L, SERVER_MT);
    lua_setmetatable(L, -2);

    return 1;
}

static int l_server_poll(lua_State *L) {
    struct net_server **sp = luaL_checkudata(L, 1, SERVER_MT);
    struct net_event event;

    if (*sp && net_server_poll(*sp, &event)) {
        return push_event(L, &event);
    }

    lua_pushnil(L);

    return 1;
}

static int l_server_close(lua_State *L) {
    struct net_server **sp = luaL_checkudata(L, 1, SERVER_MT);
    if (*sp) {
        net_server_destroy(*sp);
        *sp = NULL;
    }

    return 0;
}

static int l_server_send(lua_State *L) {
    struct net_server **sp = luaL_checkudata(L, 1, SERVER_MT);
    const uint32_t client_id = (uint32_t)luaL_checkint(L, 2);
    size_t len;
    const char *data = luaL_checklstring(L, 3, &len);

    if (*sp) {
        net_server_send(*sp, client_id, data, (uint32_t)len);
    }

    return 0;
}

static int l_server_broadcast(lua_State *L) {
    struct net_server **sp = luaL_checkudata(L, 1, SERVER_MT);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);

    if (*sp) {
        net_server_broadcast(*sp, data, (uint32_t)len);
    }

    return 0;
}

static const luaL_Reg server_methods[] = {
    {"poll", l_server_poll},           {"send", l_server_send},
    {"broadcast", l_server_broadcast}, {"close", l_server_close},
    {"__gc", l_server_close},          {NULL, NULL},
};

// client

static int l_client_new(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    const uint16_t port = (uint16_t)luaL_checkint(L, 2);

    struct net_client *c = net_client_create(host, port);

    if (!c) {
        return luaL_error(L, "core.client.new: failed for %s:%d", host, port);
    }

    struct net_client **cp = lua_newuserdata(L, sizeof *cp);
    *cp = c;

    voice_set_client(c);

    luaL_getmetatable(L, CLIENT_MT);
    lua_setmetatable(L, -2);

    return 1;
}

static int l_client_poll(lua_State *L) {
    struct net_client **cp = luaL_checkudata(L, 1, CLIENT_MT);
    struct net_event ev;

    if (*cp && net_client_poll(*cp, &ev)) {
        return push_event(L, &ev);
    }

    lua_pushnil(L);
    return 1;
}

static int l_client_connected(lua_State *L) {
    struct net_client **cp = luaL_checkudata(L, 1, CLIENT_MT);
    lua_pushboolean(L, *cp && (*cp)->connected);

    return 1;
}

static int l_client_close(lua_State *L) {
    struct net_client **cp = luaL_checkudata(L, 1, CLIENT_MT);
    if (*cp) {
        net_client_destroy(*cp);
        *cp = NULL;
    }

    voice_set_client(NULL);
    return 0;
}

static int l_client_send(lua_State *L) {
    struct net_client **cp = luaL_checkudata(L, 1, CLIENT_MT);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);

    if (*cp && (*cp)->connected) {
        net_client_send(*cp, data, (uint32_t)len);
    }

    return 0;
}

static const luaL_Reg client_methods[] = {
    {"poll", l_client_poll},           {"send", l_client_send},
    {"connected", l_client_connected}, {"close", l_client_close},
    {"__gc", l_client_close},          {NULL, NULL}};

static void meta(lua_State *L, const char *name, const luaL_Reg *methods) {
    luaL_newmetatable(L, name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    for (const luaL_Reg *f = methods; f->name; f++) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }

    lua_pop(L, 1);
}

static int l_local_load(lua_State *L) {
    const char *locale = luaL_checkstring(L, 1);

    if (local_load(locale) != 0) {
        return luaL_error(L, "core.local_load: no locale %s", locale);
    }

    return 0;
}

static int l_local_get(lua_State *L) {
    const char *key = luaL_checkstring(L, 1);
    const char *value = local_get(key);

    lua_pushstring(L, value);

    return 1;
}

static int l_local_current_locale(lua_State *L) {
    const char *locale = local_current_locale();
    lua_pushstring(L, locale);

    return 1;
}

////////////////
/* voice chat */
////////////////

static int l_voice_init(lua_State *L) {
    (void)L;
    if (voice_init() != 0)
        return luaL_error(L, "core.voice.init: failed to initialize");
    return 0;
}

static int l_voice_mute(lua_State *L) {
    voice_set_muted(lua_toboolean(L, 1));
    return 0;
}

static int l_voice_volume(lua_State *L) {
    voice_set_volume((float)luaL_checknumber(L, 1));
    return 0;
}

static int l_voice_transmit(lua_State *L) {
    voice_set_ptt_active(lua_toboolean(L, 1));
    return 0;
}

static const luaL_Reg api[] = {
    {"quit", l_quit},
    {"print", l_print},

    /* Render */
    {"push_rect", l_push_rect},
    {"push_rect_ex", l_push_rect_ex},
    {"push_circle", l_push_circle},
    {"push_texture", l_push_texture},
    {"push_texture_ex", l_push_texture_ex},
    {"push_text", l_push_text},
    {"push_text_ex", l_push_text_ex},

    {"load_texture", l_load_texture},
    {"load_font", l_load_font},
    {"create_softbody", l_create_softbody},
    {"update_softbody", l_update_softbody},
    {"softbody_apply_velocity", l_softbody_apply_velocity},
    {"draw_softbody", l_draw_softbody},
    {"destroy_softbody", l_destroy_softbody},
    {"get_screen_dimensions", l_get_screen_dimensions},

    /* ui */
    {"button", l_button},
    {"screen_to_world", l_screen_to_world},

    /* collison */
    {"check_point_circle", l_check_point_circle},
    {"check_point_rect", l_check_point_rect},

    /* localization */
    {"local_load", l_local_load},
    {"local_get", l_local_get},
    {"local_current_locale", l_local_current_locale},

    /* Input */
    {"key_down", l_key_down},
    {"key_just_down", l_key_just_down},
    {"key_just_up", l_key_just_up},
    {"mouse_down", l_mouse_down},
    {"mouse_just_down", l_mouse_just_down},
    {"mouse_just_up", l_mouse_just_up},
    {"mouse_position", l_mouse_position},
    {"get_text_input", l_get_text_input},

    {"read_stdin", l_read_stdin},
    {NULL, NULL},
};

static void keys_init(lua_State *L) {
    int i;
    char key[2];
    key[1] = '\0';

    lua_newtable(L);

    for (i = GLFW_KEY_A; i <= GLFW_KEY_Z; i++) {
        lua_pushinteger(L, i);
        key[0] = (char)(i - GLFW_KEY_A + 'a');
        lua_setfield(L, -2, key);
    }

    for (i = GLFW_KEY_A; i <= GLFW_KEY_Z; i++) {
        lua_pushinteger(L, i);
        key[0] = (char)(i - GLFW_KEY_A + 'A');
        lua_setfield(L, -2, key);
    }

    for (i = GLFW_KEY_0; i <= GLFW_KEY_9; i++) {
        lua_pushinteger(L, i);
        key[0] = (char)(i - GLFW_KEY_0 + '0');
        lua_setfield(L, -2, key);
    }

    lua_pushinteger(L, GLFW_KEY_SPACE);
    lua_setfield(L, -2, "space");

    lua_pushinteger(L, GLFW_KEY_ENTER);
    lua_setfield(L, -2, "return");

    lua_pushinteger(L, GLFW_KEY_ENTER);
    lua_setfield(L, -2, "enter");

    lua_pushinteger(L, GLFW_KEY_TAB);
    lua_setfield(L, -2, "tab");

    lua_pushinteger(L, GLFW_KEY_ESCAPE);
    lua_setfield(L, -2, "escape");

    lua_pushinteger(L, GLFW_KEY_BACKSPACE);
    lua_setfield(L, -2, "backspace");

    lua_pushinteger(L, GLFW_KEY_LEFT_SHIFT);
    lua_setfield(L, -2, "left_shift");

    lua_pushinteger(L, GLFW_KEY_RIGHT_SHIFT);
    lua_setfield(L, -2, "right_shift");

    lua_pushinteger(L, GLFW_KEY_LEFT_CONTROL);
    lua_setfield(L, -2, "left_control");

    lua_pushinteger(L, GLFW_KEY_RIGHT_CONTROL);
    lua_setfield(L, -2, "right_control");

    lua_pushinteger(L, GLFW_KEY_LEFT_ALT);
    lua_setfield(L, -2, "left_alt");

    lua_pushinteger(L, GLFW_KEY_RIGHT_ALT);
    lua_setfield(L, -2, "right_alt");

    lua_pushinteger(L, GLFW_KEY_UP);
    lua_setfield(L, -2, "up");

    lua_pushinteger(L, GLFW_KEY_DOWN);
    lua_setfield(L, -2, "down");

    lua_pushinteger(L, GLFW_KEY_LEFT);
    lua_setfield(L, -2, "left");

    lua_pushinteger(L, GLFW_KEY_RIGHT);
    lua_setfield(L, -2, "right");

    lua_pushinteger(L, GLFW_KEY_DELETE);
    lua_setfield(L, -2, "delete");

    lua_pushinteger(L, GLFW_KEY_HOME);
    lua_setfield(L, -2, "home");

    lua_pushinteger(L, GLFW_KEY_END);
    lua_setfield(L, -2, "end");

    for (i = 0; i < 12; i++) {
        char name[4];
        snprintf(name, sizeof(name), "f%d", i + 1);

        lua_pushinteger(L, GLFW_KEY_F1 + i);
        lua_setfield(L, -2, name);
    }

    lua_setglobal(L, "key");
}

static const char *mouse_names[] = {
    "left",     "right",    "middle",   "button_4",
    "button_5", "button_6", "button_7", "button_8",
};

static void mouse_init(lua_State *L) {
    lua_newtable(L);

    for (int i = 0; i < 8; i++) {
        lua_pushinteger(L, i);
        lua_setfield(L, -2, mouse_names[i]);
    }

    lua_setglobal(L, "mouse");
}

// this archive loader is needed so lua modules can be properly found

static int archive_loader(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);

    char filename[64];
    snprintf(filename, sizeof(filename), "%s.lua", name);

    uint32_t len;
    char *buf = archive_read_alloc(SAUSAGES_DATA, filename, &len);
    if (!buf) {
        lua_pushfstring(L, "\n\tno file '%s' in archive", filename);
        return 1;
    }

    if (luaL_loadbuffer(L, buf, len, filename) != 0) {
        free(buf);
        return lua_error(L);
    }

    free(buf);
    return 1;
}

static void loader_init(lua_State *L) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaders");

    // insert it at index 2 which is after preload
    int n = (int)lua_objlen(L, -1);
    for (int i = n; i >= 2; i--) {
        lua_rawgeti(L, -1, i);
        lua_rawseti(L, -2, i + 1);
    }

    lua_pushcfunction(L, archive_loader);
    lua_rawseti(L, -2, 2);

    lua_pop(L, 2);
}

void lua_api_init(lua_State *L) {
    meta(L, SERVER_MT, server_methods);
    meta(L, CLIENT_MT, client_methods);

    lua_newtable(L);
    for (const luaL_Reg *f = api; f->name; f++) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }

    /* core.server */
    lua_newtable(L);
    lua_pushcfunction(L, l_server_new);
    lua_setfield(L, -2, "new");
    lua_setfield(L, -2, "server");

    /* core.client */
    lua_newtable(L);
    lua_pushcfunction(L, l_client_new);
    lua_setfield(L, -2, "new");
    lua_setfield(L, -2, "client");

    /* core.net_event */
    lua_newtable(L);
    lua_pushinteger(L, NET_EVENT_CONNECT);
    lua_setfield(L, -2, "connect");
    lua_pushinteger(L, NET_EVENT_DISCONNECT);
    lua_setfield(L, -2, "disconnect");
    lua_pushinteger(L, NET_EVENT_DATA);
    lua_setfield(L, -2, "data");
    lua_setfield(L, -2, "net_event");

    /* core.anchor */
    lua_newtable(L);
    lua_pushinteger(L, ANCHOR_TOP_LEFT);
    lua_setfield(L, -2, "top_left");
    lua_pushinteger(L, ANCHOR_BOTTOM_LEFT);
    lua_setfield(L, -2, "bottom_left");
    lua_pushinteger(L, ANCHOR_CENTER);
    lua_setfield(L, -2, "center");
    lua_setfield(L, -2, "anchor");

    /* core.voice */
    lua_newtable(L);
    lua_pushcfunction(L, l_voice_init);
    lua_setfield(L, -2, "init");
    lua_pushcfunction(L, l_voice_mute);
    lua_setfield(L, -2, "mute");
    lua_pushcfunction(L, l_voice_volume);
    lua_setfield(L, -2, "volume");
    lua_pushcfunction(L, l_voice_transmit);
    lua_setfield(L, -2, "transmit");
    lua_setfield(L, -2, "voice");

    lua_setglobal(L, "core");

    keys_init(L);
    mouse_init(L);
    loader_init(L);
}
