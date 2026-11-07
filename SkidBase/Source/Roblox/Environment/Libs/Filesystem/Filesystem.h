#pragma once
#include "lua.h"
#include "lualib.h"
#include "lobject.h"
#include "lstate.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <Windows.h>
#include "../../../../Core/Execution/Execution.h"

namespace fs_lib
{
    namespace fs = std::filesystem;

    static const char* BLOCKED_EXTS[] = {
        ".exe", ".dll", ".bat", ".cmd", ".vbs", ".ps1", ".msi", ".scr",
        ".com", ".pif", ".sh", ".cpl", ".msc", ".js", ".jse", ".wsf",
        ".wsh", ".hta", ".scf", ".lnk", ".zip", ".rar", ".7z", ".cab",
        ".iso", ".img", ".xml", ".msp", ".reg", ".py", ".pyc", ".pyw",
        ".rb", ".pl", ".php", ".jar", ".sys", ".drv", ".ocx", ".psd1",
        ".vbe", ".inf", ".url", ".ts", ".tar", ".gz", ".xz", ".bz2",
        ".tgz", ".wim", ".docm", ".pptm", nullptr
    };

    inline bool is_blocked_extension(const fs::path& p)
    {
        std::string ext = p.extension().string();
        for (char& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        for (int i = 0; BLOCKED_EXTS[i]; i++)
        {
            if (ext == BLOCKED_EXTS[i])
                return true;
        }
        return false;
    }

    inline fs::path get_workspace_root()
    {
        char path[MAX_PATH];
        GetModuleFileNameA(GetModuleHandleA("Module.dll"), path, MAX_PATH);
        fs::path root = fs::path(path).parent_path() / "workspace";
        std::error_code ec;
        fs::create_directories(root, ec);
        return root;
    }

    inline bool resolve_safe_path(const std::string& relPath, fs::path& out)
    {
        fs::path root = get_workspace_root();
        std::error_code ec;
        fs::path cleanRel = fs::path(relPath).lexically_normal();
        
        fs::path target = (root / cleanRel).lexically_normal();

        std::string rootStr = root.string();
        std::string targetStr = target.string();

        if (targetStr.rfind(rootStr, 0) != 0)
            return false;

        out = target;
        return true;
    }

    inline int readfile(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string p = lua_tostring(L, 1);

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            luaL_error(L, "file is outside workspace");
            return 0;
        }

        if (!fs::is_regular_file(target))
        {
            luaL_error(L, "file does not exist");
            return 0;
        }

        std::ifstream file(target, std::ios::binary);
        if (!file.is_open())
        {
            luaL_error(L, "failed to open file");
            return 0;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        lua_pushlstring(L, content.data(), content.size());
        return 1;
    }

    inline int writefile(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);

        std::string p = lua_tostring(L, 1);
        size_t len = 0;
        const char* data = lua_tolstring(L, 2, &len);

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            luaL_error(L, "file is outside workspace");
            return 0;
        }

        if (is_blocked_extension(target))
        {
            luaL_error(L, "disallowed file extension");
            return 0;
        }

        std::error_code ec;
        if (target.has_parent_path())
            fs::create_directories(target.parent_path(), ec);

        std::ofstream file(target, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            luaL_error(L, "failed to write file");
            return 0;
        }

