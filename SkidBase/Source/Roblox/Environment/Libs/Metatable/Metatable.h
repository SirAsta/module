#pragma once
#include "lua.h"
#include "lualib.h"
#include "lobject.h"
#include "lstate.h"
#include "lapi.h"
#include "ltable.h"
#include "lgc.h"
#include <string>

namespace metatable
{
    inline int getrawmetatable(lua_State* L) {
        luaL_checkany(L, 1);
        if (!lua_getmetatable(L, 1)) lua_pushnil(L);
        return 1;
    }

    inline int setrawmetatable(lua_State* L) {
        luaL_checkany(L, 1);
        int mt = lua_type(L, 2);
        if (mt != LUA_TTABLE && mt != LUA_TNIL)
            luaL_typeerror(L, 2, "table or nil");
        lua_settop(L, 2);
        lua_setmetatable(L, 1);
        lua_pushvalue(L, 1);
        return 1;
    }

    inline int setreadonly(lua_State* L) {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TBOOLEAN);
        hvalue(luaA_toobject(L, 1))->readonly = lua_toboolean(L, 2);
        return 0;
    }

    inline int isreadonly(lua_State* L) {
        luaL_checktype(L, 1, LUA_TTABLE);
        lua_pushboolean(L, hvalue(luaA_toobject(L, 1))->readonly);
        return 1;
    }

    inline int getnamecallmethod(lua_State* L) {
        const char* namecall = lua_namecallatom(L, nullptr);
        if (!namecall) { lua_pushnil(L); return 1; }
        lua_pushstring(L, namecall);
        return 1;
    }

    inline int hookmetamethod(lua_State* L) {
        luaL_checkany(L, 1);
        luaL_checktype(L, 2, LUA_TSTRING);
        luaL_checktype(L, 3, LUA_TFUNCTION);

        if (!lua_getmetatable(L, 1))
            luaL_argerror(L, 1, "object has no metatable");

        const int mta = lua_gettop(L);
        const char* method = lua_tostring(L, 2);
        lua_rawgetfield(L, mta, method);

        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            luaL_argerror(L, 2, "metamethod is not a function");
        }
        const int fff = lua_gettop(L);
        const bool locked = lua_getreadonly(L, mta);
        if (locked)
            lua_setreadonly(L, mta, false);

        lua_pushvalue(L, 3);
        lua_rawsetfield(L, mta, method);

        if (locked)
            lua_setreadonly(L, mta, true);
        lua_pushvalue(L, fff);
        return 1;
    }

}
