#ifndef LUA_H
#define LUA_H

#include <luajit-2.1/lua.h>

lua_State *lua_init(const char *archive, const char *entry);

void lua_quit(lua_State * L);

lua_State *lua_reload(lua_State *L, const char *archive, const char *entry);

void lua_call_init(lua_State * L);

void lua_call_update(lua_State *L, double delta_time);

void lua_call_quit(lua_State * L);

#endif /* LUA_H */
