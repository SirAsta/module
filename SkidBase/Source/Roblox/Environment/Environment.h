#pragma once
#include "Libs/Http/Http.h"
#include "Libs/Closures/Closures.h"
#include "Libs/Misc/Misc.h"
#include "Libs/Crypt/Crypt.h"
#include "Libs/Metatable/Metatable.h"
#include "Libs/Cache/Cache.h"
#include "Libs/Filesystem/Filesystem.h"
#include "Libs/Signals/Signals.h"
#include "lapi.h"
#include "lstate.h"
#include "lobject.h"
#include "lgc.h"
#include "../../Core/Execution/Execution.h"
#include <Windows.h>
#include <string>
#include <unordered_set>

inline bool isitourskidthread(lua_State* L) {
    return L != nullptr && L->userdata != nullptr && (L->userdata->capabilities & 0b1000000);
}

namespace env
{
    inline lua_CFunction original_index = nullptr;
    inline lua_CFunction original_namecall = nullptr;

    inline int index_hook(lua_State* L)
    {
        if (!isitourskidthread(L)) return original_index(L);

        if (lua_isstring(L, 2)) {
            const char* key = lua_tostring(L, 2);

            if (key && strcmp(key, "HttpGet") == 0)
            {
                lua_pushcfunction(L, http::HttpGet, "HttpGet");
                return 1;
            }
            if (key && strcmp(key, "HttpGetAsync") == 0)
            {
                lua_pushcfunction(L, http::HttpGet, "HttpGetAsync");
                return 1;
            }
        }
        
        if (original_index)
            return original_index(L);
        return 0;
    }

    inline int namecall_hook(lua_State* L)
    {
        if (!isitourskidthread(L)) return original_namecall(L);

        const char* key = L->namecall ? getstr(L->namecall) : nullptr;
        if (key) {
            if (strcmp(key, "HttpGet") == 0 || strcmp(key, "HttpGetAsync") == 0)
                return http::HttpGet(L);
        }
        
        if (original_namecall)
            return original_namecall(L);
        return 0;
    }

