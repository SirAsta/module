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

    inline auto Print = reinterpret_cast<uintptr_t(*)(int, const char*, ...)>(Main::Functions::Print);
    inline auto PushInstance = reinterpret_cast<uintptr_t*(__fastcall*)(lua_State*, void*)>(Main::Functions::PushInstance);
    inline auto GetLuaStateForInstance = reinterpret_cast<lua_State*(__fastcall*)(uint64_t, uint64_t*, uint64_t*)>(Main::Functions::GetScriptVMState);
    inline auto LuaVM_Load = reinterpret_cast<int(__fastcall*)(lua_State*, void*, const char*, int)>(Main::Functions::LuaVMLoad);
    inline auto FireTouchInterest = reinterpret_cast<void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, bool, bool)>(Main::Functions::FireTouchInterest);
    inline auto FireProximityPrompt = reinterpret_cast<uintptr_t*(__thiscall*)(uintptr_t)>(Main::Functions::FireProximityPrompt);
    inline auto ScriptContextResume = reinterpret_cast<uint64_t(__fastcall*)(uint64_t, void*, void*, uint32_t, uint8_t, uint64_t)>(Main::Functions::ScriptContextResume);
}

namespace Offsets {
    inline const uintptr_t Print = Main::Functions::Print;
    inline const uintptr_t PushInstance = Main::Functions::PushInstance;
    inline const uintptr_t GetLuaStateForInstance = Main::Functions::GetScriptVMState;
    inline const uintptr_t ScriptContextResume = Main::Functions::ScriptContextResume;
    inline const uintptr_t FireTouchInterest = Main::Functions::FireTouchInterest;
    inline const uintptr_t FireProximityPrompt = Main::Functions::FireProximityPrompt;
    inline const uintptr_t CastArgs = Main::Functions::CastArgs;
    inline const uintptr_t OpcodeLookupTable = Main::Functions::OpcodeLookupTable;
    inline const uintptr_t TaskDefer = Main::Functions::TaskDefer;
    inline const uintptr_t ConnectionDisconnect = Main::Functions::ConnectionDisconnect;
    inline const uintptr_t FakeDataModelPointer = Main::Miscellaneous::FakeDatamodelPOINTER;
    inline const uintptr_t TaskScheduler = Main::Scheduler::TaskScheduler;

    namespace Luau {
        inline const uintptr_t Luau_Execute = Main::Functions::LuauExecute;
        inline const uintptr_t luaD_throw = Main::Functions::LuaDThrow;
        inline const uintptr_t LuaVM_Load = Main::Functions::LuaVMLoad;
        inline const uintptr_t LuaO_NilObject = Main::Miscellaneous::LuaNil;
        inline const uintptr_t LuaH_DummyNode = Main::Miscellaneous::LuaDummy;
        inline const uintptr_t LuaC_Step = Main::Functions::LuaCStep;
    }

    namespace Reflections {
        inline const uintptr_t CastArgs = Main::Functions::CastArgs;
    }

    namespace Signals {
        inline const uintptr_t FireTouchInterest = Main::Functions::FireTouchInterest;
        inline const uintptr_t DisconnectConnect = Main::Functions::ConnectionDisconnect;
    }

    namespace Flags {
        inline const uintptr_t FireLeftMouseClick = Main::Functions::FireLeftMouseClick;
        inline const uintptr_t FireRightMouseClick = Main::Functions::FireRightMouseClick;
        inline const uintptr_t FireMouseHoverEnter = Main::Functions::FireMouseHoverEnter;
        inline const uintptr_t FireMouseHoverLeave = Main::Functions::FireMouseHoverLeave;
        inline const uintptr_t FireProximityPrompt = Main::Functions::FireProximityPrompt;
    }

    namespace Extras {
        inline const uintptr_t k_table = Main::Functions::KTable;
    }

    namespace ExtraSpace {
        inline const uintptr_t RequireBypass = Main::ExtraSpace::RequireBypass;
        inline const uintptr_t ScriptContextToResume = Main::ExtraSpace::ToResume;
    }

    namespace InstanceBridge {
        inline const uintptr_t Push = Main::Functions::PushInstance;
    }

    namespace ClassDescriptor {
        inline const uintptr_t GetPropertyData = Main::Functions::GetProperty;
        inline const uintptr_t GetProperty = Main::Functions::GetProperty;
        inline const uintptr_t Name = Main::ClassDescriptor::Name;
        inline const uintptr_t BaseClass = Main::ClassDescriptor::BaseClass;
    }

