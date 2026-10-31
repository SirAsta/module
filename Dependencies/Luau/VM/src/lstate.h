// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include <cstddef> // offsetof, for the 738 layout guards below

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
// Roblox 0.740 order: {size, nuse, hash} - back to upstream Luau, the exact
// reverse of 739's {hash, nuse, size}.
// Verified in IDA: every string-table bucket lookup in the VM does
//   mov edx, [g+18h] ; dec edx ; and rdx, hash ; mov rax, [g+20h]
// and strt sits at global_State+0x18, so `size` is strt+0x00 and `hash` is
// strt+0x08. With size/hash swapped, luaS_newlstr masks the hash with a pointer
// and dereferences an integer.
// sizeof is 16 either way, which is why global_State's asserts never catch it.
// 02c37bc: back to upstream order - hash leads, size trails.
// 742 (cec3ad5889b447cf): {size, nuse, hash} again. Verified in IDA - the three
// string-table bucket lookups (0x25C2F2B, 0x25C40C2, 0x25C5A15) all do
//   mov edx, [g+18h] ; dec edx ; and rdx, hash ; mov rax, [g+20h]
// with strt at global_State+0x18.
// 743 (7a0c6d10fcb34a2c): back to upstream {hash, size, nuse}, strt at
// global_State+0x50; from the Achilles 743 dump, not yet checked in IDA.
// 744 (0.744.0.7441077): {hash, nuse, size}, strt at global_State+0x00.
// Verified in IDA - the string bucket lookup does
//   movsxd rcx, [g+0Ch] ; dec rcx ; and rcx, hash ; mov rax, [g+0]
// and luaS_resize writes the new size to [g+0Ch] and the new array to [g+0].
typedef struct stringtable
{
    TString** hash;    /* offset 0 */
    unsigned int nuse; /* offset 8 */
    int size;          /* offset C */
} stringtable;
static_assert(offsetof(stringtable, hash) == 0x0, "stringtable::hash moved");
static_assert(offsetof(stringtable, nuse) == 0x8, "stringtable::nuse moved");
static_assert(offsetof(stringtable, size) == 0xC, "stringtable::size moved");
static_assert(sizeof(stringtable) == 0x10, "stringtable must be 16 bytes");
// clang-format on

/*
** informations about a call
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
// 0.740: base and p swapped back relative to 739. Verified in luau_execute at
// 0x26ACA3C-0x26ACA4F: `mov rax,[ci+8]` (func), `mov rcx,[ci+18h]` then
// `mov r10,[rcx+40h]` (ci->p->k), `mov r12,[ci+20h]` (savedpc), and the native
// entry test `test byte ptr [ci+2Ch], 4` (flags & LUA_CALLINFO_NATIVE).
// 02c37bc swapped base and top against 740 (base now leads).
// 742: {base, top, p, func}. Verified in IDA - luau_execute entry reads
// `[ci+10h]` (p) then `[p+20h]` (k), `[ci+18h]` (func), `[ci+20h]` (savedpc),
// tests `byte [ci+2Ch], 4`; the stack check in luaB_* compares L->top against
// `[ci+8]` (top); the capability walk (0x4185DD0) reads func at ci+0x18.
// 743: func and top swapped back {base, func, p, top}; from the Achilles 743
// dump, not yet checked in IDA.
struct CallInfo {
    Proto* p;
    TValue* func;
    TValue* base;
    TValue* top;
    union {
        const Instruction* savedpc;
        int errfunc;
    };
    int nresults;
    unsigned int flags;
};

// clang-format on

#define LUA_CALLINFO_RETURN (1 << 0) // should the interpreter return after returning from this callinfo? first frame must have this set
#define LUA_CALLINFO_HANDLE (1 << 1) // should the error thrown during execution get handled by continuation from this callinfo? func must be C
#define LUA_CALLINFO_NATIVE (1 << 2) // should this function be executed using execution callback for native code
#define LUA_CALLINFO_OPYIELD (1 << 3) // call frame has yielded on a non-call opcode and requires luaV_finishop
#define LUA_CALLINFO_PCALL (1 << 4)   // call frame was setup by a synthetic protected call and requires luau_pospcallsuccess

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

/*
** `global state', shared by all threads of this state
*/
// clang-format off
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



