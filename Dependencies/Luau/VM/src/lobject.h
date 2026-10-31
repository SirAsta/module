// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include <cstddef> // offsetof, for the 738 layout guards

#include "lua.h"
#include "lcommon.h"

/*
** Union of all collectible objects
*/
typedef union GCObject GCObject;

/*
** Common Header for all collectible objects (in macro form, to be included in other objects)
*/
// clang-format off
// Roblox 0.740 order: tt, marked, memcat - marked and memcat swapped back
// relative to 739.
// Verified in IDA at 0x42A6D4F, the thread barrier ahead of a resume:
//   movzx eax, byte [L+1] ; test al, 4 ; jz ...    (black bit)
//   and al, 0FBh ; mov [L+1], al                   (clear black)
//   mov rax, [g+38h] ; mov [L+50h], rax ; mov [g+38h], L   (relink grayagain)
// so `marked` is byte1 and `memcat` is byte2.
// Getting this wrong puts the type tag in the byte Roblox reads as something
// else - lua_newthread appears to succeed, then luaL_sandboxthread faults the
// moment a table built that way is used.
// 02c37bc reordered the GC object header: memcat now leads and tt/marked shift
// up one. Verified in IDA, luaB_getmetatable (0x269ED80), which inlines
// luaC_threadbarrier:
//     v4 = *(BYTE*)(L + 2);
//     if ((v4 & 4) != 0) {            // isblack: marked & (1 << BLACKBIT)
//         *(BYTE*)(L + 2) = v4 & 0xFB;          // black2gray: clear that bit
//         *(QWORD*)(L + 0x10) = *(QWORD*)(g + 0x40);   // linkgclist -> grayagain
//         *(QWORD*)(g + 0x40) = L;
//     }
// so `marked` is at +2 and `tt` at +1. Getting this wrong silently corrupts
// every ttype()/GC-colour test in the VM rather than failing loudly.
// 742 (cec3ad5889b447cf): back to tt, marked, memcat. Verified in IDA - every
// GC allocation site (e.g. the table constructor at 0x258CCA7) does
//   mov byte [o], 7 ; movzx ecx, byte [g] ; and cl, 3 ; mov [o+1], cl
//   movzx ebp, byte [L+4] ; mov [o+2], bpl
// i.e. tt +0, marked = currentwhite +1, memcat = L->activememcat +2; and the
// inlined thread barrier tests `byte [L+1] & 4`.
// 743 (7a0c6d10fcb34a2c): tt, memcat, marked per the Achilles 743 dump (the dump
// detects this per build - 741 had memcat first). Not yet checked in IDA.
// 744 (0.744.0.7441077): marked, memcat, tt. Verified in IDA - lua_newstate
// writes marked=white at L+0 and tt=LUA_TTHREAD at L+2, every closure
// constructor writes `byte [cl+2], 8`, and luaC_traversestack tests
// `byte [uv], 3` (iswhite) on open upvalues.
#define CommonHeader \
    uint8_t marked; /* offset 0 */ \
    uint8_t memcat; /* offset 1 */ \
    uint8_t tt; /* offset 2 */
// clang-format on

/*
** Common header in struct form
*/
typedef struct GCheader
{
    CommonHeader;
} GCheader;

/*
** Union of all Lua values
*/
typedef union
{
    GCObject* gc;
    void* p;
    double n;
    int b;
    int64_t l;
    float v[2]; // v[0], v[1] live here; v[2] lives in TValue::extra
} Value;

/*
** Tagged Values
*/

typedef struct lua_TValue
{
    Value value;
    int extra[LUA_EXTRA_SIZE];
    int tt;
} TValue;

#if LUA_VECTOR_SIZE == 4
#define condvector4(vec4expr, vec3expr) vec4expr
#else
#define condvector4(vec4expr, vec3expr) vec3expr
#endif

#if LUA_VECTOR_DOUBLE == 1
#define condvectordouble(vecdoubleexpr, vecfloatexpr) vecdoubleexpr
#else
#define condvectordouble(vecdoubleexpr, vecfloatexpr) vecfloatexpr
#endif