        file.write(data, len);
        return 0;
    }

    inline int appendfile(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);

        std::string p = lua_tostring(L, 1);
        size_t len = 0;
        const char* data = lua_tolstring(L, 2, &len);

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            luaL_error(L, "file is outside workspace");
            return 0;
        }

        if (is_blocked_extension(target))
        {
            luaL_error(L, "disallowed file extension");
            return 0;
        }

        if (!fs::is_regular_file(target))
        {
            luaL_error(L, "file does not exist");
            return 0;
        }

        std::ofstream file(target, std::ios::binary | std::ios::app);
        if (!file.is_open())
        {
            luaL_error(L, "failed to append file");
            return 0;
        }

        file.write(data, len);
        return 0;
    }

    inline int isfile(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string p = lua_tostring(L, 1);

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            lua_pushboolean(L, false);
            return 1;
        }

        lua_pushboolean(L, fs::is_regular_file(target));
        return 1;
    }

    inline int isfolder(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string p = lua_tostring(L, 1);

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            lua_pushboolean(L, false);
            return 1;
        }

        lua_pushboolean(L, fs::is_directory(target));
        return 1;
    }

    inline int makefolder(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string p = lua_tostring(L, 1);

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            luaL_error(L, "folder is outside workspace");
            return 0;
        }

        std::error_code ec;
        fs::create_directories(target, ec);
        return 0;
    }

    inline int listfiles(lua_State* L)
    {
        std::string p = (lua_gettop(L) >= 1 && lua_isstring(L, 1)) ? lua_tostring(L, 1) : "";

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            luaL_error(L, "folder is outside workspace");
            return 0;
        }

        if (!fs::is_directory(target))
        {
            luaL_error(L, "folder does not exist");
            return 0;
        }

        fs::path root = get_workspace_root();
        std::string rootStr = root.string();

        lua_newtable(L);
        int idx = 1;

        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(target, ec))
        {
            std::string fullPath = entry.path().string();
            std::string rel = (fullPath.rfind(rootStr, 0) == 0 && fullPath.size() > rootStr.size()) 
                ? fullPath.substr(rootStr.size() + 1) 
                : fullPath;
            std::replace(rel.begin(), rel.end(), '\\', '/');

            lua_pushstring(L, rel.c_str());
            lua_rawseti(L, -2, idx++);
        }

        return 1;
    }

    inline int delfile(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string p = lua_tostring(L, 1);

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            luaL_error(L, "file is outside workspace");
            return 0;
        }

        if (!fs::is_regular_file(target))
        {
            luaL_error(L, "file does not exist");
            return 0;
        }

        std::error_code ec;
        fs::remove(target, ec);
        return 0;
    }

    inline int delfolder(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string p = lua_tostring(L, 1);

        fs::path target;
        if (!resolve_safe_path(p, target) || target == get_workspace_root())
        {
            luaL_error(L, "cannot delete workspace root");
            return 0;
        }

        if (!fs::is_directory(target))
        {
            luaL_error(L, "folder does not exist");
            return 0;
        }

        std::error_code ec;
        fs::remove_all(target, ec);
        return 0;
    }

    inline int loadfile(lua_State* L)
    {
        luaL_checktype(L, 1, LUA_TSTRING);
        std::string p = lua_tostring(L, 1);
        const char* chunkname = luaL_optstring(L, 2, "loadfile");

        fs::path target;
        if (!resolve_safe_path(p, target))
        {
            lua_pushnil(L);
            lua_pushstring(L, "file is outside workspace");
            return 2;
        }

        if (!fs::is_regular_file(target))
        {
            lua_pushnil(L);
            lua_pushstring(L, "file does not exist");
            return 2;
        }

        std::ifstream file(target, std::ios::binary);
        if (!file.is_open())
        {
            lua_pushnil(L);
            lua_pushstring(L, "failed to open file");
            return 2;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string src = buffer.str();

        std::string bc = Execution::aexecute(src);
        if (luau_load(L, chunkname, bc.c_str(), bc.size(), 0) != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            std::string msg = err ? err : "compile error";
            lua_pop(L, 1);
            lua_pushnil(L);
            lua_pushstring(L, msg.c_str());
            return 2;
        }

        Closure* fn = clvalue(const_cast<TValue*>(luaA_toobject(L, -1)));
        if (fn && fn->l.p)
            Execution::setprotocapabilities(fn->l.p, const_cast<uintptr_t*>(&Execution::caps));

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
        return 1;
    }

    inline int dofile(lua_State* L)
    {
        if (loadfile(L) != 1)
            return 2;

        lua_call(L, 0, 0);
        return 0;
    }
}