// Roblox 0.740 layout (version-2366ba214ec740ca). The head is reshuffled again
// from 739 - and this time it lands back on plain upstream Luau field order, so
// nothing from the 739 header carries over. Take the .739.bak seriously before
// assuming any single field "did not move".
//
// Verified in IDA:
//   strt 0x18  - bucket lookups do `mov edx,[g+18h]; dec edx; and rdx,hash;
//                mov rax,[g+20h]`, i.e. strt.size +0x18 / strt.hash +0x20
//   GCthreshold 0x50 / totalbytes 0x58 - the luaC_step guard at 0x42A6D37 reads
//                `[g+50h]`, compares `[g+58h]` and calls luaC_step (0x26838F0)
//   grayagain 0x38 - upvalue re-link in 0x42A6D60
//   freepages 0x1E0 - page allocator 0x269C740 / 0x269CE00 index `[g+rax*8+1E0h]`
//   cb 0x640   - LOP_BREAK tests `[g+650h]` (cb.debugbreak, cb+0x10)
//   ecb 0x6D8  - luau_execute's native entry calls `[g+6F0h]` (ecb.enter, +0x18)
//   interrupt is cb+0x48: 8 sites in luau_execute load `[g+688h]`, null-check it,
//   store savedpc and call it with rdx = -1.
// 02c37bc moved everything before 0x4B0. Verified in IDA against luaC_step
// (0x26670B0) and the gcstep worker (0x2666E90), both reached through
// g = *(L + 0x68):
//   *(g+0x10) / *(g+0x18)   GCthreshold / totalbytes  (were 0x50 / 0x58)
//   *(g+0x20) / *(g+0x21)   currentwhite / gcstate    (were 0x00 / 0x01)
//   *(g+0x40)               grayagain, relinked by the thread barrier (was 0x38)
//   *(g+0x48)               gray                      (was 0x28)
//   *(g+0x54) / *(g+0x58)   gcstepsize / gcstepmul    (were 0x40 / 0x44)
//   *(g+0x2E0) / *(g+0x2E8) sweepgcopage / mainthread (were 0x98 / 0x60)
//   *(g+0x5F8)              sweepgcopage_cage
//   *(g + 8*tt + 808)       mt[]  -> 0x328            (was 0x440)
//   *(g+0x08) / *(g+0x0C)   strt.nuse / strt.size, so strt leads at 0x00
//                           (was 0x18 - this is what made luaS_newlstr read a
//                            garbage hash table and fault on a valid state)
struct global_State {
    stringtable strt;
    GCObject* gray;
    GCObject* weak;
    GCObject* grayagain;
    lua_Alloc frealloc;
    void* ud;
    unsigned char currentwhite;
    unsigned char gcstate;
    int gcgoal;
    int gcstepsize;
    int gcstepmul;
    size_t GCthreshold;
    size_t totalbytes;
    lua_Page* allgcopages;
    lua_Page* allpages;
    lua_Page* sweepgcopage;
    lua_State* mainthread;
    UpVal uvhead;
    lua_Page* freepages[40];
    lua_Page* freegcopages[40];
    TString* tmname[TM_N];
    TString* ttname[LUA_T_COUNT];
    LuaTable* mt[LUA_T_COUNT];
    lua_Page* sweepgcopage_cage;
    void* cageud;
    lua_Page* allgcopages_cage;
    lua_CageAlloc cagealloc;
    lua_Page* freegcopages_cage[40];
    TValue pseudotemp;
    TValue registry;
    registryfree_value registryfree;
    struct lua_jmpbuf* errorjmp;
    unsigned __int64 rngstate;
    lua_Callbacks cb;
    unsigned __int64 ptrenckey[4];
    lua_ExecutionCallbacks ecb;
    alignas(16) uint8_t ecbdata[LUA_EXECUTION_CALLBACK_STORAGE];
    lua_UdataDirectAccessData udatadirect[UTAG_INTERNAL_LIMIT];
    struct {
        char unseen[0x140];
        lua_Page* head;
        lua_Page* sweepcursor;
    } pagesets[8];
    int pagesetcursor;
    size_t memcatbytes[LUA_MEMORY_CATEGORIES];
    void (*udatagc[LUA_UTAG_LIMIT])(struct lua_State*, void*);
    lua_UserdataMark udatamark[LUA_UTAG_LIMIT];
    LuaTable* udatamt[LUA_UTAG_LIMIT];
    TValue weakregistry;
    int weakregistryfree;
    int weakregistrytop;
    lua_EmbedderGc embeddergc;
    TString* lightuserdataname[LUA_LUTAG_LIMIT];
    struct LuaTable* udatadirectfields[UTAG_INTERNAL_LIMIT];
    Closure* builtinPcall;
    Closure* builtinXpcall;
    unsigned __int64 ptrenckeynew[8];
    bool ptrencactive;
    struct GCStats gcstats;
    unsigned int lastprotoid;
    unsigned int lastclassid;
#ifdef LUAI_GCMETRICS
    GCMetrics gcmetrics;
#endif
};


