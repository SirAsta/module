#pragma once
#include "Offsets.h"
#include "lua.h"

namespace SkidBase
{
    inline lua_State* Decrypt_LuaState(uintptr_t context)
    {
        uintptr_t skid = context;
        uint32_t* aa = reinterpret_cast<uint32_t*>(skid);
        
        uint32_t a1 = aa[0] - static_cast<uint32_t>(skid);
        uint32_t a2 = aa[1] - static_cast<uint32_t>(skid);

        uint64_t skids = (static_cast<uint64_t>(a2) << 32) | a1;
        return reinterpret_cast<lua_State*>(skids);
    }

    inline auto GetLuaState = (lua_State * (__fastcall*)(uint64_t, uint64_t*, uint64_t*))Main::Functions::GetGlobalState;
    inline auto ExecuteVM = (void(__fastcall*)(lua_State*))Main::Functions::LuauExecute;

    inline auto GetCapabilities = (__int64(__fastcall*)(int*))Main::Identity2::GetCapabilities;
    inline auto GetIdentityStruct = reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(Main::Identity1::GetTlsPointer);
}