// Macros to test type
#define ttisnil(o) (ttype(o) == LUA_TNIL)
#define ttisnumber(o) (ttype(o) == LUA_TNUMBER)
#define ttisinteger(o) (ttype(o) == LUA_TINTEGER)
#define ttisstring(o) (ttype(o) == LUA_TSTRING)
#define ttistable(o) (ttype(o) == LUA_TTABLE)
#define ttisfunction(o) (ttype(o) == LUA_TFUNCTION)
#define ttisboolean(o) (ttype(o) == LUA_TBOOLEAN)
#define ttisuserdata(o) (ttype(o) == LUA_TUSERDATA)
#define ttisthread(o) (ttype(o) == LUA_TTHREAD)
#define ttisbuffer(o) (ttype(o) == LUA_TBUFFER)
#define ttislightuserdata(o) (ttype(o) == LUA_TLIGHTUSERDATA)
#define ttisvector(o) (ttype(o) == LUA_TVECTOR)
#define ttisupval(o) (ttype(o) == LUA_TUPVAL)
#define ttisclass(o) (ttype(o) == LUA_TCLASS)
#define ttisobject(o) (ttype(o) == LUA_TOBJECT)

// Macros to access values
#define ttype(o) ((o)->tt)
#define gcvalue(o) check_exp(iscollectable(o), (o)->value.gc)
#define pvalue(o) check_exp(ttislightuserdata(o), (o)->value.p)
#define nvalue(o) check_exp(ttisnumber(o), (o)->value.n)
#define lvalue(o) check_exp(ttisinteger(o), (o)->value.l)
#define vvalue(o) check_exp(ttisvector(o), condvectordouble((o)->value.gc->vec.v, (o)->value.v))
#define tsvalue(o) check_exp(ttisstring(o), &(o)->value.gc->ts)
#define uvalue(o) check_exp(ttisuserdata(o), &(o)->value.gc->u)
#define clvalue(o) check_exp(ttisfunction(o), &(o)->value.gc->cl)
#define hvalue(o) check_exp(ttistable(o), &(o)->value.gc->h)
#define bvalue(o) check_exp(ttisboolean(o), (o)->value.b)
#define thvalue(o) check_exp(ttisthread(o), &(o)->value.gc->th)
#define bufvalue(o) check_exp(ttisbuffer(o), &(o)->value.gc->buf)
#define upvalue(o) check_exp(ttisupval(o), &(o)->value.gc->uv)
#define classvalue(o) check_exp(ttisclass(o), &(o)->value.gc->lclass)
#define objectvalue(o) check_exp(ttisobject(o), &(o)->value.gc->lobject)

#define l_isfalse(o) (ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))

#define lightuserdatatag(o) check_exp(ttislightuserdata(o), (o)->extra[0])

// Internal tags used by the VM
#define LU_TAG_ITERATOR LUA_UTAG_LIMIT

/*
** for internal debug only
*/
#define checkconsistency(obj) LUAU_ASSERT(!iscollectable(obj) || (ttype(obj) == (obj)->value.gc->gch.tt))

#define checkliveness(g, obj) LUAU_ASSERT(!iscollectable(obj) || ((ttype(obj) == (obj)->value.gc->gch.tt) && !isdead(g, (obj)->value.gc)))

// Macros to set values
#define setnilvalue(obj) ((obj)->tt = LUA_TNIL)

#define setnvalue(obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.n = (x); \
        i_o->tt = LUA_TNUMBER; \
    }

#define setlvalue(obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.l = (x); \
        i_o->tt = LUA_TINTEGER; \
    }

#if LUA_VECTOR_DOUBLE == 1
#define setvvalue(L, obj, x, y, z, w) \
    { \
        TValue* i_o = (obj); \
        LuauVector* i_vec = luaVec_newvector((L), double(x), double(y), double(z), condvector4(double(w), 0)); \
        i_o->value.gc = cast_to(GCObject*, i_vec); \
        i_o->tt = LUA_TVECTOR; \
        checkliveness((L)->global, i_o); \
    }
#else
#define setvvalue(L, obj, x, y, z, w) \
    { \
        TValue* i_o = (obj); \
        float* i_v = i_o->value.v; \
        i_v[0] = float(x); \
        i_v[1] = float(y); \
        i_v[2] = float(z); \
        condvector4(i_v[3] = float(w), (void)(w)); \
        i_o->tt = LUA_TVECTOR; \
    }
#endif

#define setpvalue(obj, x, tag) \
    { \
        TValue* i_o = (obj); \
        i_o->value.p = (x); \
        i_o->extra[0] = (tag); \
        i_o->tt = LUA_TLIGHTUSERDATA; \
    }

#define setbvalue(obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.b = (x); \
        i_o->tt = LUA_TBOOLEAN; \
    }

