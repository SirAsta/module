// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lobject.h"
#include "ltm.h"
#include "ludata.h"

// registry
#define registry(L) (&L->global->registry)

// extra stack space to handle TM calls and some other extras
#define EXTRA_STACK 5

#define BASIC_CI_SIZE 8

#define BASIC_STACK_SIZE (2 * LUA_MINSTACK)

// clang-format off
// Structure For Hash(f0ea5da9412f4e35)

struct stringtable {
    int size; /* (0) (0x0) */
    uint32_t nuse; /* (4) (0x4) */
    TString** hash; /* (8) (0x8) */
};
static_assert(sizeof(stringtable) == 16, "sizeof(stringtable) == 16");

// clang-format on

/*
** information about a call
**
** the general Lua stack frame structure is as follows:
** - each function gets a stack frame, with function "registers" being stack slots on the frame
** - function arguments are associated with registers 0+
** - function locals and temporaries follow after; usually locals are a consecutive block per scope, and temporaries are allocated after this, but
*this is up to the compiler
**
** when function doesn't have varargs, the stack layout is as follows:
** ^ (func) ^^ [fixed args] [locals + temporaries]
** where ^ is the 'func' pointer in CallInfo struct, and ^^ is the 'base' pointer (which is what registers are relative to)
**
** when function *does* have varargs, the stack layout is more complex - the runtime has to copy the fixed arguments so that the 0+ addressing still
*works as follows:
** ^ (func) [fixed args] [varargs] ^^ [fixed args] [locals + temporaries]
**
** computing the sizes of these individual blocks works as follows:
** - the number of fixed args is always matching the `numparams` in a function's Proto object; runtime adds `nil` during the call execution as
*necessary
** - the number of variadic args can be computed by evaluating (ci->base - ci->func - 1 - numparams)
**
** the CallInfo structures are allocated as an array, with each subsequent call being *appended* to this array (so if f calls g, CallInfo for g
*immediately follows CallInfo for f)
** the `nresults` field in CallInfo is set by the caller to tell the function how many arguments the caller is expecting on the stack after the
*function returns
** the `flags` field in CallInfo contains internal execution flags that are important for pcall/etc, see LUA_CALLINFO_*
*/
// clang-format off
// Structure For Hash(f0ea5da9412f4e35)
struct CallInfo {
    TValue* base; /* (0) (0x0) */
    TValue* top; /* (8) (0x8) */
    TValue* func; /* (16) (0x10) */
    Proto* p; /* (24) (0x18) */
    union {
        const Instruction* savedpc;
        int errfunc;
    }; /* (32) (0x20) */
    int nresults; /* (40) (0x28) */
    unsigned int flags; /* (44) (0x2C) */
};
static_assert(sizeof(CallInfo) == 48, "sizeof(CallInfo) == 48");
// clang-format on

#define LUA_CALLINFO_RETURN (1 << 0) // should the interpreter return after returning from this callinfo? first frame must have this set
#define LUA_CALLINFO_HANDLE (1 << 1) // should the error thrown during execution get handled by continuation from this callinfo? func must be C
#define LUA_CALLINFO_NATIVE (1 << 2) // should this function be executed using execution callback for native code
#define LUA_CALLINFO_OPYIELD (1 << 3) // call frame has yielded on a non-call opcode and requires luau_finishop
#define LUA_CALLINFO_PCALL (1 << 4)   // call frame was setup by a synthetic protected call and requires luau_pospcallsuccess

#define TAGGED_PAGESET_BASE (LUA_TSTRING)
#define TAGGED_PAGESET_SIZE (LUA_T_COUNT - TAGGED_PAGESET_BASE)

#define curr_func(L) (clvalue(L->ci->func))
#define ci_func(ci) (clvalue((ci)->func))
#define f_isLua(ci) (!ci_func(ci)->isC)
#define isLua(ci) (ttisfunction((ci)->func) && f_isLua(ci))

struct GCStats
{
    // data for proportional-integral controller of heap trigger value
    int32_t triggerterms[32] = {0};
    uint32_t triggertermpos = 0;
    int32_t triggerintegral = 0;

    size_t atomicstarttotalsizebytes = 0;
    size_t endtotalsizebytes = 0;
    size_t heapgoalsizebytes = 0;

    double starttimestamp = 0;
    double atomicstarttimestamp = 0;
    double endtimestamp = 0;
};

