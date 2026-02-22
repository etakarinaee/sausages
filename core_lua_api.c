#include <stdio.h>
#include <stdlib.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>

static int quit(lua_State *L) {
    (void) L;
    exit(0);

    return 0;
}

static int print(lua_State *L) {
    printf("%s\n", luaL_checkstring(L, 1));

    return 0;
}

static const luaL_Reg api[] = {
    {"quit", quit},
    {"print", print},
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
