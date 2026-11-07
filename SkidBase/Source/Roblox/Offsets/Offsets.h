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
        inline const uintptr_t Print = Rebase(0x1CA9A30);
        inline const uintptr_t GetScriptVMState = Rebase(0x4274C30);
        inline const uintptr_t GetGlobalState = GetScriptVMState;
        inline const uintptr_t LuaVMLoad = Rebase(0x426A570);
        inline const uintptr_t LuauExecute = Rebase(0x277A970);
        inline const uintptr_t LuaDThrow = Rebase(0x2750300);
        inline const uintptr_t OpcodeLookupTable = Rebase(0x735B280);
        inline const uintptr_t OpcodeDecodeTable = Rebase(0x735B380);
        inline const uintptr_t PushInstance = Rebase(0x42594C0);
        inline const uintptr_t ScriptContextResume = Rebase(0x431B1F0);
        inline const uintptr_t CastArgs = Rebase(0x41CB820);
        inline const uintptr_t ConnectionDisconnect = Rebase(0x4239210);
        inline const uintptr_t GetCapabilities = Rebase(0x1CEA140);
        inline const uintptr_t Impersonator = Rebase(0x1CEA200);
        inline const uintptr_t GetProperty = Rebase(0x1CE8360);
        inline const uintptr_t KTable = Rebase(0x8530CF0);
        inline const uintptr_t NameManager = Rebase(0x858AC58);
        inline const uintptr_t InternPropertyName = Rebase(0x2A7BA40);
        inline const uintptr_t LookupPropertyName = Rebase(0x2A7BE50);
        inline const uintptr_t GetTlsPointer = Rebase(0xC700);
        inline const uintptr_t TaskSynchronize = Rebase(0x435B180);
        inline const uintptr_t TaskDesynchronize = Rebase(0x435B700);
        inline const uintptr_t TaskDefer = Rebase(0x435C440);
        inline const uintptr_t TaskSpawn = Rebase(0x435C920);
        inline const uintptr_t TaskDelay = Rebase(0x435CCB0);
        inline const uintptr_t TaskWait = Rebase(0x435CFB0);
        inline const uintptr_t TaskCancel = Rebase(0x435D290);
        inline const uintptr_t FireTouchInterest = Rebase(0x14B6AE0);
        inline const uintptr_t FireProximityPrompt = Rebase(0x30FD840);
        inline const uintptr_t FireLeftMouseClick = Rebase(0x3C6DAF0);
        inline const uintptr_t FireRightMouseClick = Rebase(0x3C6DC90);
        inline const uintptr_t FireMouseHoverEnter = Rebase(0x3C6F060);
        inline const uintptr_t FireMouseHoverLeave = Rebase(0x3C6F200);
        inline const uintptr_t IsLegalSendEvent = Rebase(0x47FBFE0);
        inline const uintptr_t LuaCStep = Rebase(0x265D640);
        inline const uintptr_t GcStep = Rebase(0x265D2F0);
        inline const uintptr_t Freeobj = Rebase(0x265BD90);
        inline const uintptr_t LuauLoad = Rebase(0x265F9D0);
        inline const uintptr_t LuaTEventnames = Rebase(0x68DC080);
        inline const uintptr_t LuaTTypenames = Rebase(0x68DC010);
        inline const uintptr_t LuaFFreeproto = Rebase(0x2672800);
        inline const uintptr_t LuaFNewproto = Rebase(0x2671850);
        inline const uintptr_t LuaLArgerrorL = Rebase(0x263F620);
        inline const uintptr_t Currfuncname = Rebase(0x263F5A0);
        inline const uintptr_t LuaGRunerrorL = Rebase(0x2671020);
        inline const uintptr_t LuaOPushvfstring = Rebase(0x266EEC0);
        inline const uintptr_t LuaDRawrunprotected = Rebase(0x26360B0);
        inline const uintptr_t LuaAToobject = Rebase(0x262FCD0);
        inline const uintptr_t LuaPushvfstring = Rebase(0x26311D0);
        inline const uintptr_t Pseudo2addr = Rebase(0x262FC10);
        inline const uintptr_t LuaClock = Rebase(0x26A0F50);
        inline const uintptr_t LuaReadonlyerror = Rebase(0x26704B0);
        inline const uintptr_t LuaMethoderror = Rebase(0x2670470);
        inline const uintptr_t LuaIndexerror = Rebase(0x2670370);
        inline const uintptr_t LuaOrdererror = Rebase(0x26702F0);
        inline const uintptr_t LuaForerrorL = Rebase(0x26701C0);
        inline const uintptr_t Newpage = Rebase(0x269FDB0);
        inline const uintptr_t Newclasspage = Rebase(0x26A0640);
        inline const uintptr_t Freeclasspage = Rebase(0x269FBD0);
        inline const uintptr_t Freeblock = Rebase(0x269F700);
        inline const uintptr_t LuaMFree = Rebase(0x269F7C0);
        inline const uintptr_t LuaMFreegco = Rebase(0x269F870);
        inline const uintptr_t LuaMVisitgco = Rebase(0x269FA70);
        inline const uintptr_t LuaMToobig = Rebase(0x269F6D0);
        inline const uintptr_t ConnectionDisconnectRaw = Rebase(0x4AF09E0);
        inline const uintptr_t ConnectionUnlink = Rebase(0x4AF0E30);
        inline const uintptr_t ConnectionPushFront = Rebase(0x4AF11D0);
        inline const uintptr_t ConnectionConnected = Rebase(0x4AF0B70);
        inline const uintptr_t GetFFlag = Rebase(0x4B12B50);
        inline const uintptr_t SetFFlag = Rebase(0x4B12050);
        inline const uintptr_t LockViolationInstanceCrash = Rebase(0x81AACB8);
        inline const uintptr_t LockViolationScriptCrash = Rebase(0x8195AE0);
        inline const uintptr_t LuaStepIntervalMsOverrideEnabled = Rebase(0x8196468);
        inline const uintptr_t AsyncTrampoline = Rebase(0xB82730);
    }

    namespace Miscellaneous {
        inline const uintptr_t FakeDatamodelPOINTER = Rebase(0x8EB6960);
        inline const uintptr_t LuaNil = Rebase(0x6437A18);
        inline const uintptr_t LuaDummy = Rebase(0x6437098);
        inline const uintptr_t TargetFPS = Rebase(0x81715B0);
        inline const uintptr_t RawScheduler = Rebase(0x8E4D7B8);
        inline const uintptr_t TaskTable = Rebase(0x8D878E0);
        inline const uintptr_t FFlagPointer = Rebase(0x858F8E8);
        inline const uintptr_t AppdataInfo = Rebase(0x147B4B0);
        inline const uintptr_t SingleSurfaceApp = Rebase(0x8263530);
        inline const uintptr_t SingleSurfaceAppVft = Rebase(0x70A1EB0);
        inline const uintptr_t BooleanType = Rebase(0x71DE5A8);
        inline const uintptr_t BooleanValueType = Rebase(0x71DE7D8);
        inline const uintptr_t IntegerType = Rebase(0x71DE6C0);
        inline const uintptr_t IntegerValueType = Rebase(0x71DE8F0);
    }

    namespace Offsets {
        inline const uintptr_t DataModel = 0x1F8;
        inline const uintptr_t ScriptContext = 0x440;
        inline const uintptr_t Children = 0x70;
        inline const uintptr_t GameLoaded = 0x588;
    }

    namespace DataModel {
        inline const uintptr_t FakeDataModelToDataModel = 0x1F8;
        inline const uintptr_t Children = 0x70;
        inline const uintptr_t ChildrenEndOffset = 0x8;
        inline const uintptr_t ChildrenEntryStride = 0x10;
        inline const uintptr_t PlaceId = 0x180;
        inline const uintptr_t GameId = 0x178;
        inline const uintptr_t JobId = 0x108;
        inline const uintptr_t ScriptContext = 0x440;
        inline const uintptr_t GameLoaded = 0x588;
        inline const uintptr_t InParallelPhase = 0x53D;
    }

    namespace Scheduler {
        inline const uintptr_t TaskScheduler = Rebase(0x8E4D7B8);
        inline const uintptr_t JobStart = 0xC8;
        inline const uintptr_t JobEnd = 0xD0;
        inline const uintptr_t JobsStart = 0xC8;
        inline const uintptr_t JobsEnd = 0xD0;
        inline const uintptr_t JobName = 0x18;
        inline const uintptr_t JobTypeName = 0xF8;
        inline const uintptr_t ScriptContext = 0x1C0;
        inline const uintptr_t Fps = 0xB0;
        inline const uintptr_t StepVTSlot = 0x8;
        inline const uintptr_t JobVTableSlots = 0x9;
    }

    namespace ScriptContext {
        inline const uintptr_t RequireBypass = 0xB71;
        inline const uintptr_t ToResume = 0xA28;
        inline const uintptr_t DataModel = 0x100;
        inline const uintptr_t ExecutionContext = 0x100;
        inline const uintptr_t InParallelPhase = 0x53D;
    }

    namespace ExtraSpace {
        inline const uintptr_t RequireBypass = 0xB71;
        inline const uintptr_t ToResume = 0xA28;
        inline const uintptr_t Capabilities = 0x30;
        inline const uintptr_t CapabilityResolver = 0x38;
    }

    namespace Identity1 {
        inline const uintptr_t IdentityPointer = Rebase(0x855CF68);
        inline const uintptr_t GetTlsPointer = Rebase(0xC700);
    }

    namespace Identity2 {
        inline const uintptr_t GetCapabilities = Rebase(0x1CEA140);
        inline const uintptr_t Impersonator = Rebase(0x1CEA200);
        inline const uintptr_t Capabilities = 0x30;
        inline const uintptr_t CapabilityResolver = 0x38;
    }

    namespace Tls {
        inline const uintptr_t Identity = 0x0;
        inline const uintptr_t Caps = 0x30;
        inline const uintptr_t CapabilityResolver = 0x38;
    }

    namespace SignalOffsets {
        inline const uintptr_t FireMouseClick = Rebase(0x3C6DAF0);
        inline const uintptr_t FireRightMouseClick = Rebase(0x3C6DC90);
        inline const uintptr_t FireMouseHoverEnter = Rebase(0x3C6F060);
        inline const uintptr_t FireMouseHoverLeave = Rebase(0x3C6F200);
        inline const uintptr_t FireProximityPrompt = Rebase(0x30FD840);
        inline const uintptr_t DisconnectConnect = Rebase(0x4239210);
    }

    namespace Instance {
        inline const uintptr_t Self = 0x8;
        inline const uintptr_t SharedRefUseCount = 0x8;
        inline const uintptr_t SharedRefControl = 0x10;
        inline const uintptr_t Class = 0x18;
        inline const uintptr_t Parent = 0x60;
        inline const uintptr_t Name = 0x68;
        inline const uintptr_t Children = 0x70;
        inline const uintptr_t ClassDescriptor = 0x18;
    }

    namespace BasePart {
        inline const uintptr_t Primitive = 0x178;
    }

    namespace Primitive {
        inline const uintptr_t Overlap = 0x200;
    }

    namespace ClassDescriptor {
        inline const uintptr_t Name = 0x8;
        inline const uintptr_t BaseClass = 0x1B0;
        inline const uintptr_t IsADepth = 0x1B8;
        inline const uintptr_t IsARange = 0x1BA;
    }

    namespace TypeInfo {
        inline const uintptr_t Name = 0x8;
        inline const uintptr_t TypeNum = 0x30;
        inline const uintptr_t EnumItemFromVariantVftSlot = 0x20;
    }

    namespace EnumItem {
        inline const uintptr_t Name = 0x8;
        inline const uintptr_t Value = 0x30;
    }

    namespace ProtectedString {
        inline const uintptr_t Data = 0x10;
        inline const uintptr_t Size = 0x20;
    }

    namespace Property {
        inline const uintptr_t Name = 0x8;
        inline const uintptr_t ReadCapabilities = 0x38;
        inline const uintptr_t LockSlot = 0x44;
        inline const uintptr_t TypeInfo = 0x70;
        inline const uintptr_t InstanceOffset = 0x80;
        inline const uintptr_t IsScriptable = 0xAC;
        inline const uintptr_t FlagScriptable = 0x10;
        inline const uintptr_t GetSet = 0xB0;
        inline const uintptr_t GetSetRef = 0xB8;
        inline const uintptr_t GetSetVftGetter = 0x18;
        inline const uintptr_t GetSetVftSetter = 0x20;
        inline const uintptr_t GetterVftSlot = 0x18;
        inline const uintptr_t IsReadOnlyVftSlot = 0x8;
        inline const uintptr_t IsWriteOnlyVftSlot = 0x10;
        inline const uintptr_t ThreadSafety = 0x40;
        inline const uintptr_t GetVariantVftSlot = 0x80;
        inline const uintptr_t SetVariantVftSlot = 0x90;
        inline const uintptr_t SetEnumStringVftSlot = 0xC8;
        inline const uintptr_t RefDescriptorVftGetValue = 0x100;
        inline const uintptr_t SetEnumValueVftSlot = 0x108;
        inline const uintptr_t GetEnumItemVftSlot = 0x110;
        inline const uintptr_t SetEnumItemVftSlot = 0x118;

        namespace MemberMap {
            inline const uintptr_t Base = 0x1E0;
            inline const uintptr_t Capacity = 0x1E4;
            inline const uintptr_t Ctrl = 0x1E8;
            inline const uintptr_t Entries = 0x1F0;
            inline const uintptr_t Mask = 0x1F8;
            inline const uintptr_t Log2Capacity = 0x1FC;
            inline const uintptr_t EntryStride = 0x10;
            inline const uint32_t EmptyMeta = 0xFFFFFFFFu;
        }

        namespace MemberVariant {
            inline const uintptr_t TypeTag = 0x8;
            inline const uint32_t Tag_Callback = 4;
            inline const uint32_t Tag_Property = 0x0;
        }

        namespace Callback {
            inline const uintptr_t InstanceFieldOffset = 0x88;
            inline const uintptr_t Closure_BodyPtr = 0x10;
            inline const uintptr_t Body_LambdaPtr = 0x0;
            inline const uintptr_t Lambda_Vtable = 0x0;
            inline const uintptr_t Lambda_WeakObjRef = 0x10;
            inline const uintptr_t WeakObjRef_SelfRef = 0x0;
            inline const uintptr_t WeakObjRef_LuaState = 0x28;
            inline const uintptr_t WeakObjRef_RefId = 0x34;
        }
    }

    namespace MemberKind {
        inline const uint32_t Property = 0x0;
        inline const uint32_t Event = 0x1;
        inline const uint32_t Function = 0x2;
        inline const uint32_t Callback = 0x4;
    }

    namespace MemberMap {
        inline const uintptr_t Capacity = 0x1E4;
        inline const uintptr_t Mask = 0x1F8;
    }

    namespace CallbackDesc {
        inline const uintptr_t IsAsync = 0x80;
        inline const uintptr_t InstanceOffset = 0x88;
    }

    namespace CallbackChain {
        inline const uintptr_t HolderObject = 0x0;
        inline const uintptr_t LambdaLiveRef = 0x10;
        inline const uintptr_t LiveRefAlive = 0x10;
        inline const uintptr_t FunctionImpl = 0x18;
        inline const uintptr_t LiveRefThread = 0x28;
        inline const uintptr_t LiveRefValueRef = 0x34;
    }

    namespace RobloxProperty {
        inline const uintptr_t PropertyMapOffset = 0x1E0;
        inline const uintptr_t NameId = 0x20;
        inline const uint32_t MaxNameId = 0x4000;
    }

    namespace Flags {
        inline const uintptr_t LockViolationInstanceCrash = Rebase(0x81AACB8);
        inline const uintptr_t LockViolationScriptCrash = Rebase(0x8195AE0);
        inline const uintptr_t LuaStepIntervalMsOverrideEnabled = Rebase(0x8196468);
        inline const uintptr_t TaskSchedulerTargetFps = Rebase(0x81715B0);
        inline const uintptr_t GetFFlag = Rebase(0x4B12B50);
        inline const uintptr_t SetFFlag = Rebase(0x4B12050);
        inline const uintptr_t FFlagPointer = Rebase(0x858F8E8);
    }

    namespace Bytecode {
        inline const uintptr_t OpcodeMultiplier = 227;
        inline const uintptr_t LocalScriptByteCode = 0x178;
        inline const uintptr_t ModuleScriptByteCode = 0x120;
        inline const uintptr_t Pointer = 0x10;
        inline const uintptr_t Size = 0x20;
    }

    namespace Signals {
        inline const uintptr_t OwnerVtableRaiseSlot = 0x18;
        inline const uintptr_t EventInstanceOwner = 0x18;
        inline const uintptr_t EventDescSignature = 0x60;
        inline const uintptr_t EventDescSignatureEnd = 0x68;
        inline const uintptr_t SignatureFirstSlot = 0x8;
        inline const uintptr_t SignatureSlotStride = 0x70;
        inline const uintptr_t RbxTypeName = 0x8;
        inline const uintptr_t RbxTypeTypeId = 0x30;
        inline const uintptr_t ListHead = 0x8;

        namespace Connection {
            inline const uintptr_t Next = 0x10;
            inline const uintptr_t Enabled = 0x20;
            inline const uintptr_t SlotWrapper = 0x30;
            inline const uintptr_t SlotWrapperSelf = 0x38;
        }

        namespace SlotWrapper {
            inline const uintptr_t Thread = 0x28;
            inline const uintptr_t FunctionId = 0x30;
            inline const uintptr_t FunctionIdFallback = 0x34;
            inline const uintptr_t WeakObjectRef = 0xA0;
            inline const uintptr_t WeakObjectRefFallback = 0x90;
            inline const uintptr_t Ptr = 0x10;
            inline const uintptr_t WeakObjectPrimary = 0x68;
            inline const uintptr_t WeakObjectFallbackA = 0x38;
            inline const uintptr_t WeakObjectFallbackB = 0x48;
            inline const uintptr_t SelfOffsetInSelf = 0x18;
            inline const uintptr_t WaitContainerRef = 0x50;
            inline const uintptr_t ChainIndirect = 0x8;
        }

        namespace Slot {
            inline const uintptr_t Connection = 0x8;
            inline const uintptr_t NextConnection = 0x10;
            inline const uintptr_t Enabled = 0x20;
            inline const uintptr_t Disconnected = 0x20;
            inline const uintptr_t IsSubmitTask = 0xB4;
            inline const uintptr_t IsOnce = 0xB5;
            inline const uintptr_t OnceFired = 0xB6;
            inline const uintptr_t LuaThreadRefId = 0x30;
            inline const uintptr_t FunctionRefId = 0x34;
        }
    }

    namespace Connection {
        inline const uintptr_t Next = 0x10;
        inline const uintptr_t Enabled = 0x20;
        inline const uintptr_t SlotWrapper = 0x30;
    }

    namespace Slot_wrapper {
        inline const uintptr_t WeakObjectRef = 0x68;
        inline const uintptr_t Thread = 0x28;
    }

    namespace WeakObject {
        inline const uintptr_t Thread = 0x28;
        inline const uintptr_t FunctionRefId = 0x34;
        inline const uintptr_t FunctionRefIdFallback = 0x38;
    }

    namespace WaitContainer {
        inline const uintptr_t Thread = 0x28;
    }

    namespace RunService {
        inline const uintptr_t RenderStepBindings = 0x110;
        inline const uintptr_t MaxNameLength = 0x400;
        inline const uint32_t MaxRefId = 0x00FFFFFF;

        namespace BindingTree {
            inline const uintptr_t Left = 0x0;
            inline const uintptr_t Parent = 0x8;
            inline const uintptr_t Right = 0x10;
            inline const uintptr_t IsNil = 0x19;
            inline const uintptr_t Priority = 0x20;
            inline const uintptr_t VecFirst = 0x28;
            inline const uintptr_t VecLast = 0x30;
        }

        namespace Binding {
            inline const uintptr_t Stride = 0x50;
            inline const uintptr_t Name = 0x0;
            inline const uintptr_t NameSize = 0x10;
            inline const uintptr_t NameCaps = 0x18;
            inline const uintptr_t Callback = 0x20;
            inline const uintptr_t CallbackObject = 0x30;
        }

        namespace CallbackRef {
            inline const uintptr_t Owner = 0x10;
            inline const uintptr_t Thread = 0x28;
            inline const uintptr_t ThreadRefId = 0x30;
            inline const uintptr_t FunctionRefId = 0x34;
        }
    }

    namespace FFlag {
        inline const uintptr_t Sentinel = 0x8;
        inline const uintptr_t Buckets = 0x18;
        inline const uintptr_t Mask = 0x30;
        inline const uintptr_t DescBacking = 0x20;
        inline const uintptr_t VtStride = 0x118;

        namespace Node {
            inline const uintptr_t Prev = 0x8;
            inline const uintptr_t Name = 0x10;
            inline const uintptr_t Desc = 0x30;
        }
    }

    namespace Input {
        inline const uintptr_t MouseServiceStatePtr = 0xE8;
        inline const uintptr_t MouseStateAbsCursor = 0xBC;
        inline const uintptr_t GuiServiceWindowOrigin = 0x318;
    }

    namespace SharedString {
        inline const uintptr_t String = 0x10;
    }

    namespace Raknet {
        inline const uintptr_t RawPacketStride = 112;

        namespace PacketJob {
            inline const uintptr_t RakPeerPtr = 0x1E0;
        }

        namespace RakPeerLayout {
            inline const uintptr_t VtSlotSendBitStream = 20;
            inline const uintptr_t VtSlotSendRawBytes = 22;
            inline const uintptr_t VtSlotRunUpdateCycle = 25;
            inline const uintptr_t VtSlotDeallocatePacket = 29;
            inline const uintptr_t TotalSlots = 124;
            inline const uintptr_t RecvQueueDataPtr = 0x810;
            inline const uintptr_t RecvQueuePopIndex = 0x818;
            inline const uintptr_t RecvQueuePushIndex = 0x81C;
            inline const uintptr_t RecvQueueCapacity = 0x820;
        }

        namespace InboundPacket {
            inline const uintptr_t Length = 0x30;
            inline const uintptr_t BitSize = 0x34;
            inline const uintptr_t Data = 0x38;
            inline const uintptr_t DeleteData = 0x40;
            inline const uintptr_t PoolSlot = 0x68;
            inline const uintptr_t Stride = 0x70;
        }

        namespace BitStreamMethods {
            inline const uintptr_t VtSlotGetData = 1;
            inline const uintptr_t VtSlotGetNumberOfBytesUsed = 2;
        }

        namespace NetworkClientLayout {
            inline const uintptr_t Replicator = 0xF20;
            inline const uintptr_t ConnectionDirect = 0xF28;
        }

        namespace ReplicatorLayout {
            inline const uintptr_t Connection = 0x108;
        }

        namespace ConnectionLayout {
            inline const uintptr_t DisconnectFlag = 0xA38;
            inline const uintptr_t ClientApp = 0xA40;
        }

        namespace BitStreamBlobLayout {
            inline const uintptr_t Size = 64;
            inline const uintptr_t Vtable = 0x0;
            inline const uintptr_t DataPtr = 0x10;
            inline const uintptr_t LenU32 = 0x30;
            inline const uintptr_t CapU32 = 0x34;
            inline const uintptr_t OwnsData = 0x3E;
        }
    }

    enum ReflectionType : int {
        Type_Bool = 0x01,
        Type_Int = 0x02,
        Type_Int64 = 0x03,
        Type_Float = 0x04,
        Type_Double = 0x05,
        Type_String = 0x06,
        Type_Instance = 0x08,
        Type_Vector2 = 0x0B,
        Type_Vector3 = 0x0C,
        Type_Vector3Int16 = 0x0E,
        Type_Rect = 0x0F,
        Type_CFrame = 0x10,
        Type_Color3 = 0x11,
        Type_Color3uint8 = 0x12,
        Type_UDim = 0x13,
        Type_UDim2 = 0x14,
        Type_Region3Int16 = 0x18,
        Type_PhysicalProperties = 0x1B,
        Type_BrickColor = 0x1C,
        Type_SystemAddress = 0x1D,
        Type_BinaryString = 0x1E,
        Type_Enum = 0x21,
        Type_ColorSequence = 0x2A,
        Type_NumberRange = 0x2C,
        Type_NumberSequence = 0x2D,
        Type_SharedString = 0x3F,
        Type_DateTime = 0x40,
    };

    enum IdentityLevel : uint64_t {
        Level0_2 = 0ULL,
        Level1_4 = 0x2000000000000003ULL,
        Level3 = 0x300000000000000BULL,
        Level5 = 0x2000000000000001ULL,
        Level6 = 0x700000000000000BULL,
        Level7_8 = 0x200000000000003FULL,
        Level9_13 = 0xCULL,
        Level10 = 0x6000000000000003ULL,
        Level11 = 0x2000000000000000ULL,
        Level12 = 0x1000000000000000ULL,
    };
}