#define setsvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TSTRING; \
        checkliveness(L->global, i_o); \
    }

#define setuvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TUSERDATA; \
        checkliveness(L->global, i_o); \
    }

#define setthvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TTHREAD; \
        checkliveness(L->global, i_o); \
    }

#define setbufvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TBUFFER; \
        checkliveness(L->global, i_o); \
    }

#define setclvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TFUNCTION; \
        checkliveness(L->global, i_o); \
    }

#define sethvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TTABLE; \
        checkliveness(L->global, i_o); \
    }

#define setptvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TPROTO; \
        checkliveness(L->global, i_o); \
    }

#define setupvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TUPVAL; \
        checkliveness(L->global, i_o); \
    }

#define setobj(L, obj1, obj2) \
    { \
        const TValue* o2 = (obj2); \
        TValue* o1 = (obj1); \
        *o1 = *o2; \
        checkliveness(L->global, o1); \
    }

#define setclassvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TCLASS; \
        checkliveness(L->global, i_o); \
    }


#define setobjectvalue(L, obj, x) \
    { \
        TValue* i_o = (obj); \
        i_o->value.gc = cast_to(GCObject*, (x)); \
        i_o->tt = LUA_TOBJECT; \
        checkliveness(L->global, i_o); \
    }

/*
** different types of sets, according to destination
*/

// to stack
#define setobj2s setobj
// from table to same table (no barrier)
#define setobjt2t setobj
// to table (needs barrier)
#define setobj2t setobj
// to new object (no barrier)
#define setobj2n setobj
// to class instance or static member (needs barrier)
#define setobj2class setobj

#define setttype(obj, tt) (ttype(obj) = (tt))

#define iscollectable(o) (ttype(o) >= LUA_TSTRING)

// wrapper for returning userdata properties directly
struct DirectFieldResult
{
    lua_State* L;
    TValue* slot;
};

typedef TValue* StkId; // index to stack elements

/*
** String headers for string table
*/
struct TString
{
    CommonHeader;                                   /* 0x00 */
    unsigned char gap_03[0x1];                      /* 0x03 */
    uint16_t atomflag;                              /* 0x04 */
    int16_t atom;                                   /* 0x06 */
    TString* next;                                  /* 0x08 */
    TSTRING_HASH_ENC<unsigned int> hash;            /* 0x10 */
    unsigned int len;                               /* 0x14 */
    char data[1];                                   /* 0x18 */
};


#define getstr(ts) (ts)->data
#define svalue(o) getstr(tsvalue(o))

typedef struct TKey
{
    ::Value value;
    int extra[LUA_EXTRA_SIZE];
    unsigned tt : 4;
    int next : 28; // for chaining
} TKey;

typedef struct LuaNode
{
    TValue val;
    TKey key;
} LuaNode;

