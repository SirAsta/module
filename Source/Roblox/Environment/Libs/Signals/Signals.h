#pragma once
#include <Windows.h>
#include <lua.h>
#include <lualib.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include "lapi.h"
#include "lobject.h"
#include "ldo.h"
#include "lstate.h"
#include "../../../Offsets/Offsets.h"

namespace signals_lib
{
    namespace off
    {
        constexpr uintptr_t o_next = 0x10;
        constexpr uintptr_t o_act = 0x20;
        constexpr uintptr_t o_swrap = 0x30;
        constexpr uintptr_t o_sself = 0x38;
        constexpr uintptr_t o_th = 0x28;
        constexpr uintptr_t o_fid = 0x34;
        constexpr uintptr_t o_wobj = 0x60;
    }

    template <typename T>
    static bool rmem(uintptr_t a1, T* out)
    {
        if (!a1 || !out) return false;
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<void*>(a1), &mbi, sizeof(mbi))) return false;
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
        const uintptr_t s1 = a1;
        const uintptr_t e1 = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (s1 + sizeof(T) < s1 || s1 + sizeof(T) > e1) return false;
        __try
        {
            *out = *reinterpret_cast<const T*>(a1);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    template <typename T>
    static bool wmem(uintptr_t a1, const T& val)
    {
        if (!a1) return false;
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<void*>(a1), &mbi, sizeof(mbi))) return false;
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
        __try
        {
            *reinterpret_cast<T*>(a1) = val;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    static bool chkptr(uintptr_t a1)
    {
        if (!a1 || a1 < 0x10000 || a1 > 0x7FFFFFFFFFFF) return false;
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<void*>(a1), &mbi, sizeof(mbi))) return false;
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
        return true;
    }

    static uintptr_t gwobj(uintptr_t a1)
    {
        uintptr_t w1 = 0;
        if (!rmem(a1 + off::o_swrap, &w1) || !w1)
            return 0;

        uintptr_t s1 = 0;
        rmem(a1 + off::o_sself, &s1);

        if (s1 && chkptr(s1) && (w1 == s1 + 0x18))
        {
            uintptr_t o1 = 0;
            if (rmem(w1 + off::o_wobj, &o1) && chkptr(o1))
                return o1;
            if (rmem(w1 + 0x48, &o1) && chkptr(o1))
                return o1;
        }

        uintptr_t v0 = 0, v1 = 0, v2 = 0;
        if (rmem(a1 + 0x68, &v0) && chkptr(v0) &&
            rmem(v0 + 8, &v1) && chkptr(v1) &&
            rmem(v1 + off::o_wobj, &v2) && chkptr(v2))
        {
            return v2;
        }
        return 0;
    }

    static lua_State* gth(uintptr_t a1)
    {
        if (!a1) return nullptr;
        lua_State* th = nullptr;
        if (!rmem(a1 + off::o_th, &th) || !chkptr(reinterpret_cast<uintptr_t>(th)))
            return nullptr;
        uint8_t tt = 0;
        if (!rmem(reinterpret_cast<uintptr_t>(&th->tt), &tt) || tt != LUA_TTHREAD)
            return nullptr;
        return th;
    }

    static Closure* gfn(uintptr_t a1)
    {
        lua_State* th = gth(a1);
        if (!th) return nullptr;

        auto pref = [&](int r) -> Closure*
        {
            if (r <= 0) return nullptr;
            lua_getref(th, r);
            if (lua_isfunction(th, -1))
            {
                auto* cl = clvalue(luaA_toobject(th, -1));
                lua_pop(th, 1);
                return cl;
            }
            lua_pop(th, 1);
            return nullptr;
        };

        int32_t fid = 0;
        if (rmem(a1 + off::o_fid, &fid))
        {
            if (auto* cl = pref(fid))
                return cl;
        }

        int32_t bid = 0;
        if (rmem(a1, &bid))
        {
            for (int i = 0; i <= 0xFFC; ++i)
            {
                if (auto* cl = pref(bid + i))
                    return cl;
            }
        }
        return nullptr;
    }

    static lua_State* gwth(uintptr_t a1)
    {
        uintptr_t s1 = 0;
        if (!rmem(a1 + off::o_sself, &s1) || !chkptr(s1))
            return nullptr;

        uintptr_t c1 = 0;
        if (!rmem(s1 + 0x50, &c1) || !chkptr(c1))
            return nullptr;

        lua_State* th = nullptr;
        if (!rmem(c1 + 0x28, &th) || !chkptr(reinterpret_cast<uintptr_t>(th)))
            return nullptr;

        uint8_t tt = 0;
        if (!rmem(reinterpret_cast<uintptr_t>(&th->tt), &tt) || tt != LUA_TTHREAD)
            return nullptr;

        global_State* g = nullptr;
        if (!rmem(reinterpret_cast<uintptr_t>(&th->global), &g) || !g)
            return nullptr;

        return th;
    }

    struct skid_conn
    {
        uintptr_t ptr = 0;
        uintptr_t prev = 0;
        bool act = true;
        bool frg = false;
    };

    static inline std::unordered_map<uintptr_t, int> c_cache;
    static inline std::unordered_set<uintptr_t> f_set;

    static bool is_act(uintptr_t a1)
    {
        if (!a1 || !chkptr(a1)) return false;
        uintptr_t v = 0;
        if (!rmem(a1 + off::o_act, &v)) return false;
        return v > 0;
    }

    static void set_act(uintptr_t a1, uintptr_t val)
    {
        if (!a1) return;
        wmem(a1 + off::o_act, val);
    }

    static skid_conn* skid_getnode(lua_State* L, int idx)
    {
        lua_rawgetfield(L, idx, "Object");
        if (!lua_islightuserdata(L, -1))
        {
            lua_pop(L, 1);
            return nullptr;
        }
        auto* node = reinterpret_cast<skid_conn*>(lua_tolightuserdata(L, -1));
        lua_pop(L, 1);
        return node;
    }

    static int noop(lua_State*) { return 0; }

    struct skid_bridge
    {
        std::unordered_map<const void*, int> trefs;
        std::vector<int> refs;
    };

    static bool cpyval(lua_State* src, int idx, lua_State* dst, skid_bridge& br, int d = 0)
    {
        idx = lua_absindex(src, idx);

        switch (lua_type(src, idx))
        {
        case LUA_TNIL:
            lua_pushnil(dst);
            return true;
        case LUA_TBOOLEAN:
            lua_pushboolean(dst, lua_toboolean(src, idx));
            return true;
        case LUA_TNUMBER:
            lua_pushnumber(dst, lua_tonumber(src, idx));
            return true;
        case LUA_TVECTOR:
        {
            const float* v = lua_tovector(src, idx);
            lua_pushvector(dst, v[0], v[1], v[2]);
            return true;
        }
        case LUA_TSTRING:
        {
            size_t len = 0;
            const char* s = lua_tolstring(src, idx, &len);
            lua_pushlstring(dst, s, len);
            return true;
        }
        case LUA_TBUFFER:
        {
            size_t sz = 0;
            const void* b = lua_tobuffer(src, idx, &sz);
            void* res = lua_newbuffer(dst, sz);
            if (sz && b && res)
                memcpy(res, b, sz);
            return true;
        }
        case LUA_TUSERDATA:
            if (strcmp(luaL_typename(src, idx), "Instance") == 0)
            {
                void* ud = lua_touserdata(src, idx);
                if (ud)
                {
                    void* inst = *reinterpret_cast<void**>(ud);
                    if (inst)
                    {
                        using pushinst_fn = void(__fastcall*)(lua_State*, void*);
                        auto pfn = reinterpret_cast<pushinst_fn>(Main::Functions::PushInstance);
                        pfn(dst, inst);
                        return !lua_isnil(dst, -1);
                    }
                }
            }
            lua_pushnil(dst);
            return false;
        case LUA_TTABLE:
        {
            if (d >= 32)
            {
                lua_pushnil(dst);
                return false;
            }

            const void* tid = lua_topointer(src, idx);
            const auto it = br.trefs.find(tid);
            if (it != br.trefs.end())
            {
                lua_getref(dst, it->second);
                return true;
            }

            lua_newtable(dst);
            const int t1 = lua_gettop(dst);
            const int r1 = lua_ref(dst, t1);
            br.trefs.emplace(tid, r1);
            br.refs.push_back(r1);

            lua_pushnil(src);
            while (lua_next(src, idx) != 0)
            {
                const int vp = lua_gettop(src);
                const int kp = vp - 1;
                cpyval(src, kp, dst, br, d + 1);
                cpyval(src, vp, dst, br, d + 1);
                lua_settable(dst, t1);
                lua_pop(src, 1);
            }
            return true;
        }
        default:
            lua_pushnil(dst);
            return false;
        }
    }

    static void unrefs(lua_State* L, skid_bridge& br)
    {
        for (const int r : br.refs)
            lua_unref(L, r);
    }

    static bool mk(lua_State* s1, lua_State* s2)
    {
        if (!s1 || !s2) return false;
        return s1->global != s2->global;
    }

    static bool rfn(lua_State* src, lua_State* owner, Closure* cl, int start, int cnt)
    {
        if (!owner || !cl || cl->tt != LUA_TFUNCTION)
            return false;

        const int top = lua_gettop(owner);
        lua_State* th = lua_newthread(owner);
        if (!th)
            return false;

        setclvalue(th, th->top, cl);
        incr_top(th);

        skid_bridge br;
        for (int i = 0; i < cnt; ++i)
            cpyval(src, start + i, th, br);

        const int res = lua_resume(th, owner, cnt);
        unrefs(th, br);
        lua_settop(owner, top);
        return res == LUA_OK || res == LUA_YIELD;
    }

    static bool wkw(lua_State* src, lua_State* waiter, int start, int cnt)
    {
        if (!waiter || lua_status(waiter) != LUA_YIELD)
            return false;

        skid_bridge br;
        for (int i = 0; i < cnt; ++i)
            cpyval(src, start + i, waiter, br);

        lua_State* p = waiter->global ? waiter->global->mainthread : nullptr;
        const int res = lua_resume(waiter, p, cnt);
        unrefs(waiter, br);
        return res == LUA_OK || res == LUA_YIELD;
    }

    static int conn_fire(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        auto* n = skid_getnode(L, 1);
        if (!n || !n->ptr || !is_act(n->ptr)) return 0;

        const uintptr_t w = gwobj(n->ptr);
        Closure* cl = nullptr;
        if (w)
        {
            lua_State* th = gth(w);
            const bool alien = th ? mk(L, th) : false;
            if (alien) return 0;
            cl = gfn(w);
        }

        if (cl && cl->tt == LUA_TFUNCTION)
        {
            lua_remove(L, 1);
            const int argc = lua_gettop(L);
            lua_State* co = lua_newthread(L);
            setclvalue(co, co->top, cl);
            incr_top(co);
            for (int i = 1; i <= argc; ++i)
            {
                lua_pushvalue(L, i);
                lua_xmove(L, co, 1);
            }
            lua_resume(co, L, argc);
            f_set.insert(n->ptr);
            return 0;
        }

        lua_State* wt = gwth(n->ptr);
        if (!wt && w)
            wt = gth(w);

        if (wt && wt != L && lua_status(wt) == LUA_YIELD)
        {
            set_act(n->ptr, 0);
            n->act = false;
            lua_remove(L, 1);
            const int argc = lua_gettop(L);
            for (int i = 1; i <= argc; ++i)
            {
                lua_pushvalue(L, i);
                lua_xmove(L, wt, 1);
            }
            lua_resume(wt, L, argc);
            f_set.insert(n->ptr);
            return 0;
        }

        return 0;
    }

    static int conn_defer(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        auto* n = skid_getnode(L, 1);
        if (!n || !n->ptr) return 0;
        const uintptr_t w = gwobj(n->ptr);
        if (!w) return 0;
        const auto cl = gfn(w);
        if (!cl || cl->tt != LUA_TFUNCTION) return 0;

        lua_remove(L, 1);
        const int argc = lua_gettop(L);

        lua_getglobal(L, "task");
        lua_getfield(L, -1, "defer");
        lua_remove(L, -2);

        setclvalue(L, L->top, cl);
        incr_top(L);
        for (int i = 1; i <= argc; ++i)
            lua_pushvalue(L, i);
        lua_call(L, argc + 1, 0);
        return 0;
    }

    static int conn_disable(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        auto* n = skid_getnode(L, 1);
        if (n && n->ptr) set_act(n->ptr, 0);
        return 0;
    }

    static int conn_enable(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        auto* n = skid_getnode(L, 1);
        if (n && n->ptr) set_act(n->ptr, n->prev ? n->prev : 1);
        return 0;
    }

    static int conn_disconnect(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        auto* n = skid_getnode(L, 1);
        if (!n || !n->ptr) return 0;
        set_act(n->ptr, 0);
        n->act = false;
        auto it = c_cache.find(n->ptr);
        if (it != c_cache.end())
        {
            if (it->second > 0) lua_unref(L, it->second);
            c_cache.erase(it);
        }
        return 0;
    }

    static int conn_index(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TSTRING);
        const char* k1 = lua_tostring(L, 2);
        auto* n = skid_getnode(L, 1);

        if (n && n->ptr && !n->act)
        {
            if (strcmp(k1, "Enabled") == 0 || strcmp(k1, "LuaConnection") == 0) { lua_pushboolean(L, false); return 1; }
            if (strcmp(k1, "ForeignState") == 0) { lua_pushboolean(L, n->frg); return 1; }
            if (strcmp(k1, "Function") == 0 || strcmp(k1, "Thread") == 0) { lua_pushnil(L); return 1; }
            lua_pushcfunction(L, noop, "noop");
            return 1;
        }

        if (!n || !n->ptr)
        {
            if (strcmp(k1, "Enabled") == 0 || strcmp(k1, "ForeignState") == 0 || strcmp(k1, "LuaConnection") == 0) { lua_pushboolean(L, false); return 1; }
            if (strcmp(k1, "Function") == 0 || strcmp(k1, "Thread") == 0) { lua_pushnil(L); return 1; }
            lua_pushcfunction(L, noop, "noop");
            return 1;
        }

        if (strcmp(k1, "Enabled") == 0) { lua_pushboolean(L, is_act(n->ptr)); return 1; }
        if (strcmp(k1, "Disable") == 0) { lua_pushcfunction(L, conn_disable, "Connection.Disable"); return 1; }
        if (strcmp(k1, "Disconnect") == 0) { lua_pushcfunction(L, conn_disconnect, "Connection.Disconnect"); return 1; }
        if (strcmp(k1, "Enable") == 0) { lua_pushcfunction(L, conn_enable, "Connection.Enable"); return 1; }

        const uintptr_t w = gwobj(n->ptr);
        lua_State* nth = nullptr;
        Closure* ncl = nullptr;
        if (w)
        {
            nth = gth(w);
            ncl = gfn(w);
        }

        if (nth)
        {
            global_State* ng = nullptr;
            global_State* cg = nullptr;
            rmem(reinterpret_cast<uintptr_t>(&nth->global), &ng);
            rmem(reinterpret_cast<uintptr_t>(&L->global), &cg);
            n->frg = (ng && cg && ng != cg);
        }
        else
        {
            n->frg = false;
        }

        bool hasf = false;
        if (w)
        {
            int32_t fid = 0;
            if (rmem(w + off::o_fid, &fid)) hasf = (fid > 0);
            if (!hasf)
            {
                int32_t bid = 0;
                if (rmem(w, &bid)) hasf = (bid > 0);
            }
        }

        bool islua = (ncl && !ncl->isC) || (hasf && !n->frg);
        lua_State* wt = gwth(n->ptr);
        if (wt) islua = true;

        if (strcmp(k1, "ForeignState") == 0)
        {
            lua_pushboolean(L, n->frg);
        }
        else if (strcmp(k1, "LuaConnection") == 0)
        {
            lua_pushboolean(L, islua);
        }
        else if (strcmp(k1, "Function") == 0)
        {
            if (ncl && ncl->tt == LUA_TFUNCTION)
            {
                setclvalue(L, L->top, ncl);
                incr_top(L);
            }
            else if (wt)
            {
                lua_pushvalue(L, 1);
                lua_pushcclosure(L, conn_fire, "Connection.Function", 1);
            }
            else
            {
                lua_pushnil(L);
            }
        }
        else if (strcmp(k1, "Thread") == 0)
        {
            lua_State* th = nth ? nth : wt;
            if (th && th->tt == LUA_TTHREAD)
            {
                global_State* ng = nullptr;
                global_State* cg = nullptr;
                rmem(reinterpret_cast<uintptr_t>(&th->global), &ng);
                rmem(reinterpret_cast<uintptr_t>(&L->global), &cg);
                if (ng == cg)
                {
                    setthvalue(L, L->top, th);
                    incr_top(L);
                }
                else
                {
                    lua_pushnil(L);
                }
            }
            else
            {
                lua_pushnil(L);
            }
        }
        else if (strcmp(k1, "Fire") == 0)
        {
            lua_pushcfunction(L, conn_fire, "Connection.Fire");
        }
        else if (strcmp(k1, "Defer") == 0)
        {
            lua_pushcfunction(L, conn_defer, "Connection.Defer");
        }
        else
        {
            lua_pushnil(L);
        }

        return 1;
    }

    static int conn_newindex(lua_State*) { return 0; }

    static int push_node(lua_State* L, uintptr_t a1)
    {
        auto* n = static_cast<skid_conn*>(lua_newuserdata(L, sizeof(skid_conn)));
        lua_ref(L, -1);
        lua_pop(L, 1);

        n->ptr = a1;
        uintptr_t v = 0;
        rmem(a1 + off::o_act, &v);
        n->prev = v;
        n->act = true;
        n->frg = false;

        lua_newtable(L);
        lua_pushlightuserdata(L, n);
        lua_setfield(L, -2, "Object");
        lua_getfield(L, LUA_REGISTRYINDEX, "skid_conn_sig");
        lua_setmetatable(L, -2);
        return 1;
    }

    struct skid_chain
    {
        uintptr_t links[256];
        int count = 0;
    };

    static skid_chain scan_chain(uintptr_t s1)
    {
        skid_chain ch{};
        __try
        {
            uintptr_t cur = 0;
            if (rmem(s1 + off::o_next, &cur))
            {
                while (cur && cur != s1 && ch.count < 256)
                {
                    if (!chkptr(cur)) break;
                    ch.links[ch.count++] = cur;
                    if (!rmem(cur + off::o_next, &cur)) break;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return ch;
    }

    inline int getconnections(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        if (strcmp(luaL_typename(L, 1), "RBXScriptSignal") != 0)
            luaL_typeerror(L, 1, "RBXScriptSignal");

        lua_getfield(L, 1, "Connect");
        if (!lua_isfunction(L, -1))
        {
            lua_pop(L, 1);
            luaL_error(L, "Signal does not have 'Connect' method");
        }

        lua_pushvalue(L, 1);
        lua_pushcfunction(L, noop, "sentinel_connect");
        if (lua_pcall(L, 2, 1, 0) != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            luaL_error(L, "%s", err ? err : "Connect failed");
        }

        void* ud = lua_touserdata(L, -1);
        if (!ud)
        {
            lua_pop(L, 1);
            luaL_error(L, "Connect did not return a valid connection object");
        }

        uintptr_t s1 = 0;
        if (lua_type(L, -1) == LUA_TLIGHTUSERDATA)
            s1 = reinterpret_cast<uintptr_t>(ud);
        else
            rmem(reinterpret_cast<uintptr_t>(ud), &s1);

        if (!s1)
        {
            lua_getfield(L, -1, "Disconnect");
            if (lua_isfunction(L, -1))
            {
                lua_pushvalue(L, -2);
                lua_pcall(L, 1, 0, 0);
            }
            else
            {
                lua_pop(L, 2);
            }
            lua_newtable(L);
            return 1;
        }

        skid_chain ch = scan_chain(s1);

        lua_getfield(L, -1, "Disconnect");
        if (lua_isfunction(L, -1))
        {
            lua_insert(L, -2);
            if (lua_pcall(L, 1, 0, 0) != LUA_OK)
                lua_pop(L, 1);
        }
        else
        {
            lua_pop(L, 2);
        }

        lua_createtable(L, ch.count, 0);
        int outidx = 1;
        const bool f_empty = f_set.empty();

        for (int i = 0; i < ch.count; ++i)
        {
            const uintptr_t p = ch.links[i];
            if (!p || !is_act(p))
            {
                auto it = c_cache.find(p);
                if (it != c_cache.end())
                {
                    if (it->second > 0) lua_unref(L, it->second);
                    c_cache.erase(it);
                }
                continue;
            }

            if (!f_empty && f_set.count(p))
                continue;

            bool pushed = false;
            auto it = c_cache.find(p);
            if (it != c_cache.end())
            {
                if (it->second > 0)
                {
                    lua_getref(L, it->second);
                    if (lua_istable(L, -1))
                    {
                        pushed = true;
                    }
                    else
                    {
                        lua_pop(L, 1);
                        lua_unref(L, it->second);
                        c_cache.erase(it);
                    }
                }
                else
                {
                    c_cache.erase(it);
                }
            }

            if (!pushed)
            {
                push_node(L, p);
                const int r1 = lua_ref(L, -1);
                c_cache[p] = r1;
            }

            lua_rawseti(L, -2, outidx++);
        }

        return 1;
    }

    inline int getconnection(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        const int idx = luaL_checkinteger(L, 2);
        if (idx < 1)
            luaL_argerror(L, 2, "index must be greater than zero");

        getconnections(L);
        lua_rawgeti(L, -1, idx);
        lua_remove(L, -2);
        return 1;
    }

    inline int firesignal(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        if (strcmp(luaL_typename(L, 1), "RBXScriptSignal") != 0)
            luaL_typeerror(L, 1, "RBXScriptSignal");

        const int argc = lua_gettop(L);

        getconnections(L);
        const int ctop = lua_gettop(L);

        if (!lua_istable(L, ctop))
        {
            lua_pop(L, 1);
            return 0;
        }

        lua_pushnil(L);
        while (lua_next(L, ctop))
        {
            auto* n = lua_istable(L, -1) ? skid_getnode(L, -1) : nullptr;
            if (n && n->ptr)
            {
                lua_State* wt = gwth(n->ptr);
                if (wt && wt != L && lua_status(wt) == LUA_YIELD)
                {
                    set_act(n->ptr, 0);
                    n->act = false;

                    bool ok = false;
                    if (mk(L, wt))
                    {
                        ok = wkw(L, wt, 2, argc - 1);
                    }
                    else
                    {
                        for (int i = 2; i <= argc; ++i)
                        {
                            lua_pushvalue(L, i);
                            lua_xmove(L, wt, 1);
                        }
                        const int st = lua_resume(wt, L, argc - 1);
                        ok = (st == LUA_OK || st == LUA_YIELD);
                    }

                    if (ok)
                    {
                        f_set.insert(n->ptr);
                    }
                    lua_pop(L, 1);
                    continue;
                }

                const uintptr_t w = gwobj(n->ptr);
                lua_State* owner = w ? gth(w) : nullptr;
                Closure* cl = w ? gfn(w) : nullptr;

                if (owner && cl && mk(L, owner))
                {
                    if (rfn(L, owner, cl, 2, argc - 1))
                    {
                        f_set.insert(n->ptr);
                    }
                    lua_pop(L, 1);
                    continue;
                }
            }

            lua_getfield(L, -1, "Function");
            bool hasf = !lua_isnil(L, -1) && lua_isfunction(L, -1);
            lua_pop(L, 1);

            if (hasf)
            {
                lua_getfield(L, -1, "Fire");
                if (lua_isfunction(L, -1))
                {
                    lua_pushvalue(L, -2);
                    for (int i = 2; i <= argc; ++i)
                        lua_pushvalue(L, i);
                    lua_pcall(L, argc, 0, 0);
                }
                else
                {
                    lua_pop(L, 1);
                }
            }
            else
            {
                lua_getfield(L, -1, "Thread");
                if (lua_isthread(L, -1))
                {
                    lua_State* co = lua_tothread(L, -1);
                    if (co && co != L && lua_status(co) == LUA_YIELD)
                    {
                        for (int i = 2; i <= argc; ++i)
                        {
                            lua_pushvalue(L, i);
                            lua_xmove(L, co, 1);
                        }
                        lua_resume(co, L, argc - 1);
                    }
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
        return 0;
    }

    inline int fireproximityprompt(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);

        lua_getglobal(L, "typeof");
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        const bool inst = (strcmp(lua_tolstring(L, -1, 0), "Instance") == 0);
        lua_pop(L, 1);

        if (!inst)
            luaL_argerror(L, 1, "Expected an Instance");

        lua_getfield(L, 1, "IsA");
        lua_pushvalue(L, 1);
        lua_pushstring(L, "ProximityPrompt");
        lua_call(L, 2, 1);
        const bool expected = lua_toboolean(L, -1);
        lua_pop(L, 1);

        if (!expected)
            luaL_argerror(L, 1, "Expected a ProximityPrompt");

        using fireprox_fn = void(__thiscall*)(uintptr_t);
        auto pfn = reinterpret_cast<fireprox_fn>(Main::SignalOffsets::FireProximityPrompt);
        pfn(*reinterpret_cast<std::uintptr_t*>(lua_touserdata(L, 1)));
        return 0;
    }

    inline int fireclickdetector(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TUSERDATA);

        auto d1 = static_cast<float>(luaL_optnumber(L, 2, 0));
        auto evt = luaL_optstring(L, 3, "MouseClick");

        lua_getglobal(L, "game");
        lua_getfield(L, -1, "GetService");
        lua_insert(L, -2);
        lua_pushstring(L, "Players");
        lua_pcall(L, 2, 1, 0);
        lua_getfield(L, -1, "LocalPlayer");

        void* cd = *static_cast<void**>(lua_touserdata(L, 1));
        void* pl = lua_isuserdata(L, -1) ? *static_cast<void**>(lua_touserdata(L, -1)) : nullptr;

        if (strcmp(evt, "MouseClick") == 0)
        {
            using fn_t = void(__fastcall*)(void*, float, void*);
            auto fn = reinterpret_cast<fn_t>(Main::SignalOffsets::FireMouseClick);
            fn(cd, d1, pl);
        }
        else if (strcmp(evt, "RightMouseClick") == 0)
        {
            using fn_t = void(__fastcall*)(void*, float, void*);
            auto fn = reinterpret_cast<fn_t>(Main::SignalOffsets::FireRightMouseClick);
            fn(cd, d1, pl);
        }
        else if (strcmp(evt, "MouseHoverEnter") == 0)
        {
            using fn_t = void(__fastcall*)(void*, void*);
            auto fn = reinterpret_cast<fn_t>(Main::SignalOffsets::FireMouseHoverEnter);
            fn(cd, pl);
        }
        else if (strcmp(evt, "MouseHoverLeave") == 0)
        {
            using fn_t = void(__fastcall*)(void*, void*);
            auto fn = reinterpret_cast<fn_t>(Main::SignalOffsets::FireMouseHoverLeave);
            fn(cd, pl);
        }

        return 0;
    }

    inline int setproximitypromptduration(lua_State* L)
    {
        if (!lua_isuserdata(L, 1) || !lua_isnumber(L, 2))
            return 0;

        lua_getfield(L, 1, "ClassName");
        const char* cn = lua_isstring(L, -1) ? lua_tostring(L, -1) : nullptr;
        lua_pop(L, 1);

        if (!cn || strcmp(cn, "ProximityPrompt") != 0)
            return 0;

        double d1 = lua_tonumber(L, 2);
        lua_pushnumber(L, d1);
        lua_setfield(L, 1, "HoldDuration");

        return 0;
    }

    inline int getproximitypromptduration(lua_State* L)
    {
        if (!lua_isuserdata(L, 1))
        {
            lua_pushnumber(L, 0);
            return 1;
        }

        lua_getfield(L, 1, "ClassName");
        const char* cn = lua_isstring(L, -1) ? lua_tostring(L, -1) : nullptr;
        lua_pop(L, 1);

        if (!cn || strcmp(cn, "ProximityPrompt") != 0)
        {
            lua_pushnumber(L, 0);
            return 1;
        }

        lua_getfield(L, 1, "HoldDuration");
        if (!lua_isnumber(L, -1))
        {
            lua_pop(L, 1);
            lua_pushnumber(L, 0);
            return 1;
        }

        return 1;
    }

    inline void Register(lua_State* L)
    {
        luaL_newmetatable(L, "skid_conn_sig");
        lua_pushcfunction(L, conn_index, "connection_index");
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, conn_newindex, "connection_newindex");
        lua_setfield(L, -2, "__newindex");
        lua_pushstring(L, "Connection");
        lua_setfield(L, -2, "__type");
        lua_pop(L, 1);

        lua_pushcfunction(L, fireproximityprompt, "fireproximityprompt");
        lua_setglobal(L, "fireproximityprompt");

        lua_pushcfunction(L, fireclickdetector, "fireclickdetector");
        lua_setglobal(L, "fireclickdetector");

        lua_pushcfunction(L, getconnections, "getconnections");
        lua_setglobal(L, "getconnections");
        lua_pushcfunction(L, getconnections, "get_signal_cons");
        lua_setglobal(L, "get_signal_cons");

        lua_pushcfunction(L, getconnection, "getconnection");
        lua_setglobal(L, "getconnection");

        lua_pushcfunction(L, firesignal, "firesignal");
        lua_setglobal(L, "firesignal");

        lua_pushcfunction(L, setproximitypromptduration, "setproximitypromptduration");
        lua_setglobal(L, "setproximitypromptduration");

        lua_pushcfunction(L, getproximitypromptduration, "getproximitypromptduration");
        lua_setglobal(L, "getproximitypromptduration");
    }
}
