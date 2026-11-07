#pragma once
#include "lua.h"
#include "lualib.h"
#include "lobject.h"
#include "lstate.h"
#include "lapi.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "../../../../Core/Execution/Execution.h"
#include <string>
#include <Windows.h>

namespace closures
{
    static inline Closure* to_closure(lua_State* L, int index)
    {
        luaL_checktype(L, index, LUA_TFUNCTION);
        return clvalue(index2addr(L, index));
    }

    static inline void push_closure(lua_State* L, Closure* closure)
    {
        luaC_threadbarrier(L);
        setclvalue(L, L->top, closure);
        incr_top(L);
    }

    static Closure* clone_closure(lua_State* L, Closure* source)
    {
        Closure* clone = nullptr;

        if (source->isC)
        {
            clone = luaF_newCclosure(L, source->nupvalues, source->env);
            clone->stacksize = source->stacksize;
            clone->preload = source->preload;
            clone->c.f = source->c.f;
            clone->c.cont = static_cast<lua_Continuation>(source->c.cont);
            clone->c.debugname = source->c.debugname;

            for (int i = 0; i < source->nupvalues; ++i)
                setobj2n(L, &clone->c.upvals[i], &source->c.upvals[i]);
        }
        else
        {
            clone = luaF_newLclosure(L, source->nupvalues, source->env, source->l.p);
            clone->stacksize = source->stacksize;
            clone->preload = source->preload;

            for (int i = 0; i < source->nupvalues; ++i)
                setobj2n(L, &clone->l.uprefs[i], &source->l.uprefs[i]);
        }

        return clone;
    }

    inline int loadstring(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        size_t len = 0;
        const char* src = lua_tolstring(L, 1, &len);
        const char* chunkname = luaL_optstring(L, 2, "SkidBase");

        std::string bytecode = Execution::aexecute(std::string(src, len));
        if (bytecode.empty() || bytecode[0] == '\0')
        {
            lua_pushnil(L);
            lua_pushstring(L, bytecode.c_str() + 1);
            return 2;
        }

        if (luau_load(L, chunkname, bytecode.c_str(), bytecode.size(), 0) != LUA_OK)
        {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        Closure* fn = clvalue(const_cast<TValue*>(luaA_toobject(L, -1)));
        if (fn && fn->l.p)
            Execution::setprotocapabilities(fn->l.p, const_cast<uintptr_t*>(&Execution::caps));

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
        return 1;
    }

    inline int checkcaller(lua_State* L)
    {
        bool isOur = (L != nullptr && L->userdata != nullptr && (L->userdata->capabilities & 0b1000000));
        lua_pushboolean(L, isOur);
        return 1;
    }

    inline int iscclosure(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        Closure* c = to_closure(L, 1);
        lua_pushboolean(L, c->isC);
        return 1;
    }

    inline int islclosure(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        Closure* c = to_closure(L, 1);
        lua_pushboolean(L, !c->isC);
        return 1;
    }

    inline int clonefunction(lua_State* L)
    {
        Closure* source = to_closure(L, 1);
        push_closure(L, clone_closure(L, source));
        return 1;
    }

    inline int identifyexecutor(lua_State* L)
    {
        lua_pushstring(L, "SkidBase");
        lua_pushstring(L, "1.0.0");
        return 2;
    }

    static inline HMODULE GetCurrentModule()
    {
        static HMODULE hMod = nullptr;
        if (!hMod)
        {
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&to_closure, &hMod);
        }
        return hMod;
    }

    static inline bool IsInsideOurModule(void* ptr)
    {
        if (!ptr) return false;
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) != 0)
        {
            return mbi.AllocationBase == GetCurrentModule();
        }
        return false;
    }

    static inline bool IsExecutorProto(Proto* proto)
    {
        if (!proto) return false;
        void* raw = proto->userdata.Get();
        if (raw && *static_cast<uintptr_t*>(raw) == 0xFFFFFFFFFFFFFFFFULL)
            return true;
        for (int i = 0; proto->p && i < proto->sizep; ++i)
        {
            if (IsExecutorProto(proto->p[i]))
                return true;
        }
        return false;
    }

    inline int isexecutorclosure(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        Closure* cl = to_closure(L, 1);
        if (!cl) { lua_pushboolean(L, false); return 1; }
        if (cl->isC)
        {
            lua_pushboolean(L, IsInsideOurModule(reinterpret_cast<void*>(cl->c.f)));
            return 1;
        }
        lua_pushboolean(L, IsExecutorProto(cl->l.p));
        return 1;
    }

    inline int getcallingscript(lua_State* L)
    {
        for (CallInfo* frame = L->ci; frame >= L->base_ci; --frame)
        {
            if (!frame || !frame->func || !ttisfunction(frame->func))
                continue;

            Closure* fn = clvalue(frame->func);
            if (!fn || !fn->env)
                continue;

            luaC_threadbarrier(L);
            sethvalue(L, L->top, fn->env);
            incr_top(L);

            lua_getfield(L, -1, "script");
            lua_remove(L, -2);

            if (!lua_isnil(L, -1))
                return 1;

            lua_pop(L, 1);
        }

        lua_pushnil(L);
        return 1;
    }
}