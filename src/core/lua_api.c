#include <stdio.h>
#include <stdlib.h>

#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>

#include "renderer.h"
#include <GLFW/glfw3.h> /* after renderer because renderer includes glad which muss be included after glfw */

#include "archive.h"
#include "net.h"

// metatables
#define SERVER_MT "net_server"
#define CLIENT_MT "net_client"

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

static int l_quit(lua_State *L) {
    (void) L;
    exit(0);
}

static int l_print(lua_State *L) {
    printf("%s\n", luaL_checkstring(L, 1));

    return 0;
}

static int l_get_screen_dimensions(lua_State *L) {
    int width, height;
    glfwGetWindowSize(ctx.window, &width, &height);

    lua_newtable(L);
    lua_pushinteger(L, width);
    lua_setfield(L, -2, "width");

    lua_pushinteger(L, height);
    lua_setfield(L, -2, "height");

    return 1;
}

static int l_push_texture(lua_State *L) {
    const struct vec2 pos = check_vec2(L, 1);
    const struct vec2 scale = check_vec2(L, 2);
    const texture_id tex = luaL_checkint(L, 3);

    renderer_push_texture(&ctx, pos, scale, 0.0f, tex);

    return 0;
}

static int l_load_texture(lua_State *L) {
    const texture_id tex = renderer_load_texture(luaL_checkstring(L, 1));
    lua_pushinteger(L, tex);
    return 1;
}

static int l_key_pressed(lua_State *L) {
    if (glfwGetKey(ctx.window, luaL_checkint(L, 1)) == GLFW_PRESS) {
        lua_pushinteger(L, 1);
        return 1;
    }
    return 0;
}

static int l_key_down(lua_State *L) {
    if (glfwGetKey(ctx.window, luaL_checkint(L, 1)) != GLFW_RELEASE) {
        lua_pushinteger(L, 1);
        return 1;
    }
    return 0;
}

static int l_mouse_pressed(lua_State *L) {
    if (glfwGetMouseButton(ctx.window, luaL_checkint(L, 1)) == GLFW_PRESS) {
        lua_pushinteger(L, 1);
        return 1;
    }
    return 0;
}

static int l_mouse_down(lua_State *L) {
    if (glfwGetMouseButton(ctx.window, luaL_checkint(L, 1)) != GLFW_RELEASE) {
        lua_pushinteger(L, 1);
        return 1;
    }
    return 0;
}

static int l_mouse_pos(lua_State *L) {
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

// networking

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
        lua_pushlstring(L, (const char *) event->data, event->len);
        lua_setfield(L, -2, "data");
    }


    return 1;
}

// server

static int l_server_new(lua_State *L) {
    const uint16_t port = (uint16_t) luaL_checkint(L, 1);
    const uint32_t n = (uint32_t) luaL_optint(L, 2, 32);

    struct net_server *server = net_server_create(port, n);
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
    const uint32_t client_id = (uint32_t) luaL_checkint(L, 2);
    size_t len;
    const char *data = luaL_checklstring(L, 3, &len);

    if (*sp) {
        net_server_send(*sp, client_id, data, (uint32_t) len);
    }

    return 0;
}

static int l_server_broadcast(lua_State *L) {
    struct net_server **sp = luaL_checkudata(L, 1, SERVER_MT);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);

    if (*sp) {
        net_server_broadcast(*sp, data, (uint32_t) len);
    }

    return 0;
}

static const luaL_Reg server_methods[] = {
    {"poll", l_server_poll},
    {"send", l_server_send},
    {"broadcast", l_server_broadcast},
    {"close", l_server_close},
    {"__gc", l_server_close},
    {NULL,NULL},
};

// client

static int l_client_new(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    const uint16_t port = (uint16_t) luaL_checkint(L, 2);

    struct net_client *c = net_client_create(host, port);

    if (!c) {
        return luaL_error(L, "core.client.new: failed for %s:%d", host, port);
    }

    struct net_client **cp = lua_newuserdata(L, sizeof *cp);
    *cp = c;

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

    return 0;
}

static int l_client_send(lua_State *L) {
    struct net_client **cp = luaL_checkudata(L, 1, CLIENT_MT);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);

    if (*cp && (*cp)->connected) {
        net_client_send(*cp, data, (uint32_t) len);
    }

    return 0;
}

static const luaL_Reg client_methods[] = {
    {"poll", l_client_poll},
    {"send", l_client_send},
    {"connected", l_client_connected},
    {"close", l_client_close},
    {"__gc", l_client_close},
    {NULL, NULL}
};

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


static const luaL_Reg api[] = {
    {"quit", l_quit},
    {"print", l_print},

    /* Render */
    {"push_texture", l_push_texture},
    {"load_texture", l_load_texture},
    {"get_screen_dimensions", l_get_screen_dimensions},

    /* Input */
    {"key_pressed", l_key_pressed},
    {"key_down", l_key_down},
    {"mouse_pressed", l_mouse_pressed},
    {"mouse_down", l_mouse_down},
    {"mouse_pos", l_mouse_pos},
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
    int n = (int) lua_objlen(L, -1);
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

    lua_setglobal(L, "core");

    keys_init(L);
    mouse_init(L);
    loader_init(L);
}