// 0.739 byte fields verified in IDA (luau_execute), NOT taken from the struct
// dump - the dump has tmcache and safeenv swapped:
//   +0x03 safeenv   : GETIMPORT/GETGLOBAL test `byte [cl->env+3], 0`
//   +0x05 tmcache   : fasttm tests `byte [mt+5], 1<<event`, and luaH_getstr
//                     sets `or byte [h+5], 1` on a __index miss
//   +0x06 lsizenode : node lookup does `1 << byte [h+6]`
//   +0x07 readonly  : SETTABLEKS fast path checks `byte [h+7], 0` before writing
// 0.740: the node/array/gclist tail moved relative to 739 (node 0x20 -> 0x18,
// array 0x28 -> 0x20, gclist 0x18 -> 0x28). The tmcache/safeenv swap is still
// there: verified at runtime 2026-09-29 - clearing +0x05 left FASTCALL active,
// Roblox's own getfenv turned it off, and +0x05 cannot be readonly (sandboxed
// scripts write their globals fine with it set).
// 02c37bc: re-verified in IDA for this build, so the 740 notes above no longer
// apply - both the byte block and the metatable/gclist pair moved.
//   luaB_next (0x26A4530) does `1 << byte [h+4]`        -> lsizenode 0x04
//                         reads `[h+8]`                 -> sizearray 0x08
//                         reads `[h+18h]` / `[h+20h]`   -> node 0x18 / array 0x20
//   luaB_getmetatable (0x269ED80) LUA_TTABLE case reads `[h+28h]`
//                                                       -> metatable 0x28
// so metatable and gclist are swapped relative to 740 (metatable 0x10 -> 0x28,
// gclist 0x28 -> 0x10), and the byte order matches this build's struct dump.
// 742 (cec3ad5889b447cf): byte block and tail moved again; matches the Achilles
// 742 dump. Verified in IDA for this build:
//   luaB_next (0x25ED9A0)      `1 << byte [h+5]`, `[h+10h]` -> lsizenode 0x05, node 0x10
//   luaB_getmetatable (0x25E81F0) LUA_TTABLE reads `[h+20h]` -> metatable 0x20
//   luaB_setfenv/getfenv       clear `byte [h+7]`            -> safeenv 0x07
//   table ctor (0x258CCA7)     stores dummynode, mt at +0x20
// 743 (7a0c6d10fcb34a2c): byte block and tail moved again; taken from the
// Achilles 743 dump, not yet checked in IDA.
// 744 (0.744.0.7441077): verified in IDA. The 744 dumps disagree on the
// safeenv/readonly pair (struct(1).hpp has them swapped); the binary says:
//   every luaG_readonlyerror (0x26B6D60) site tests `byte [h+7], 0` -> readonly 0x07
//   the API setter at 0x268D990 writes `byte [h+4]`                 -> safeenv 0x04
//   lua_unref's registry lookup: `1 << byte [h+6]`, `[h+20h]` node,
//   `[h+18h]` array, `[h+8]` sizearray; traversestack gclist `[h+28h]`.
struct LuaTable
{
    CommonHeader;                                   /* 0x00 */
    unsigned char tmcache;                          /* 0x03 */
    unsigned char safeenv;                          /* 0x04 */
    unsigned char nodemask8;                        /* 0x05 */
    unsigned char lsizenode;                        /* 0x06 */
    unsigned char readonly;                         /* 0x07 */
    int sizearray;                                  /* 0x08 */
    union                                           /* 0x0C */
    {
        int lastfree;
        int aboundary;
    };
    LuaTable* metatable;                            /* 0x10 */
    TValue* array;                                  /* 0x18 */
    LuaNode* node;                                  /* 0x20 */
    GCObject* gclist;                               /* 0x28 */
};
// (asserts for this struct live in the consolidated block further down)

struct Udata
{
    CommonHeader; /* offset 0 */
    unsigned char tag; /* offset 3 */
    int len; /* offset 4 */
    UDATA_META_ENC<struct LuaTable*> metatable; /* offset 8 */
    char data[1]; /* offset 16 */
};

typedef struct LuauBuffer
{
    CommonHeader;

    unsigned int len;

    alignas(8) char data[1];
} Buffer;

typedef struct LuauVector
{
    CommonHeader;
    // 1 byte padding

    LUA_VECTOR_TYPE v[LUA_VECTOR_SIZE];
} LuauVector;

enum FeedbackVectorSlotKind
{
    CALL_TARGET,
    CONSTRUCT,
};

enum LuauClassConstructMatch
{
    LCM_MISMATCH,
    LCM_MATCH,
    LCM_MATCH_ZEROED,
};

struct FeedbackVectorSlot
{
    FeedbackVectorSlotKind kind;

    union
    {
        struct
        {
            uint32_t pc;
            uint32_t proto;
            uint32_t hits;
        } call_target;

        // 743 added the CONSTRUCT slot. Both arms are 12 bytes, so the
        // union - and the slot - keep the size the client allocates.
        struct
        {
            uint32_t shape;
            uint32_t classid;
            LuauClassConstructMatch match;
        } construct;
    };
};

/*
** Function Prototypes
*/
// LocVar is defined further down, but Proto::locvars needs the name here.
// Without this the template argument fails to parse and every later use of
// Closure/Proto reports bogus "is not a member" errors.
struct LocVar;

