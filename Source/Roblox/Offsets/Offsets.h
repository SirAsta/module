#pragma once
#include <cstdint>
#include <Windows.h>

namespace Main {
    inline uintptr_t krah() {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    }

    inline uintptr_t Rebase(uintptr_t offset) {
        return krah() + offset;
    }

    namespace Functions {
        inline const uintptr_t Print = Rebase(0x1ca9a30);
        inline const uintptr_t GetGlobalState = Rebase(0x4274c30);
        inline const uintptr_t LuauExecute = Rebase(0x26789e0);
        inline const uintptr_t LuaDThrow = Rebase(0x26360e0);
        inline const uintptr_t OpcodeLookupTable = Rebase(0x735b280);
        inline const uintptr_t PushInstance = Rebase(0x42594c0);
    }

    namespace Miscellaneous {
        inline const uintptr_t FakeDatamodelPOINTER = Rebase(0x8e4d7b8);
        inline const uintptr_t LuaNil = Rebase(0x68ba8e8);
        inline const uintptr_t LuaDummy = Rebase(0x68b9f58);
        inline const uintptr_t TargetFPS = Rebase(0x81715b0);
    }

    namespace Offsets {
        inline const uintptr_t DataModel = 0x1F8;
        inline const uintptr_t ScriptContext = 0x440;
        inline const uintptr_t Children = 0x70;
        inline const uintptr_t GameLoaded = 0x638;
    }

    namespace Scheduler {
        inline const uintptr_t TaskScheduler = Rebase(0x8C9D698);
        inline const uintptr_t JobStart = 0xC8;
        inline const uintptr_t JobEnd = 0xD0;
        inline const uintptr_t JobTypeName = 0x18;
    }

    namespace ExtraSpace {
        inline const uintptr_t RequireBypass = 0xB50;
    }

    namespace Identity1 {
        inline const uintptr_t IdentityPointer = Rebase(0x83B6678);
        inline const uintptr_t GetTlsPointer = Rebase(0xC700);
    }

    namespace Identity2 {
        inline const uintptr_t GetCapabilities = Rebase(0x1C905D0);
        inline const uintptr_t Capabilities = 0x30;
    }

    namespace SignalOffsets {
        inline const uintptr_t FireMouseClick = Rebase(0x3C00500);
        inline const uintptr_t FireRightMouseClick = Rebase(0x3C006A0);
        inline const uintptr_t FireMouseHoverEnter = Rebase(0x3C01A70);
        inline const uintptr_t FireMouseHoverLeave = Rebase(0x3C01C10);
        inline const uintptr_t FireProximityPrompt = Rebase(0x30A8980);
    }
}