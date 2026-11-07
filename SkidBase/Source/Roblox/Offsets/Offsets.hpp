#pragma once
#include "Offsets.h"
#include <stdexcept>

struct lua_State;

namespace Roblox {
    using LuaD_ThrowT = void(__fastcall*)(lua_State*, int);
    inline LuaD_ThrowT LuaD_Throw = reinterpret_cast<LuaD_ThrowT>(Main::Functions::LuaDThrow);

    inline void Luau_Execute(lua_State* L) {
        using Luau_Execute_t = void(__fastcall*)(lua_State*);
        reinterpret_cast<Luau_Execute_t>(Main::Functions::LuauExecute)(L);
    }
}

namespace Offsets {
    inline const uintptr_t OpcodeLookupTable = Main::Functions::OpcodeLookupTable;
    namespace Luau {
        inline const uintptr_t LuaO_NilObject = Main::Miscellaneous::LuaNil;
        inline const uintptr_t LuaH_DummyNode = Main::Miscellaneous::LuaDummy;
    }
}
