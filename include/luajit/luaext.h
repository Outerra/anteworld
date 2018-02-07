#ifndef luaext_h
#define luaext_h

extern "C" {
#include "luaconf.h"
}

namespace coid {
    struct token;
}

#define lua_totoken(L,i)	lua_toltoken(L, (i), NULL)
#define lua_pushbot(L)  (lua_pushvalue(L, 1), lua_remove(L, 1))
LUA_API void  (lua_pushtoken)(lua_State *L, const coid::token& s);
LUA_API coid::token (lua_toltoken)(lua_State *L, int idx, size_t *len);
LUA_API int   (lua_load)(lua_State *L, lua_Reader reader, void *dt,
    const coid::token& name);
LUA_API int (luaL_loadbuffer)(lua_State *L, const char *buff, size_t sz,
    const coid::token& name);

LUA_API void lua_getfield(lua_State *L, int idx, const coid::token& k);
LUA_API void lua_setfield(lua_State *L, int idx, const coid::token& k);
LUA_API bool lua_hasfield(lua_State *L, int idx, const coid::token& k);
LUA_API void luaL_where_ext(lua_State *L, int level);
//LUA_API void lua_pushcopy(lua_State *L, int idx);

#endif 