// clang-format off
struct Proto
{
    // 02c37bc layout. Reordered again against 740 - the pointer block and the
    // whole int tail both moved, so none of the 740 notes carry over. Deltas:
    //   flags      0x06 -> 0x03    is_vararg  0x03 -> 0x04
    //   numparams  0x04 -> 0x05    nups       0x05 -> 0x06
    //   source     0x18 -> 0x08    locvars    0x58 -> 0x10
    //   lineinfo   0x10 -> 0x18    debugname  0x60 -> 0x20
    //   gclist     0x78 -> 0x30    userdata   0x50 -> 0x38
    //   typeinfo   0x08 -> 0x40    k          0x40 -> 0x48
    //   code       0x48 -> 0x50    debuginsn  0x68 -> 0x58
    //   execdata   0x30 -> 0x60    exectarget 0x38 -> 0x68
    //   abslineinfo 0x20 -> 0x70   codeentry  0x80 -> 0x78
    //   upvalues   0x70 -> 0x80
    //
    // 742 (cec3ad5889b447cf): reordered again; matches the Achilles 742 dump.
    // Verified in IDA for this build:
    //   luau_execute entry: `mov r10, [p+20h]`            -> k 0x20
    //   LOP_BREAK: `sub rax, [p+28h]` / debuginsn at p+0x80 (VMValue2)
    //                                                     -> code 0x28, debuginsn 0x80
    //   capability read (0x4185DD0): `p+0x40`, XOR decode -> userdata 0x40 (VMValue3)
    //
    // 743 (7a0c6d10fcb34a2c): reordered again; taken from the Achilles 743 dump,
    // not yet checked in IDA. k / code / userdata kept their 742 offsets.
    // 744 (0.744.0.7441077): close to upstream order again; all three 744
    // dumps agree. Verified in IDA: luaF_getlocal reads locvars `[p+20h]`
    // (VMValue1, 0x18-byte LocVar) and sizelocvars `[p+0ACh]`; luaG_getline
    // reads lineinfo `[p+40h]` (XOR, VMValue3), abslineinfo `p+58h - [p+58h]`
    // (VMValue2), linegaplog2 `byte [p+9Ch]`; luaC_traverseproto reads source
    // `[p+8]` (VMValue1), upvalues `[p+10h]` (XOR), debugname `[p+60h]`, and
    // p `[p+30h]` raw (the Hyperion dump's PROTO_P_ENC is plain this build);
    // luaF_newproto stores debuginsn as `-&field` (VMValue4); luau_execute
    // loads k `[p+48h]` / code `[p+50h]`.
    CommonHeader;                                   /* 0x00 */
    unsigned char nups;                             /* 0x03 */
    unsigned char is_vararg;                        /* 0x04 */
    unsigned char maxstacksize;                     /* 0x05 */
    unsigned char numparams;                        /* 0x06 */
    unsigned char flags;                            /* 0x07 */
    PROTO_SOURCE_ENC<TString*> source;              /* 0x08 */
    PROTO_UPVALUES_ENC<TString**> upvalues;         /* 0x10 */
    GCObject* gclist;                               /* 0x18 */
    PROTO_LOCVARS_ENC<struct LocVar*> locvars;      /* 0x20 */
    unsigned int* codeentry;                        /* 0x28 */
    Proto** p;                                      /* 0x30 */
    PROTO_USERDATA_ENC<void*> userdata;             /* 0x38 */
    PROTO_LINEINFO_ENC<unsigned char*> lineinfo;    /* 0x40 */
    TValue* k;                                      /* 0x48 */
    unsigned int* code;                             /* 0x50 */
    PROTO_ABSLINEINFO_ENC<int*> abslineinfo;        /* 0x58 */
    PROTO_DEBUGNAME_ENC<TString*> debugname;        /* 0x60 */
    PROTO_DEBUGINSN_ENC<unsigned char*> debuginsn;  /* 0x68 */
    void* execdata;                                 /* 0x70 */
    uintptr_t exectarget;                           /* 0x78 */
    PROTO_TYPEINFO_ENC<unsigned char*> typeinfo;    /* 0x80 */
    int sizetypeinfo;                               /* 0x88 */
    int sizeupvalues;                               /* 0x8C */
    int linedefined;                                /* 0x90 */
    int sizek;                                      /* 0x94 */
    int bytecodeid;                                 /* 0x98 */
    int linegaplog2;                                /* 0x9C */
    int sizelineinfo;                               /* 0xA0 */
    int sizecode;                                   /* 0xA4 */
    int sizep;                                      /* 0xA8 */
    int sizelocvars;                                /* 0xAC */
    struct FeedbackVectorSlot* feedbackvec;         /* 0xB0 */
    int feedbackvecsize;                            /* 0xB8 */
    int funid;                                      /* 0xBC */
    Proto* optimized;                               /* 0xC0 */
    Proto* deoptimized;                             /* 0xC8 */
    uint64_t cost;                                  /* 0xD0 */
};
// clang-format on

// (asserts for this struct live in the consolidated block further down)

typedef struct LocVar
{
    TString* varname;
    int startpc; // first point where variable is active
    int endpc;   // first point where variable is dead
    uint8_t reg; // register slot, relative to base, where variable is stored
} LocVar;