// ---------------------------------------------------------------------------
// Layout guards for Roblox 743 (7a0c6d10fcb34a2c).
//
// Every field below is read or written against memory Roblox allocated, so a
// size drift in ANY member type silently shifts everything after it and turns
// into a heap corruption at runtime. These asserts turn that into a build
// error instead. They are how the lua_Callbacks 104-vs-112 byte bug was found.
//
// If one of these fires, do NOT adjust the number - find the member type whose
// size changed and fix that.
// ---------------------------------------------------------------------------
static_assert(sizeof(lua_UdataDirectAccessData) == 0x48, "udatadirect stride must be 0x48");
static_assert(sizeof(lua_ExecutionCallbacks) == 0x48, "lua_ExecutionCallbacks must be 72 bytes");

static_assert(offsetof(global_State, strt) == 0x0000, "global_State::strt");
static_assert(offsetof(global_State, currentwhite) == 0x0010, "global_State::currentwhite");
static_assert(offsetof(global_State, gcgoal) == 0x0014, "global_State::gcgoal");
static_assert(offsetof(global_State, frealloc) == 0x0020, "global_State::frealloc");
static_assert(offsetof(global_State, ud) == 0x0028, "global_State::ud");
static_assert(offsetof(global_State, gray) == 0x0030, "global_State::gray");
static_assert(offsetof(global_State, grayagain) == 0x0038, "global_State::grayagain");
static_assert(offsetof(global_State, weak) == 0x0040, "global_State::weak");
static_assert(offsetof(global_State, GCthreshold) == 0x0048, "global_State::GCthreshold");
static_assert(offsetof(global_State, totalbytes) == 0x0050, "global_State::totalbytes");
static_assert(offsetof(global_State, freepages) == 0x0058, "global_State::freepages");
static_assert(offsetof(global_State, allpages) == 0x0198, "global_State::allpages");
static_assert(offsetof(global_State, mainthread) == 0x01A0, "global_State::mainthread");
static_assert(offsetof(global_State, uvhead) == 0x01A8, "global_State::uvhead");
static_assert(offsetof(global_State, freegcopages) == 0x01D0, "global_State::freegcopages");
static_assert(offsetof(global_State, sweepgcopage) == 0x0310, "global_State::sweepgcopage");
static_assert(offsetof(global_State, allgcopages) == 0x0318, "global_State::allgcopages");
static_assert(offsetof(global_State, mt) == 0x0320, "global_State::mt");
static_assert(offsetof(global_State, tmname) == 0x0390, "global_State::tmname");
static_assert(offsetof(global_State, ttname) == 0x0438, "global_State::ttname");
static_assert(offsetof(global_State, cagealloc) == 0x04A8, "global_State::cagealloc");
static_assert(offsetof(global_State, allgcopages_cage) == 0x04B0, "global_State::allgcopages_cage");
static_assert(offsetof(global_State, sweepgcopage_cage) == 0x04B8, "global_State::sweepgcopage_cage");
static_assert(offsetof(global_State, freegcopages_cage) == 0x04C0, "global_State::freegcopages_cage");
static_assert(offsetof(global_State, cageud) == 0x0600, "global_State::cageud");
static_assert(offsetof(global_State, pseudotemp) == 0x0608, "global_State::pseudotemp");
static_assert(offsetof(global_State, registry) == 0x0618, "global_State::registry");
static_assert(offsetof(global_State, registryfree) == 0x0628, "global_State::registryfree");
static_assert(offsetof(global_State, errorjmp) == 0x0630, "global_State::errorjmp");
static_assert(offsetof(global_State, ptrenckey) == 0x0638, "global_State::ptrenckey");
static_assert(offsetof(global_State, rngstate) == 0x0658, "global_State::rngstate");
static_assert(offsetof(global_State, cb) == 0x0660, "global_State::cb");
static_assert(offsetof(global_State, ecb) == 0x06D0, "global_State::ecb");
static_assert(offsetof(global_State, ecbdata) == 0x0720, "global_State::ecbdata");
static_assert(offsetof(global_State, udatadirect) == 0x0920, "global_State::udatadirect");
static_assert(offsetof(global_State, memcatbytes) == 0x2DB0, "global_State::memcatbytes");
static_assert(offsetof(global_State, udatagc) == 0x35B0, "global_State::udatagc");
static_assert(offsetof(global_State, udatamark) == 0x39B0, "global_State::udatamark");
static_assert(offsetof(global_State, udatamt) == 0x3DB0, "global_State::udatamt");
static_assert(offsetof(global_State, weakregistry) == 0x41B0, "global_State::weakregistry");
static_assert(offsetof(global_State, embeddergc) == 0x41C8, "global_State::embeddergc");
static_assert(offsetof(global_State, lightuserdataname) == 0x41D0, "global_State::lightuserdataname");
static_assert(offsetof(global_State, udatadirectfields) == 0x45D0, "global_State::udatadirectfields - check LUA_LUTAG_LIMIT is 128");
static_assert(offsetof(global_State, builtinPcall) == 0x49E0, "global_State::builtinPcall - check LUA_LUTAG_LIMIT is 128");
static_assert(offsetof(global_State, ptrenckeynew) == 0x49F0, "global_State::ptrenckeynew");
static_assert(offsetof(global_State, gcstats) == 0x4A38, "global_State::gcstats");
static_assert(offsetof(global_State, lastprotoid) == 0x4AF0, "global_State::lastprotoid");
static_assert(offsetof(global_State, lastclassid) == 0x4AF4, "global_State::lastclassid");
#ifndef LUAI_GCMETRICS
static_assert(sizeof(global_State) == 0x4B00, "global_State must be 19200 bytes");
#endif
// clang-format on

