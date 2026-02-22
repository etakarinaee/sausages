#include "lua.h"
#include "lua_api.h"

#include <luajit-2.1/lualib.h>
#include <luajit-2.1/lauxlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "archive.h"

static long last_mtime;

static long fmtime(const char *path) {
    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }

    return st.st_mtime;
}

/* n - amount of arguments */
static int call(lua_State *L, const char *name, const int n) {
    if (lua_pcall(L, n, 0, 0) != 0) {
        fprintf(stderr, "%s: %s\n", name, lua_tostring(L, -1));
        lua_pop(L, 1);

        return -1;
    }

    return 0;
}

static int push(lua_State *L, const char *name) {
    lua_getglobal(L, name);

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);

        return -1;
    }

    return 0;
}

lua_State *lua_init(const char *archive, const char *entry) {
    uint32_t len;

    lua_State *L = luaL_newstate();
    if (!L) {
        return NULL;
    }

    luaL_openlibs(L);
    lua_api_init(L);

    char *buf = archive_read_alloc(archive, entry, &len);
    if (!buf) {
        lua_close(L);

        return NULL;
    }

    if (luaL_loadbuffer(L, buf, len, entry) != 0) {
        fprintf(stderr, "%s: %s\n", entry, lua_tostring(L, -1));
        free(buf);
        lua_close(L);

        return NULL;
    }

    free(buf);

    if (lua_pcall(L, 0, 0, 0) != 0) {
        fprintf(stderr, "%s: %s\n", entry, lua_tostring(L, -1));
        lua_close(L);

        return NULL;
    }

    last_mtime = fmtime(archive);

    return L;
}

void lua_quit(lua_State *L) {
    if (L) {
        lua_close(L);
    }
}

lua_State *lua_reload(lua_State *L, const char *archive, const char *entry) {
    /* the new lua state */

    const long mtime = fmtime(archive);
    if (mtime <= last_mtime) {
        return L;
    }

    lua_State *N = lua_init(archive, entry);
    if (!N) {
        last_mtime = mtime;

        return L;
    }

    lua_call_quit(L);
    lua_close(L);
    lua_call_init(N);

    return N;
}

void lua_call_init(lua_State *L) {
    if (push(L, "game_init") == 0) {
        call(L, "game_init", 0);
    }
}

void lua_call_update(lua_State *L, double delta_time) {
    if (push(L, "game_update") == 0) {
        lua_pushnumber(L, delta_time);
        call(L, "game_update", 1);
    }
}

void lua_call_quit(lua_State *L) {
    if (push(L, "game_quit") == 0) {
        call(L, "game_quit", 0);
    }
}
