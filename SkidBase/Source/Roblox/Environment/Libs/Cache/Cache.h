#pragma once
#include "lua.h"
#include "lualib.h"
#include <Windows.h>
#include <string>
#include "../../../Offsets/Offsets.h"

namespace cache_lib
{
    inline int invalidate(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        const auto a1 = *static_cast<void**>(lua_touserdata(L, 1));

        lua_pushlightuserdata(L, reinterpret_cast<void*>(Main::Functions::PushInstance));
        lua_gettable(L, LUA_REGISTRYINDEX);

        if (lua_istable(L, -1))
        {
            lua_pushlightuserdata(L, reinterpret_cast<void*>(a1));
            lua_pushnil(L);
            lua_settable(L, -3);
        }

        return 0;
    }

    inline int replace(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        luaL_checktype(L, 2, LUA_TUSERDATA);

        const auto a1 = *static_cast<void**>(lua_touserdata(L, 1));

        lua_pushlightuserdata(L, reinterpret_cast<void*>(Main::Functions::PushInstance));
        lua_gettable(L, LUA_REGISTRYINDEX);

        if (lua_istable(L, -1))
        {
            lua_pushlightuserdata(L, a1);
            lua_pushvalue(L, 2);
            lua_settable(L, -3);
        }

        return 0;
    }

    inline int iscached(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        const auto a1 = *static_cast<void**>(lua_touserdata(L, 1));

        lua_pushlightuserdata(L, reinterpret_cast<void*>(Main::Functions::PushInstance));
        lua_gettable(L, LUA_REGISTRYINDEX);

        if (lua_istable(L, -1))
        {
            lua_pushlightuserdata(L, a1);
            lua_gettable(L, -2);
            lua_pushboolean(L, lua_type(L, -1) != LUA_TNIL);
            return 1;
        }

        lua_pushboolean(L, false);
        return 1;
    }

    inline int cloneref(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        const auto a1 = *static_cast<void**>(lua_touserdata(L, 1));

        lua_pushlightuserdata(L, reinterpret_cast<void*>(Main::Functions::PushInstance));
        lua_rawget(L, LUA_REGISTRYINDEX);
        const int t1 = lua_gettop(L);

        lua_pushlightuserdata(L, a1);
        lua_rawget(L, t1);
        const int r1 = lua_gettop(L);

        lua_pushlightuserdata(L, a1);
        lua_pushnil(L);
        lua_rawset(L, t1);

        lua_getmetatable(L, 1);
        const int mt = lua_gettop(L);

        using pushinst_fn = void(__fastcall*)(lua_State*, void*);
        auto pfn = reinterpret_cast<pushinst_fn>(Main::Functions::PushInstance);
        pfn(L, lua_touserdata(L, 1));
        const int n1 = lua_gettop(L);

        if (!lua_isnil(L, mt))
        {
            lua_pushvalue(L, mt);
            lua_setmetatable(L, n1);
        }

        lua_pushlightuserdata(L, a1);
        lua_pushvalue(L, r1);
        lua_rawset(L, t1);

        lua_pushvalue(L, n1);
        return 1;
    }
}