/*
** Upvalues
*/

typedef struct UpVal
{
    CommonHeader;
    uint8_t markedopen; // set if reachable from an alive thread (only valid during atomic)

    // 4 byte padding (x64)

    TValue* v; // points to stack or to its own value
    union
    {
        TValue value; // the value (when closed)
        struct
        {
            // global double linked list (when open)
            struct UpVal* prev;
            struct UpVal* next;

            // thread linked list (when open)
            struct UpVal* threadnext;
        } open;
    } u;
} UpVal;

#define upisopen(up) ((up)->v != &(up)->u.value)

/*
** Closures
*/

struct Closure
{
    // 02c37bc: preload and isC swapped back (preload 0x03, isC 0x06). Verified in
    // IDA - luaB_getfenv (0x269F940) takes the LUA_TFUNCTION branch on
    // `byte [cl+6]`, so isC is at 0x06 on this build.
    // 742: isC back at 0x03 (luaB_getfenv 0x25E8DA0 tests `byte [cl+3]`), and the
    // C union is (f, cont, debugname) again; matches the Achilles 742 dump.
    // 743: nupvalues/stacksize swapped and the C union is (f, debugname, cont);
    // from the Achilles 743 dump, not yet checked in IDA.
    // 744: C union is (debugname, cont, f). Verified in IDA - luau_execute's
    // C-call path does `mov rax, [cl+28h]` / `mov rcx, L` / icall, and
    // lua_pushcclosurek (0x269DC..) stores cont as XOR at +0x20 and the
    // interned debugname raw at +0x18 (the Hyperion dump's
    // CLOSURE_DEBUGNAME_ENC is plain this build).
    CommonHeader;                                   /* 0x00 */
    unsigned char isC;                              /* 0x03 */
    unsigned char preload;                          /* 0x04 */
    unsigned char nupvalues;                        /* 0x05 */
    unsigned char stacksize;                        /* 0x06 */
    unsigned char gap_07[0x1];                      /* 0x07 */
    GCObject* gclist;                               /* 0x08 */
    struct LuaTable* env;                           /* 0x10 */
    union                                           /* 0x18 */
    {
        struct
        {
            struct Proto* p;                        /* 0x18 */
            TValue uprefs[1];                       /* 0x20 */
        } l;
        // The C-closure order REVERSED against 740: it was (f, cont, debugname),
        // it is now (debugname, cont, f). Environment.cpp swaps c.f on the game's
        // __index/__namecall at setup, so if inject starts crashing immediately
        // after a client update, re-check this union before anything else.
        struct
        {
            TString* debugname;                     /* 0x18 */
            CLOSURE_CONT_ENC<lua_Continuation> cont;/* 0x20 */
            lua_CFunction f;                        /* 0x28 */
            TValue upvals[1];                       /* 0x30 */
        } c;
    };
};
// (asserts for this struct live in the consolidated block further down)

#define iscfunction(o) (ttype(o) == LUA_TFUNCTION && clvalue(o)->isC)
#define isLfunction(o) (ttype(o) == LUA_TFUNCTION && !clvalue(o)->isC)

/*
** Tables
*/

// copy a value into a key
#define setnodekey(L, node, obj) \
    { \
        LuaNode* n_ = (node); \
        const TValue* i_o = (obj); \
        n_->key.value = i_o->value; \
        memcpy(n_->key.extra, i_o->extra, sizeof(n_->key.extra)); \
        n_->key.tt = i_o->tt; \
        checkliveness(L->global, i_o); \
    }

// copy a value from a key
#define getnodekey(L, obj, node) \
    { \
        TValue* i_o = (obj); \
        const LuaNode* n_ = (node); \
        i_o->value = n_->key.value; \
        memcpy(i_o->extra, n_->key.extra, sizeof(i_o->extra)); \
        i_o->tt = n_->key.tt; \
        checkliveness(L->global, i_o); \
    }

// clang-format off
// clang-format on