#ifdef LUAI_GCMETRICS
struct GCCycleMetrics
{
    size_t starttotalsizebytes = 0;
    size_t heaptriggersizebytes = 0;

    double pausetime = 0.0; // time from end of the last cycle to the start of a new one

    double starttimestamp = 0.0;
    double endtimestamp = 0.0;

    double marktime = 0.0;
    double markassisttime = 0.0;
    double markmaxexplicittime = 0.0;
    size_t markexplicitsteps = 0;
    size_t markwork = 0;

    double atomicstarttimestamp = 0.0;
    size_t atomicstarttotalsizebytes = 0;
    double atomictime = 0.0;

    // specific atomic stage parts
    double atomictimeupval = 0.0;
    double atomictimeweak = 0.0;
    double atomictimegray = 0.0;
    double atomictimeembedder = 0.0;
    double atomictimeclear = 0.0;

    double sweeptime = 0.0;
    double sweepassisttime = 0.0;
    double sweepmaxexplicittime = 0.0;
    size_t sweepexplicitsteps = 0;
    size_t sweepwork = 0;

    size_t assistwork = 0;
    size_t explicitwork = 0;

    size_t propagatework = 0;
    size_t propagateagainwork = 0;

    size_t endtotalsizebytes = 0;
};

struct GCMetrics
{
    double stepexplicittimeacc = 0.0;
    double stepassisttimeacc = 0.0;

    // when cycle is completed, last cycle values are updated
    uint64_t completedcycles = 0;

    GCCycleMetrics lastcycle;
    GCCycleMetrics currcycle;
};
#endif

// Callbacks that can be used to to redirect code execution from Luau bytecode VM to a custom implementation (AoT/JiT/sandboxing/...)
struct lua_ExecutionCallbacks
{
    void* context;
    void (*close)(lua_State* L);                 // called when global VM state is closed
    void (*destroy)(lua_State* L, Proto* proto); // called when function is destroyed
    int (*enter)(lua_State* L, Proto* proto);    // called when function is about to start/resume (when execdata is present), return 0 to exit VM
    void (*disable)(lua_State* L, Proto* proto); // called when function has to be switched from native to bytecode in the debugger
    size_t (*getmemorysize)(lua_State* L, Proto* proto); // called to request the size of memory associated with native part of the Proto
    uint8_t (*gettypemapping)(lua_State* L, const char* str, size_t len); // called to get the userdata type index
    char* (*getcounterdata)(
        lua_State* L,
        Proto* proto,
        size_t* count
    ); // called to get the execution counter data and count {uint32_t, uint32_t, uint64_t}
    Proto* (*inlinefunction)(lua_State* L, Closure* caller, Closure* target, uint32_t pc); // called when inlining threshold is reached

    // called when GC marks the function unless opted out with PEF_EXECLOCKED to examine if an action on a function will be needed later
    void (*examinefunction)(lua_State* L, Proto* proto);
    // called when GC cycle has been completed and the collector is paused
    // functions previoulsy recorded by 'examinefunction' can be processed safely
    void (*commitfunctions)(lua_State* L);
};

struct lua_UdataDirectAccessData
{
    TValue indextm;
    TValue newindextm;
    TValue namecalltm;
    lua_UserdataDirectAccess index;
    lua_UserdataDirectAccess newindex;
    lua_UserdataDirectNamecall namecall;
};

struct LuauPageset
{
    struct lua_Page* freepages[LUA_SIZECLASSES]; // cage GCO free pages, per size class
    struct lua_Page* allpages;                   // all cage GCO pages (sweep target)
    struct lua_Page* sweeppage;                  // sweep cursor
};