    namespace RobloxThread {
        inline const uintptr_t IdentityPtr = Main::Identity1::IdentityPointer;
        inline const uintptr_t GetTlsPointer = Main::Identity1::GetTlsPointer;
    }

    namespace Capabilities {
        inline const uintptr_t GetCapabilities = Main::Identity2::GetCapabilities;
        inline const uintptr_t Capabilities = Main::Identity2::Capabilities;
    }

    namespace Bytecode {
        inline const uintptr_t Pointer = Main::Bytecode::Pointer;
        inline const uintptr_t Size = Main::Bytecode::Size;
        inline const uintptr_t LocalScriptByteCode = Main::Bytecode::LocalScriptByteCode;
        inline const uintptr_t ModuleScriptByteCode = Main::Bytecode::ModuleScriptByteCode;
        inline const uintptr_t OpcodeMultiplier = Main::Bytecode::OpcodeMultiplier;
    }

    namespace DataModel {
        inline const uintptr_t FakeDataModel = Main::Miscellaneous::FakeDatamodelPOINTER;
        inline const uintptr_t FakeDataModelToDataModel = Main::DataModel::FakeDataModelToDataModel;
        inline const uintptr_t DataModel = Main::Offsets::DataModel;
        inline const uintptr_t PlaceId = Main::DataModel::PlaceId;
        inline const uintptr_t GameId = Main::DataModel::GameId;
        inline const uintptr_t JobId = Main::DataModel::JobId;
        inline const uintptr_t Children = Main::DataModel::Children;
        inline const uintptr_t ScriptContext = Main::DataModel::ScriptContext;
        inline const uintptr_t GameLoaded = Main::DataModel::GameLoaded;
        inline const uintptr_t InParallelPhase = Main::DataModel::InParallelPhase;
    }

    namespace Scheduler {
        inline const uintptr_t TaskScheduler = Main::Scheduler::TaskScheduler;
        inline const uintptr_t JobsStart = Main::Scheduler::JobsStart;
        inline const uintptr_t JobsEnd = Main::Scheduler::JobsEnd;
        inline const uintptr_t JobTypeName = Main::Scheduler::JobTypeName;
        inline const uintptr_t Fps = Main::Scheduler::Fps;
    }

    namespace SchedulerJob {
        inline const uintptr_t JobName = Main::Scheduler::JobName;
        inline const uintptr_t JobTypeName = Main::Scheduler::JobTypeName;
    }

    namespace FFlags {
        inline const uintptr_t GetFFlag = Main::Functions::GetFFlag;
        inline const uintptr_t SetFFlag = Main::Functions::SetFFlag;
        inline const uintptr_t FFlagPointer = Main::Miscellaneous::FFlagPointer;
        inline const uintptr_t BooleanType = Main::Miscellaneous::BooleanType;
        inline const uintptr_t BooleanValueType = Main::Miscellaneous::BooleanValueType;
        inline const uintptr_t IntegerType = Main::Miscellaneous::IntegerType;
        inline const uintptr_t IntegerValueType = Main::Miscellaneous::IntegerValueType;
    }

    namespace Instances {
        inline const uintptr_t Self = Main::Instance::Self;
        inline const uintptr_t Class = Main::Instance::Class;
        inline const uintptr_t Parent = Main::Instance::Parent;
        inline const uintptr_t Name = Main::Instance::Name;
        inline const uintptr_t Children = Main::Instance::Children;
    }

    namespace BasePart {
        inline const uintptr_t Primitive = Main::BasePart::Primitive;
        inline const uintptr_t Overlap = Main::Primitive::Overlap;
    }

    namespace Property {
        inline const uintptr_t typeNum = Main::TypeInfo::TypeNum;
        inline const uintptr_t typeInfo = Main::Property::TypeInfo;
        inline const uintptr_t isScriptable = Main::Property::IsScriptable;
        inline const uintptr_t getSet = Main::Property::GetSet;
        inline const uintptr_t CDesc2PDesc = Main::Property::MemberMap::Base;
        inline const uintptr_t Name = Main::Property::Name;
    }

    namespace Tls {
        inline const uintptr_t Identity = Main::Tls::Identity;
        inline const uintptr_t Caps = Main::Tls::Caps;
        inline const uintptr_t CapabilityResolver = Main::Tls::CapabilityResolver;
    }

    namespace RunService {
        inline const uintptr_t RenderStepBindings = Main::RunService::RenderStepBindings;
    }
}
