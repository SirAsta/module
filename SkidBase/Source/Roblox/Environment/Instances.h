#pragma once
#include "lua.h"
#include <cstdint>

namespace laustate 
{
    lua_State* GetLuasState(uintptr_t scriptContext);
}