    inline void Register(lua_State* L)
    {
        luaL_sandboxthread(L);

        lua_pushcfunction(L, http::HttpGet, "HttpGet");
        lua_setglobal(L, "HttpGet");

        lua_pushcfunction(L, http::request, "request");
        lua_setglobal(L, "request");

        lua_pushcfunction(L, http::request, "http_request");
        lua_setglobal(L, "http_request");

        lua_newtable(L);
        lua_pushcfunction(L, http::request, "request");
        lua_setfield(L, -2, "request");
        lua_setglobal(L, "http");

        lua_newtable(L);
        lua_pushcfunction(L, http::request, "request");
        lua_setfield(L, -2, "request");
        lua_setglobal(L, "syn");

        lua_pushcfunction(L, closures::loadstring, "loadstring");
        lua_setglobal(L, "loadstring");

        lua_pushcfunction(L, closures::checkcaller, "checkcaller");
        lua_setglobal(L, "checkcaller");

        lua_pushcfunction(L, closures::iscclosure, "iscclosure");
        lua_setglobal(L, "iscclosure");

        lua_pushcfunction(L, closures::islclosure, "islclosure");
        lua_setglobal(L, "islclosure");

        lua_pushcfunction(L, closures::clonefunction, "clonefunction");
        lua_setglobal(L, "clonefunction");

        lua_pushcfunction(L, closures::identifyexecutor, "identifyexecutor");
        lua_setglobal(L, "identifyexecutor");

        lua_pushcfunction(L, closures::isexecutorclosure, "isexecutorclosure");
        lua_setglobal(L, "isexecutorclosure");
        lua_pushcfunction(L, closures::isexecutorclosure, "checkclosure");
        lua_setglobal(L, "checkclosure");

        lua_pushcfunction(L, closures::getcallingscript, "getcallingscript");
        lua_setglobal(L, "getcallingscript");

        lua_pushcfunction(L, misc::getexecutorname, "identifyexecutor");
        lua_setglobal(L, "identifyexecutor");

        lua_pushcfunction(L, misc::getexecutorname, "getexecutorname");
        lua_setglobal(L, "getexecutorname");

        lua_pushcfunction(L, misc::getgenv, "getgenv");
        lua_setglobal(L, "getgenv");

        lua_pushcfunction(L, misc::getreg, "getreg");
        lua_setglobal(L, "getreg");

        lua_pushcfunction(L, misc::getrenv, "getrenv");
        lua_setglobal(L, "getrenv");

        lua_pushcfunction(L, misc::getthreadidentity, "getthreadidentity");
        lua_setglobal(L, "getthreadidentity");
        lua_pushcfunction(L, misc::getthreadidentity, "getidentity");
        lua_setglobal(L, "getidentity");
        lua_pushcfunction(L, misc::getthreadidentity, "getthreadcontext");
        lua_setglobal(L, "getthreadcontext");

        lua_pushcfunction(L, misc::setthreadidentity, "setthreadidentity");
        lua_setglobal(L, "setthreadidentity");
        lua_pushcfunction(L, misc::setthreadidentity, "setidentity");
        lua_setglobal(L, "setidentity");
        lua_pushcfunction(L, misc::setthreadidentity, "setthreadcontext");
        lua_setglobal(L, "setthreadcontext");

        lua_pushcfunction(L, misc::gethui, "gethui");
        lua_setglobal(L, "gethui");

        lua_pushcfunction(L, misc::compareinstances, "compareinstances");
        lua_setglobal(L, "compareinstances");

        lua_pushcfunction(L, misc::setclipboard, "setclipboard");
        lua_setglobal(L, "setclipboard");
        lua_pushcfunction(L, misc::setclipboard, "toclipboard");
        lua_setglobal(L, "toclipboard");



        lua_pushcfunction(L, misc::getfpscap, "getfpscap");
        lua_setglobal(L, "getfpscap");

        lua_pushcfunction(L, misc::setfpscap, "setfpscap");
        lua_setglobal(L, "setfpscap");
        lua_newtable(L);
        lua_pushcfunction(L, cache_lib::invalidate, "invalidate");
        lua_setfield(L, -2, "invalidate");
        lua_pushcfunction(L, cache_lib::iscached, "iscached");
        lua_setfield(L, -2, "iscached");
        lua_pushcfunction(L, cache_lib::replace, "replace");
        lua_setfield(L, -2, "replace");
        lua_setglobal(L, "cache");

        lua_pushcfunction(L, cache_lib::cloneref, "cloneref");
        lua_setglobal(L, "cloneref");
        lua_pushcfunction(L, cache_lib::cloneref, "clonereference");
        lua_setglobal(L, "clonereference");
        lua_pushcfunction(L, fs_lib::readfile, "readfile");
        lua_setglobal(L, "readfile");

        lua_pushcfunction(L, fs_lib::writefile, "writefile");
        lua_setglobal(L, "writefile");

        lua_pushcfunction(L, fs_lib::appendfile, "appendfile");
        lua_setglobal(L, "appendfile");

        lua_pushcfunction(L, fs_lib::isfile, "isfile");
        lua_setglobal(L, "isfile");

        lua_pushcfunction(L, fs_lib::isfolder, "isfolder");
        lua_setglobal(L, "isfolder");

        lua_pushcfunction(L, fs_lib::makefolder, "makefolder");
        lua_setglobal(L, "makefolder");

        lua_pushcfunction(L, fs_lib::listfiles, "listfiles");
        lua_setglobal(L, "listfiles");

        lua_pushcfunction(L, fs_lib::delfile, "delfile");
        lua_setglobal(L, "delfile");

        lua_pushcfunction(L, fs_lib::delfolder, "delfolder");
        lua_setglobal(L, "delfolder");

        lua_pushcfunction(L, fs_lib::loadfile, "loadfile");
        lua_setglobal(L, "loadfile");

        lua_pushcfunction(L, fs_lib::dofile, "dofile");
        lua_setglobal(L, "dofile");

        lua_pushcfunction(L, metatable::getrawmetatable, "getrawmetatable");
        lua_setglobal(L, "getrawmetatable");

        lua_pushcfunction(L, metatable::setrawmetatable, "setrawmetatable");
        lua_setglobal(L, "setrawmetatable");

        lua_pushcfunction(L, metatable::setreadonly, "setreadonly");
        lua_setglobal(L, "setreadonly");

        lua_pushcfunction(L, metatable::isreadonly, "isreadonly");
        lua_setglobal(L, "isreadonly");

        lua_pushcfunction(L, metatable::getnamecallmethod, "getnamecallmethod");
        lua_setglobal(L, "getnamecallmethod");

        lua_pushcfunction(L, metatable::hookmetamethod, "hookmetamethod");
        lua_setglobal(L, "hookmetamethod");

        lua_newtable(L);
        lua_pushcfunction(L, crypt_lib::base64encode, "base64encode");
        lua_setfield(L, -2, "base64encode");
        lua_pushcfunction(L, crypt_lib::base64_encode, "base64_encode");
        lua_setfield(L, -2, "base64_encode");
        lua_pushcfunction(L, crypt_lib::base64decode, "base64decode");
        lua_setfield(L, -2, "base64decode");
        lua_pushcfunction(L, crypt_lib::base64_decode, "base64_decode");
        lua_setfield(L, -2, "base64_decode");
        lua_pushcfunction(L, crypt_lib::lz4compress, "lz4compress");
        lua_setfield(L, -2, "lz4compress");
        lua_pushcfunction(L, crypt_lib::lz4decompress, "lz4decompress");
        lua_setfield(L, -2, "lz4decompress");
        lua_pushcfunction(L, crypt_lib::generatebytes, "generatebytes");
        lua_setfield(L, -2, "generatebytes");
        lua_pushcfunction(L, crypt_lib::generatekey, "generatekey");
        lua_setfield(L, -2, "generatekey");
        lua_pushcfunction(L, crypt_lib::hash, "hash");
        lua_setfield(L, -2, "hash");

        lua_newtable(L);
        lua_pushcfunction(L, crypt_lib::base64encode, "encode");
        lua_setfield(L, -2, "encode");
        lua_pushcfunction(L, crypt_lib::base64decode, "decode");
        lua_setfield(L, -2, "decode");
        lua_setfield(L, -2, "base64");

        lua_newtable(L);
        lua_pushcfunction(L, crypt_lib::lz4compress, "compress");
        lua_setfield(L, -2, "compress");
        lua_pushcfunction(L, crypt_lib::lz4decompress, "decompress");
        lua_setfield(L, -2, "decompress");
        lua_setfield(L, -2, "lz4");

        lua_newtable(L);
        lua_pushcfunction(L, crypt_lib::hash, "hash");
        lua_setfield(L, -2, "hash");
        lua_pushcfunction(L, crypt_lib::getfunctionhash, "getfunctionhash");
        lua_setfield(L, -2, "getfunctionhash");
        lua_setfield(L, -2, "custom");

        lua_pushcfunction(L, crypt_lib::getfunctionhash, "getfunctionhash");
        lua_setfield(L, -2, "getfunctionhash");

        lua_setglobal(L, "crypt");

        lua_pushcfunction(L, crypt_lib::base64encode, "base64encode");
        lua_setglobal(L, "base64encode");
        lua_pushcfunction(L, crypt_lib::base64_encode, "base64_encode");
        lua_setglobal(L, "base64_encode");
        lua_pushcfunction(L, crypt_lib::base64decode, "base64decode");
        lua_setglobal(L, "base64decode");
        lua_pushcfunction(L, crypt_lib::base64_decode, "base64_decode");
        lua_setglobal(L, "base64_decode");
        lua_pushcfunction(L, crypt_lib::lz4compress, "lz4compress");
        lua_setglobal(L, "lz4compress");
        lua_pushcfunction(L, crypt_lib::lz4decompress, "lz4decompress");
        lua_setglobal(L, "lz4decompress");
        lua_pushcfunction(L, crypt_lib::getfunctionhash, "getfunctionhash");
        lua_setglobal(L, "getfunctionhash");
        lua_pushcfunction(L, crypt_lib::generatebytes, "generatebytes");
        lua_setglobal(L, "generatebytes");
        lua_pushcfunction(L, crypt_lib::generatekey, "generatekey");
        lua_setglobal(L, "generatekey");
        lua_pushcfunction(L, crypt_lib::hash, "hash");
        lua_setglobal(L, "hash");

        signals_lib::Register(L);

        int top = lua_gettop(L);
        lua_getglobal(L, "game");
        
        if (lua_istable(L, -1) || lua_isuserdata(L, -1))
        {
            if (luaL_getmetafield(L, -1, "__index"))
            {
                if (lua_type(L, -1) == LUA_TFUNCTION)
                {
                    Closure* cl = clvalue(const_cast<TValue*>(luaA_toobject(L, -1)));
                    original_index = cl->c.f;
                    cl->c.f = index_hook;
                }
                lua_pop(L, 1);
            }
            
            if (luaL_getmetafield(L, -1, "__namecall"))
            {
                if (lua_type(L, -1) == LUA_TFUNCTION)
                {
                    Closure* cl = clvalue(const_cast<TValue*>(luaA_toobject(L, -1)));
                    original_namecall = cl->c.f;
                    cl->c.f = namecall_hook;
                }
                lua_pop(L, 1);
            }
        }
        
        lua_settop(L, top);

        Execution::execute(L, "pcall(function() loadstring(game:HttpGet('https://raw.githubusercontent.com/RavageDevs/Extra-Libraries/refs/heads/main/Drawing.luau'))() end)"); // pcall for idk safety ig
    }
}