/*
** `global state', shared by all threads of this state
*/
// clang-format off
//typedef struct global_State
//{
//    stringtable strt; // hash table for strings
//
//    lua_Alloc frealloc;   // function to reallocate memory
//    void* ud;             // auxiliary data to `frealloc'
//
//    uint8_t currentwhite;
//    uint8_t gcstate; // state of garbage collector
//
//    GCObject* gray;      // list of gray objects
//    GCObject* grayagain; // list of objects to be traversed atomically
//    GCObject* weak;      // list of weak tables (to be cleared)
//
//    size_t GCthreshold;                       // when totalbytes >= GCthreshold, run GC step
//    size_t totalbytes;                        // number of bytes currently allocated
//
//    int gcgoal;                               // see LUAI_GCGOAL
//    int gcstepmul;                            // see LUAI_GCSTEPMUL
//    int gcstepsize;                           // see LUAI_GCSTEPSIZE
//
//    struct lua_Page* freepages[LUA_SIZECLASSES]; // free page linked list for each size class for non-collectable objects
//    struct lua_Page* freegcopages[LUA_SIZECLASSES]; // free page linked list for each size class for collectable objects
//    struct lua_Page* allpages; // page linked list with all pages for all non-collectable object classes (available with LUAU_ASSERTENABLED)
//    struct lua_Page* allgcopages; // page linked list with all pages for all collectable object classes
//    struct lua_Page* sweepgcopage; // position of the sweep in `allgcopages'
//
//    struct lua_State* mainthread;
//    UpVal uvhead; // head of double-linked list of all open upvalues
//    struct LuaTable* mt[LUA_T_COUNT]; // metatables for basic types
//    TString* ttname[LUA_T_COUNT]; // names for basic types
//    TString* tmname[TM_N]; // array with tag-method names
//
//    lua_CageAlloc cagealloc;
//    void* cageud;
//    struct lua_Page* freegcopages_cage[LUA_SIZECLASSES]; // cage GCO free pages, per size class
//    struct lua_Page* allgcopages_cage; // all cage GCO pages (sweep target)
//    struct lua_Page* sweepgcopage_cage; // sweep cursor
//
//    TValue pseudotemp; // storage for temporary values used in pseudo2addr
//
//    TValue registry; // registry table, used by lua_ref and LUA_REGISTRYINDEX
//    int registryfree; // next free slot in registry
//
//    struct lua_jmpbuf* errorjmp; // jump buffer data for longjmp-style error handling
//
//    uint64_t rngstate; // PCG random number generator state
//    uint64_t ptrenckey[4]; // pointer encoding key for display (remove when unreferenced)
//
//    lua_Callbacks cb;
//
//    lua_ExecutionCallbacks ecb;
//
//    alignas(16) uint8_t ecbdata[LUA_EXECUTION_CALLBACK_STORAGE];
//
//    // Set of userdata __index/__newindex/__namecall metamethods for a direct access
//    lua_UdataDirectAccessData udatadirect[UTAG_INTERNAL_LIMIT];
//
//    LuauPageset taggedpageset[TAGGED_PAGESET_SIZE]; // Page sets which only hold allocations for a specific GCO type
//    int sweeppageset; // Which array is being swept
//
//    size_t memcatbytes[LUA_MEMORY_CATEGORIES]; // total amount of memory used by each memory category
//
//    void (*udatagc[LUA_UTAG_LIMIT])(lua_State*, void*); // for each userdata tag, a gc callback to be called immediately before freeing memory
//    lua_UserdataMark udatamark[LUA_UTAG_LIMIT]; // gc callbacks allowing the embedder to mark underlying native objects for the given userdata
//    LuaTable* udatamt[LUA_UTAG_LIMIT]; // metatables for tagged userdata
//
//    TValue weakregistry; // backing table for lua_weakref/lua_weakunref/lua_getweakref
//    int weakregistryfree; // next free slot in weakregistry
//    int weakregistrytop; // top of the weakregistry (highest used index)
//    lua_EmbedderGc embeddergc; // embedder GC callback for keeping weak references alive
//
//    TString* lightuserdataname[LUA_LUTAG_LIMIT]; // names for tagged lightuserdata
//
//    // per-tag direct field dispatch tables; NULL until first field is registered for that tag
//    struct LuaTable* udatadirectfields[UTAG_INTERNAL_LIMIT];
//
//    // cached closures for fast protected calls
//    struct Closure* builtinPcall;
//    struct Closure* builtinXpcall;
//
//    uint64_t ptrenckeynew[8]; // pointer encoding key for display
//    bool ptrencactive;
//
//    GCStats gcstats;
//    uint32_t lastprotoid;
//    uint32_t lastclassid;
//
//#ifdef LUAI_GCMETRICS
//    GCMetrics gcmetrics;
//#endif
//} global_State;

struct registryfree_value
{
private:
    int _value;

public:
    registryfree_value() : _value(0) {}
    registryfree_value(int val) : _value(val) {}
    registryfree_value(const registryfree_value& other) : _value(other._value) {}

    void operator=(const registryfree_value& value)
    {
        _value = value._value;
    }