/*
** `per thread' state
*/
// clang-format off

// 02c37bc layout. Still 136 bytes, but the whole tail was reordered against 740 -
// the pointer block that used to lead (ci/base/global/stack/top) now trails, and
// the bookkeeping fields moved to the front. Deltas from 740:
//   singlestep / isactive   swapped (singlestep 0x05, isactive 0x06)
//   namecall   0x58 -> 0x08      gclist     0x50 -> 0x10
//   stacksize  0x38 -> 0x18      openupval  0x40 -> 0x20
//   userdata   0x78 -> 0x28      finalizers 0x48 -> 0x30
//   nCcalls    0x80 -> 0x38      cachedslot 0x84 -> 0x3C
//   gt         0x60 -> 0x40      end_ci     0x68 -> 0x48
//   base_ci    0x70 -> 0x50      ci         0x08 -> 0x58
//   base       0x10 -> 0x60      global     0x18 -> 0x68
//   top        0x28 -> 0x70      stack      0x20 -> 0x78
//   stack_last 0x30 -> 0x80
// 742 (cec3ad5889b447cf). Reordered again; matches the Achilles 742 dump.
// Verified in IDA for this build:
//   luau_execute entry `cmp byte [L+6], 0`          -> singlestep 0x06
//   every luaB_*: top `[L+20h]`, base `[L+28h]`,
//     ci `[L+8]` (ci->top at +8), g `[L+18h]`       -> top/base/ci/global
//   thread barrier `mov [L+50h], [g+58h]`          -> gclist 0x50
//   __index TM path writes `[L+5Ch]`               -> cachedslot 0x5C
//   LOP_BREAK `cmp byte [L+3], 0`                  -> status 0x03
//   userthread cb (0x4210720) reads/writes `[L+80h]` -> userdata 0x80
//   capability walk (0x4185DD0) `a1[15]`, `a1[1]`  -> base_ci 0x78, ci 0x08
//   upvalue close loop `[L+48h]`                   -> openupval 0x48
// 743 (7a0c6d10fcb34a2c): reordered again (userdata 0x80 -> 0x08, global
// 0x18 -> 0x50); from the Achilles 743 dump, not yet checked in IDA.
struct lua_State {
    CommonHeader;
    uint8_t status;
    uint8_t activememcat;
    bool singlestep;
    bool isactive;
    unsigned short nCcalls;
    unsigned short baseCcalls;
    unsigned int cachedslot;
    CallInfo* end_ci;
    CallInfo* base_ci;
    struct RobloxExtraSpace* userdata;
    GCObject* gclist;
    CallInfo* ci;
    global_State* global;
    TValue* stack;
    TValue* top;
    TValue* base;
    TValue* stack_last;
    LuaTable* finalizers;
    TString* namecall;
    LSTATE_STACKSIZE_ENC<int> stacksize;
    int size_ci;
    UpVal* openupval;
    LuaTable* gt;
};