typedef struct LuauClass
{
    CommonHeader;

    GCObject* gclist;

    TString* name;

    // Used to match feedback vector construction slots.
    // Handed out from global_State::lastclassid (0x4AF4 in the 743 dump).
    uint32_t id;

    // The superclass of this class. NULL if this class doesn't inherit.
    LuauClass* super;

    // Mapping from offset to static members (only methods for now).
    TValue* staticmembers;

    // Mapping from member name to offset.
    // For static members, subtracting numberofinstancemembers from the offset gives the actual index into staticmembers.
    LuaTable* memberstooffset;

    // Mapping from offset to member name. Instance member offsets are stored before static member offsets.
    TString** offsettomember;

    // Metatable for this *class object*. At time of writing this only contains
    // __call, but we may add more metamethods to class objects in the future.
    LuaTable* metatable;

    // Metatable for instances of this class. NULL until the first metamethod
    // is added via luaR_addclassmember.
    LuaTable* instancemetatable;

    // Number of instance members that we expect instances of this class object
    // to have.
    uint32_t numberofinstancemembers;

    // Total number of members that we expect this class object to have between
    // instance and static members.
    //
    // We store this number as an optimization. It's pretty rare that we need
    // to reference the specific number of static members, but it's very common
    // to reference the total number of members (for validating hot paths in
    // the interpreter) and the number of instance members (branching on
    // instance or static members, creating class instances).
    uint32_t numberofallmembers;

    // Can this class be extended?
    bool isopen;

    // True if this class or any of its ancestors defines an __init method.
    // The default constructor errors if this is true, which works because
    // the default constructor is overridden if a class defines __init.
    bool hasuserinitinchain;
} LuauClass;

typedef struct LuauObject
{
    CommonHeader;

    GCObject* gclist;

    // The class object that this value is an instance of.
    LuauClass* lclass;

    // The number of members that this instance contains. We need this in order
    // to free ourselves if we got swept in the same GC cycle as our class
    // pointer.
    uint32_t numberofmembers;

    // The fields of this instance.
    TValue* members;

} LuauObject;

/*
** `module' operation for hashing (size is always a power of 2)
*/
#define lmod(s, size) (check_exp((size & (size - 1)) == 0, (cast_to(int, (s) & ((size) - 1)))))

#define twoto(x) ((int)(1 << (x)))
#define sizenode(t) (twoto((t)->lsizenode))

// Metamethod cache: when readonly bit 1 is set the table carries a cache of
// TM_* slots laid out *below* array[0], so getmetacache indexes backwards.
// Pure accessors over existing LuaTable fields - no layout impact.
#define hasmetacache(t) (((t)->readonly & 2) != 0)
#define getmetacache(t, event) ((t)->array - (1 + (event)))

#define luaO_nilobject reinterpret_cast<TValue*>(Offsets::Luau::LuaO_NilObject)

LUAI_DATA const TValue luaO_nilobject_;

#define ceillog2(x) (luaO_log2((x) - 1) + 1)

LUAI_FUNC int luaO_log2(unsigned int x);
LUAI_FUNC int luaO_rawequalObj(const TValue* t1, const TValue* t2);
LUAI_FUNC int luaO_rawequalKey(const TKey* t1, const TValue* t2);
LUAI_FUNC int luaO_str2d(const char* s, double* result);
LUAI_FUNC int luaO_str2l(const char* s, int64_t* result, int base = 10);
LUAI_FUNC const char* luaO_pushvfstring(lua_State* L, const char* fmt, va_list argp);
LUAI_FUNC const char* luaO_pushfstring(lua_State* L, const char* fmt, ...);
LUAI_FUNC const char* luaO_chunkid(char* buf, size_t buflen, const char* source, size_t srclen);


// Size guards for the member types that global_State / LuaTable are built from.
// Three separate 738 bugs (RobloxExtraSpace, lua_Callbacks, stringtable) were
// member types inherited from the 737 headers rather than re-derived, and two
// of them had the right sizeof so the offsetof asserts never fired. Pin sizes
// as well as offsets.
//
// TValue 16 - global_State has pseudotemp at 0x608 and registry at 0x618 (739).
// LuaNode 32 - TValue + TKey; LuaTable::node is indexed by this stride, so a
// wrong size sends every hash lookup to the wrong slot.
static_assert(sizeof(TValue) == 0x10, "TValue must be 16 bytes");
static_assert(sizeof(TKey) == 0x10, "TKey must be 16 bytes");
static_assert(sizeof(LuaNode) == 0x20, "LuaNode must be 32 bytes");
static_assert(offsetof(LuaNode, val) == 0x0, "LuaNode::val moved");
static_assert(offsetof(LuaNode, key) == 0x10, "LuaNode::key moved");

