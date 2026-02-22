#include <stdio.h>
#include <stdlib.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>

#include "renderer.h"

static struct vec2 check_vec2(lua_State *L, int idx) {
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

static struct color3 check_color3(lua_State *L, int idx) {
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

static int quit(lua_State *L) {
    (void) L;
    exit(0);

    return 0;
}

static int print(lua_State *L) {
    printf("%s\n", luaL_checkstring(L, 1));

    return 0;
}

static int push_quad(lua_State *L) {
    struct color3 color;
    struct vec2 pos;

    pos = check_vec2(L, 1);
    color = check_color3(L, 2);

    renderer_push_quad(&ctx, pos, 1.0f, 0.0f, color);

    return 0;
}

static const luaL_Reg api[] = {
    {"quit", quit},
    {"print", print},
    {"push_quad", push_quad},
    {NULL, NULL},
};

void lua_api_init(lua_State *L) {
    const luaL_Reg *f;

    lua_newtable(L);
    for (f = api; f->name; f++) {
        lua_pushcfunction(L, f->func);
        lua_setfield(L, -2, f->name);
    }

    lua_setglobal(L, "core");
}