    void operator=(const int& value)
    {
        _value = (_value & 0xF0000000) | (value & 0xFFFFFFF);
    }

    operator const int() const
    {
        return _value & 0xFFFFFFF;
    }

    bool operator==(const int& value) const
    {
        return (_value & 0xFFFFFFF) == value;
    }

    bool operator!=(const int& value) const
    {
        return (_value & 0xFFFFFFF) != value;
    }
};

typedef registryfree_value registryfree_t;
typedef registryfree_value RegistryFree;

// Structure For Hash(f0ea5da9412f4e35)

struct global_State {
    unsigned char currentwhite; /* (0) (0x0) */
    char gap_0[0x1]; //new member?
    unsigned char gcstate; /* (2) (0x2) */
    char gap_1[0x5];
    lua_Alloc frealloc; /* (8) (0x8) */
    void* ud; /* (16) (0x10) */
    int gcstepsize; /* (24) (0x18) */
    int gcstepmul; /* (28) (0x1C) */
    int gcgoal; /* (32) (0x20) */
    char gap_2[0x4];
    GCObject* gray; /* (40) (0x28) */
    GCObject* grayagain; /* (48) (0x30) */
    GCObject* weak; /* (56) (0x38) */
    char gap_3[0x8]; //new member?
    stringtable strt; /* (72) (0x48) */
    char gap_4[0x8]; //new member?
    size_t GCthreshold; /* (96) (0x60) */
    size_t totalbytes; /* (104) (0x68) */
    lua_Page* sweepgcopage; /* (112) (0x70) */
    lua_State* mainthread; /* (120) (0x78) */
    UpVal uvhead; /* (128) (0x80) */
    lua_Page* allgcopages; /* (168) (0xA8) */
    lua_Page* freepages[40]; /* (176) (0xB0) */
    lua_Page* freegcopages[40]; /* (496) (0x1F0) */
    lua_Page* allpages; /* (818) (0x330) */
    LuaTable* mt[LUA_T_COUNT]; /* (824) (0x338) */
    TString* ttname[LUA_T_COUNT]; /* (936) (0x3A8) */
    TString* tmname[TM_N]; /* (1048) (0x418) */
    lua_Page* freegcopages_cage[40]; /* (1216) (0x4C0) */
    void* cageud; /* (1536) (0x600) */
    lua_Page* allgcopages_cage; /* (1544) (0x608) */
    lua_CageAlloc cagealloc; /* (1552) (0x610) */
    lua_Page* sweepgcopage_cage; /* (1560) (0x618) */
    TValue pseudotemp; /* (1568) (0x620) */
    TValue registry; /* (1584) (0x630) */
    registryfree_value registryfree; /* (1600) (0x640) */
    char gap_8[0x4];
    struct lua_jmpbuf* errorjmp; /* (1608) (0x648) */
    uint64_t ptrenckey[4]; /* (1616) (0x650) */
    lua_Callbacks cb; /* (1648) (0x670) */
    uint64_t rngstate; /* (1760) (0x6E0) */
    lua_ExecutionCallbacks ecb; /* (1768) (0x6E8) */
    alignas(16) uint8_t ecbdata[LUA_EXECUTION_CALLBACK_STORAGE]; /* (1856) (0x740) */
    lua_UdataDirectAccessData udatadirect[UTAG_INTERNAL_LIMIT]; /* (2368) (0x940) */
    LuauPageset taggedpageset[TAGGED_PAGESET_SIZE];
    int sweeppageset; /* (14416) (0x3850) */
    char gap_22[0x4];
    size_t memcatbytes[LUA_MEMORY_CATEGORIES]; /* (14424) (0x3858) */
    void (*udatagc[LUA_UTAG_LIMIT])(struct lua_State*, void*); /* (16472) (0x4058) */
    lua_UserdataMark udatamark[LUA_UTAG_LIMIT];
    LuaTable* udatamt[LUA_UTAG_LIMIT]; /* (18520) (0x4858) */
    TValue weakregistry;
    int weakregistryfree;
    int weakregistrytop;
    lua_EmbedderGc embeddergc;
    TString* lightuserdataname[LUA_LUTAG_LIMIT]; /* (19560) (0x4C68) */
    LuaTable* udatadirectfields[UTAG_INTERNAL_LIMIT]; /* (20600) (0x5078) */
    Closure* builtinPcall; /* (21640) (0x5488) */
    Closure* builtinXpcall; /* (21648) (0x5490) */
    uint64_t ptrenckeynew[8]; /* (21656) (0x5498) */
    bool ptrencactive; /* (21720) (0x54D8) */
    char gap_13[0x7];
    GCStats gcstats; /* (21728) (0x54E0) */
    unsigned int lastprotoid; /* (21912) (0x5598) */
    unsigned int lastclassid; /* (21916) (0x559C) */
#ifdef LUAI_GCMETRICS
    GCMetrics gcmetrics; /* (21920) (0x55A0) */
#endif
};
#ifdef LUAI_GCMETRICS
static_assert(sizeof(global_State) == 22400, "sizeof(global_State) == 22400");
#else
static_assert(sizeof(global_State) == 21920, "sizeof(global_State) == 21920");
#endif