static_assert(offsetof(lua_State, status) == 0x03, "lua_State::status");
static_assert(offsetof(lua_State, activememcat) == 0x04, "lua_State::activememcat");
static_assert(offsetof(lua_State, singlestep) == 0x05, "lua_State::singlestep");
static_assert(offsetof(lua_State, isactive) == 0x06, "lua_State::isactive");
static_assert(offsetof(lua_State, gt) == 0x08, "lua_State::gt");
static_assert(offsetof(lua_State, end_ci) == 0x10, "lua_State::end_ci");
static_assert(offsetof(lua_State, base_ci) == 0x18, "lua_State::base_ci");
static_assert(offsetof(lua_State, nCcalls) == 0x20, "lua_State::nCcalls");
static_assert(offsetof(lua_State, cachedslot) == 0x24, "lua_State::cachedslot");
static_assert(offsetof(lua_State, userdata) == 0x28, "lua_State::userdata");
static_assert(offsetof(lua_State, finalizers) == 0x30, "lua_State::finalizers");
static_assert(offsetof(lua_State, namecall) == 0x38, "lua_State::namecall");
static_assert(offsetof(lua_State, gclist) == 0x40, "lua_State::gclist");
static_assert(offsetof(lua_State, openupval) == 0x48, "lua_State::openupval");
static_assert(offsetof(lua_State, ci) == 0x50, "lua_State::ci");
static_assert(offsetof(lua_State, base) == 0x58, "lua_State::base");
static_assert(offsetof(lua_State, stack) == 0x60, "lua_State::stack");
static_assert(offsetof(lua_State, stack_last) == 0x68, "lua_State::stack_last");
static_assert(offsetof(lua_State, top) == 0x70, "lua_State::top");
static_assert(offsetof(lua_State, global) == 0x78, "lua_State::global");
static_assert(offsetof(lua_State, stacksize) == 0x80, "lua_State::stacksize");
static_assert(sizeof(lua_State) == 0x88, "lua_State must be 136 bytes");

static_assert(offsetof(CallInfo, base) == 0x00, "CallInfo::base");
static_assert(offsetof(CallInfo, func) == 0x08, "CallInfo::func");
static_assert(offsetof(CallInfo, p) == 0x10, "CallInfo::p");
static_assert(offsetof(CallInfo, top) == 0x18, "CallInfo::top");
static_assert(offsetof(CallInfo, savedpc) == 0x20, "CallInfo::savedpc");
static_assert(offsetof(CallInfo, flags) == 0x2C, "CallInfo::flags");
static_assert(sizeof(CallInfo) == 0x30, "CallInfo must be 48 bytes");


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