// LuaTable layout, 02c37bc, verified in IDA for this build:
//   luaB_next (0x26A4530): `1 << byte [h+4]`      -> lsizenode 0x04
//                          `[h+8]`                -> sizearray 0x08
//                          `[h+18h]` / `[h+20h]`  -> node 0x18, array 0x20
//   luaB_getmetatable (0x269ED80) LUA_TTABLE case reads `[h+28h]`
//                                                 -> metatable 0x28
// 743 values from the Achilles 743 dump; see the struct comments above.
static_assert(offsetof(GCheader, tt) == 0x2, "CommonHeader::tt moved");
static_assert(offsetof(GCheader, marked) == 0x0, "CommonHeader::marked moved");
static_assert(offsetof(GCheader, memcat) == 0x1, "CommonHeader::memcat moved");

static_assert(offsetof(LuaTable, tmcache) == 0x3, "LuaTable::tmcache moved");
static_assert(offsetof(LuaTable, readonly) == 0x7, "LuaTable::readonly moved");
static_assert(offsetof(LuaTable, lsizenode) == 0x6, "LuaTable::lsizenode moved");
static_assert(offsetof(LuaTable, nodemask8) == 0x5, "LuaTable::nodemask8 moved");
static_assert(offsetof(LuaTable, safeenv) == 0x4, "LuaTable::safeenv moved");
static_assert(offsetof(LuaTable, sizearray) == 0x8, "LuaTable::sizearray moved");
static_assert(offsetof(LuaTable, node) == 0x20, "LuaTable::node moved");
static_assert(offsetof(LuaTable, array) == 0x18, "LuaTable::array moved");
static_assert(offsetof(LuaTable, metatable) == 0x10, "LuaTable::metatable moved");
static_assert(offsetof(LuaTable, gclist) == 0x28, "LuaTable::gclist moved");
static_assert(sizeof(LuaTable) == 0x30, "LuaTable must be 48 bytes");

static_assert(sizeof(Udata) == 0x18, "Udata must be 24 bytes");

static_assert(offsetof(Proto, locvars) == 0x20, "Proto::locvars moved");
static_assert(offsetof(Proto, k) == 0x48, "Proto::k moved");
static_assert(offsetof(Proto, code) == 0x50, "Proto::code moved");
static_assert(offsetof(Proto, source) == 0x08, "Proto::source moved");
static_assert(offsetof(Proto, userdata) == 0x38, "Proto::userdata moved");
static_assert(offsetof(Proto, p) == 0x30, "Proto::p moved");
static_assert(offsetof(Proto, upvalues) == 0x10, "Proto::upvalues moved");
static_assert(offsetof(Proto, debuginsn) == 0x68, "Proto::debuginsn moved");
static_assert(offsetof(Proto, lineinfo) == 0x40, "Proto::lineinfo moved");
static_assert(offsetof(Proto, abslineinfo) == 0x58, "Proto::abslineinfo moved");
static_assert(offsetof(Proto, debugname) == 0x60, "Proto::debugname moved");
static_assert(offsetof(Proto, linegaplog2) == 0x9C, "Proto::linegaplog2 moved");
static_assert(offsetof(Proto, sizelocvars) == 0xAC, "Proto::sizelocvars moved");
static_assert(offsetof(Proto, sizecode) == 0xA4, "Proto::sizecode moved");
static_assert(offsetof(Proto, sizek) == 0x94, "Proto::sizek moved");
static_assert(sizeof(Proto) == 0xD8, "Proto must be 216 bytes");

static_assert(offsetof(Closure, isC) == 0x3, "Closure::isC moved");
static_assert(offsetof(Closure, nupvalues) == 0x5, "Closure::nupvalues moved");
static_assert(offsetof(Closure, env) == 0x10, "Closure::env moved");
static_assert(offsetof(Closure, l.p) == 0x18, "Closure::l.p moved");
static_assert(offsetof(Closure, c.f) == 0x28, "Closure::c.f moved");
static_assert(offsetof(Closure, c.debugname) == 0x18, "Closure::c.debugname moved");
static_assert(offsetof(Closure, c.cont) == 0x20, "Closure::c.cont moved");
static_assert(sizeof(Closure) == 0x40, "Closure must be 64 bytes");

// TString, measured from global->tmname[0] = Roblox's own "__index":
//   +0x14 len = 7 and +0x18 data = "__index"
static_assert(offsetof(TString, next) == 0x8, "TString::next moved");
static_assert(offsetof(TString, hash) == 0x10, "TString::hash moved");
static_assert(offsetof(TString, len) == 0x14, "TString::len moved");
static_assert(offsetof(TString, data) == 0x18, "TString::data moved");