// clang-format on

/*
** `per thread' state
*/
// clang-format off

// Structure For Hash(f0ea5da9412f4e35)

struct lua_State {
    CommonHeader; /* (0) (0x0) */
    uint8_t status; /* (3) (0x3) */
    bool singlestep; /* (4) (0x4) */
    bool isactive; /* (5) (0x5) */
    uint8_t activememcat; /* (6) (0x6) */
    char gap_0[0x1];
    struct RobloxExtraSpace* userdata; /* (8) (0x8) */
    LuaTable* gt; /* (16) (0x10) */
    TString* namecall; /* (24) (0x18) */
    CallInfo* end_ci; /* (32) (0x20) */
    CallInfo* base_ci; /* (40) (0x28) */
    UpVal* openupval; /* (48) (0x30) */
    unsigned short nCcalls; /* (56) (0x38) */
    unsigned short baseCcalls; /* (58) (0x3A) */
    unsigned int cachedslot; /* (60) (0x3C) */
    GCObject* gclist; /* (64) (0x40) */
    TValue* stack; /* (72) (0x48) */
    global_State* global; /* (80) (0x50) */
    TValue* base; /* (88) (0x58) */
    TValue* top; /* (96) (0x60) */
    TValue* stack_last; /* (104) (0x68) */
    CallInfo* ci; /* (112) (0x70) */
    LSTATE_STACKSIZE_ENC<int> stacksize; /* (120) (0x78) */
    int size_ci; /* (124) (0x7C) */
    LuaTable* finalizers; /* (128) (0x80) */
};
static_assert(sizeof(lua_State) == 136, "sizeof(lua_State) == 136");

// clang-format on

/*
** Union of all collectible objects
*/
union GCObject
{
    GCheader gch;
    struct TString ts;
    struct Udata u;
    struct Closure cl;
    struct LuaTable h;
    struct Proto p;
    struct UpVal uv;
    struct lua_State th; // thread
    struct LuauBuffer buf;
    struct LuauClass lclass;
    struct LuauObject lobject;
    struct LuauVector vec;
};

// macros to convert a GCObject into a specific value
#define gco2ts(o) check_exp((o)->gch.tt == LUA_TSTRING, &((o)->ts))
#define gco2u(o) check_exp((o)->gch.tt == LUA_TUSERDATA, &((o)->u))
#define gco2cl(o) check_exp((o)->gch.tt == LUA_TFUNCTION, &((o)->cl))
#define gco2h(o) check_exp((o)->gch.tt == LUA_TTABLE, &((o)->h))
#define gco2p(o) check_exp((o)->gch.tt == LUA_TPROTO, &((o)->p))
#define gco2uv(o) check_exp((o)->gch.tt == LUA_TUPVAL, &((o)->uv))
#define gco2th(o) check_exp((o)->gch.tt == LUA_TTHREAD, &((o)->th))
#define gco2buf(o) check_exp((o)->gch.tt == LUA_TBUFFER, &((o)->buf))
#define gco2class(o) check_exp((o)->gch.tt == LUA_TCLASS, &((o)->lclass))
#define gco2object(o) check_exp((o)->gch.tt == LUA_TOBJECT, &((o)->lobject))
#define gco2vec(o) check_exp((o)->gch.tt == LUA_TVECTOR, &((o)->vec))

// macro to convert any Lua object into a GCObject
#define obj2gco(v) check_exp(iscollectable(v), cast_to(GCObject*, (v) + 0))

LUAI_FUNC lua_State* luaE_newthread(lua_State* L);
LUAI_FUNC void luaE_freethread(lua_State* L, lua_State* L1, struct lua_Page* page);
