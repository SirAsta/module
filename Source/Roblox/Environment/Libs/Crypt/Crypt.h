#pragma once
#include "lua.h"
#include "lualib.h"
#include "lobject.h"
#include "lstate.h"
#include "lapi.h"
#include "ltable.h"
#include <string>
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <wincrypt.h>
#include <bcrypt.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Bcrypt.lib")

namespace sha3_impl {
    static const uint64_t RC[24] = {
        0x0000000000000001ULL,0x0000000000008082ULL,0x800000000000808aULL,
        0x8000000080008000ULL,0x000000000000808bULL,0x0000000080000001ULL,
        0x8000000080008081ULL,0x8000000000008009ULL,0x000000000000008aULL,
        0x0000000000000088ULL,0x0000000080008009ULL,0x000000008000000aULL,
        0x000000008000808bULL,0x800000000000008bULL,0x8000000000008089ULL,
        0x8000000000008003ULL,0x8000000000008002ULL,0x8000000000000080ULL,
        0x000000000000800aULL,0x800000008000000aULL,0x8000000080008081ULL,
        0x8000000000008080ULL,0x0000000080000001ULL,0x8000000080008008ULL
    };
    static inline uint64_t rol64(uint64_t x, int n) { return (x << n) | (x >> (64 - n)); }
    static void keccakf(uint64_t s[25]) {
        static const int pi[24] = { 10,7,11,17,18,3,5,16,8,21,24,4,15,23,19,13,12,2,20,14,22,9,6,1 };
        static const int ro[24] = { 1,3,6,10,15,21,28,36,45,55,2,14,27,41,56,8,25,43,62,18,39,61,20,44 };
        for (int r = 0; r < 24; r++) {
            uint64_t bc[5], t, tmp;
            for (int i = 0; i < 5; i++) bc[i] = s[i] ^ s[i+5] ^ s[i+10] ^ s[i+15] ^ s[i+20];
            for (int i = 0; i < 5; i++) { t = bc[(i+4)%5] ^ rol64(bc[(i+1)%5], 1); for (int j = 0; j < 25; j += 5) s[j+i] ^= t; }
            t = s[1];
            for (int i = 0; i < 24; i++) { tmp = s[pi[i]]; s[pi[i]] = rol64(t, ro[i]); t = tmp; }
            for (int j = 0; j < 25; j += 5) { uint64_t t0=s[j],t1=s[j+1],t2=s[j+2],t3=s[j+3],t4=s[j+4]; s[j]=t0^(~t1&t2); s[j+1]=t1^(~t2&t3); s[j+2]=t2^(~t3&t4); s[j+3]=t3^(~t4&t0); s[j+4]=t4^(~t0&t1); }
            s[0] ^= RC[r];
        }
    }
    inline std::string sha3(int bits, const char* data, size_t len) {
        int rate = (1600 - bits * 2) / 8;
        uint64_t st[25] = {}; uint8_t* sb = reinterpret_cast<uint8_t*>(st); size_t off = 0;
        while (off < len) { size_t blk = (len-off < (size_t)rate) ? (len-off) : (size_t)rate; for (size_t i = 0; i < blk; i++) sb[i] ^= ((const uint8_t*)data)[off+i]; off += blk; if (blk == (size_t)rate) keccakf(st); }
        sb[off % rate] ^= 0x06; sb[rate-1] ^= 0x80; keccakf(st);
        int outb = bits / 8; static constexpr char hx[] = "0123456789abcdef"; std::string out; out.reserve(outb*2);
        for (int i = 0; i < outb; i++) { out += hx[sb[i]>>4]; out += hx[sb[i]&0xf]; } return out;
    }
}

namespace sha384_impl {
    static const uint64_t K[80] = {
        0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
        0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
        0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL,
    };
    inline uint64_t rotr64(uint64_t x, int n) { return (x >> n) | (x << (64 - n)); }
    inline std::string sha384(const std::string& msg) {
        uint64_t h[8] = { 0xcbbb9d5dc1059ed8ULL,0x629a292a367cd507ULL,0x9159015a3070dd17ULL,0x152fecd8f70e5939ULL,0x67332667ffc00b31ULL,0x8eb44a8768581511ULL,0xdb0c2e0d64f98fa7ULL,0x47b5481dbefa4fa4ULL };
        uint64_t bitlen = (uint64_t)msg.size() * 8;
        std::vector<uint8_t> data(msg.begin(), msg.end()); data.push_back(0x80);
        while (data.size() % 128 != 112) data.push_back(0x00);
        for (int i = 7; i >= 0; --i) data.push_back(0);
        for (int i = 7; i >= 0; --i) data.push_back((uint8_t)(bitlen >> (i*8)));
        for (size_t chunk = 0; chunk < data.size(); chunk += 128) {
            uint64_t w[80] = {};
            for (int i = 0; i < 16; ++i) { w[i] = 0; for (int j = 0; j < 8; ++j) w[i] = (w[i] << 8) | data[chunk+i*8+j]; }
            for (int i = 16; i < 80; ++i) w[i] = (rotr64(w[i-2],19)^rotr64(w[i-2],61)^(w[i-2]>>6)) + w[i-7] + (rotr64(w[i-15],1)^rotr64(w[i-15],8)^(w[i-15]>>7)) + w[i-16];
            uint64_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
            for (int i = 0; i < 80; ++i) { uint64_t t1 = hh + (rotr64(e,14)^rotr64(e,18)^rotr64(e,41)) + ((e&f)^(~e&g)) + K[i] + w[i]; uint64_t t2 = (rotr64(a,28)^rotr64(a,34)^rotr64(a,39)) + ((a&b)^(a&c)^(b&c)); hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2; }
            h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
        }
        char buf[97]; snprintf(buf, sizeof(buf), "%016llx%016llx%016llx%016llx%016llx%016llx", (unsigned long long)h[0],(unsigned long long)h[1],(unsigned long long)h[2],(unsigned long long)h[3],(unsigned long long)h[4],(unsigned long long)h[5]);
        return std::string(buf);
    }
}

namespace crypt_lib {
    inline bool random_bytes(std::string& out, size_t size) {
        out.resize(size); HCRYPTPROV p = 0;
        if (!CryptAcquireContextW(&p, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return false;
        BOOL ok = CryptGenRandom(p, static_cast<DWORD>(size), reinterpret_cast<BYTE*>(out.data()));
        CryptReleaseContext(p, 0); return ok != FALSE;
    }

    inline bool cryptoapi_hash(ALG_ID alg, const char* data, size_t size, std::string& out) {
        HCRYPTPROV p = 0; HCRYPTHASH hh = 0;
        if (!CryptAcquireContextW(&p, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return false;
        bool ok = CryptCreateHash(p, alg, 0, 0, &hh) != FALSE;
        ok = ok && CryptHashData(hh, reinterpret_cast<const BYTE*>(data), static_cast<DWORD>(size), 0) != FALSE;
        DWORD hs = 0, hsl = sizeof(hs);
        ok = ok && CryptGetHashParam(hh, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hs), &hsl, 0) != FALSE;
        std::vector<BYTE> dig(hs);
        ok = ok && CryptGetHashParam(hh, HP_HASHVAL, dig.data(), &hs, 0) != FALSE;
        if (ok) { static constexpr char hex[] = "0123456789abcdef"; out.resize(hs * 2); for (DWORD i = 0; i < hs; ++i) { out[i*2] = hex[dig[i]>>4]; out[i*2+1] = hex[dig[i]&0x0f]; } }
        if (hh) CryptDestroyHash(hh); CryptReleaseContext(p, 0); return ok;
    }

    inline std::string base64_encode_bytes(const uint8_t* data, size_t len) {
        static const char* alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out; out.reserve(((len+2)/3)*4);
        for (size_t i = 0; i < len; i += 3) {
            uint32_t t = 0; int n = 0;
            t |= uint32_t(data[i]) << 16; n = 1;
            if (i+1 < len) { t |= uint32_t(data[i+1]) << 8; n = 2; }
            if (i+2 < len) { t |= uint32_t(data[i+2]); n = 3; }
            out.push_back(alpha[(t>>18)&0x3F]); out.push_back(alpha[(t>>12)&0x3F]);
            out.push_back(n >= 2 ? alpha[(t>>6)&0x3F] : '='); out.push_back(n >= 3 ? alpha[t&0x3F] : '=');
        }
        return out;
    }

    inline bool base64_decode_char(uint8_t& out, char c) {
        if (c>='A'&&c<='Z') { out=uint8_t(c-'A'); return true; } if (c>='a'&&c<='z') { out=uint8_t(c-'a'+26); return true; }
        if (c>='0'&&c<='9') { out=uint8_t(c-'0'+52); return true; } if (c=='+') { out=62; return true; } if (c=='/') { out=63; return true; } return false;
    }

    inline bool base64_decode_string(const std::string& in, std::string& out) {
        out.clear(); if (in.empty()) return true; if (in.size()%4!=0) return false;
        out.reserve((in.size()/4)*3);
        for (size_t i = 0; i < in.size(); i += 4) {
            uint8_t v[4]={}; int pad = 0;
            for (int k = 0; k < 4; k++) { char c = in[i+k]; if (c=='=') { v[k]=0; pad++; } else { if (!base64_decode_char(v[k],c)) return false; } }
            uint32_t t = (uint32_t(v[0])<<18)|(uint32_t(v[1])<<12)|(uint32_t(v[2])<<6)|uint32_t(v[3]);
            out.push_back(char((t>>16)&0xFF)); if (pad<2) out.push_back(char((t>>8)&0xFF)); if (pad<1) out.push_back(char(t&0xFF));
        }
        return true;
    }

    inline void lz4_write_len(std::vector<uint8_t>& out, size_t len) { while (len>=255) { out.push_back(255); len-=255; } out.push_back(uint8_t(len)); }

    inline std::string lz4_compress_block(const uint8_t* src, size_t srcSize) {
        const size_t HASH_SIZE = 1u << 16; std::vector<int> hash(HASH_SIZE, -1);
        auto read32 = [&](size_t pos) -> uint32_t { return uint32_t(src[pos])|(uint32_t(src[pos+1])<<8)|(uint32_t(src[pos+2])<<16)|(uint32_t(src[pos+3])<<24); };
        auto hash32 = [&](uint32_t v) -> size_t { return (v * 2654435761u) >> 16; };
        std::vector<uint8_t> out; out.reserve(srcSize + srcSize/8 + 16); size_t anchor = 0, i = 0;
        while (i+4 <= srcSize) {
            uint32_t seq = read32(i); size_t h = hash32(seq); int ref = hash[h]; hash[h] = int(i);
            bool match = false; size_t matchPos = 0;
            if (ref >= 0) { size_t r = size_t(ref); if (i > r && (i-r) <= 0xFFFF && r+4 <= srcSize && read32(r) == seq) { match = true; matchPos = r; } }
            if (!match) { i++; continue; }
            size_t litLen = i - anchor, mLen = 4;
            while (i+mLen < srcSize && matchPos+mLen < srcSize && src[i+mLen] == src[matchPos+mLen]) mLen++;
            uint8_t token = uint8_t(((std::min)(litLen,size_t(15))<<4) | (std::min)(mLen-4,size_t(15)));
            out.push_back(token); if (litLen>=15) lz4_write_len(out, litLen-15);
            out.insert(out.end(), src+anchor, src+i);
            uint16_t offset = uint16_t(i - matchPos); out.push_back(uint8_t(offset&0xFF)); out.push_back(uint8_t((offset>>8)&0xFF));
            if (mLen-4>=15) lz4_write_len(out, (mLen-4)-15);
            i += mLen; anchor = i;
        }
        if (anchor < srcSize) { size_t litLen = srcSize-anchor; out.push_back(uint8_t((std::min)(litLen,size_t(15))<<4)); if (litLen>=15) lz4_write_len(out, litLen-15); out.insert(out.end(), src+anchor, src+srcSize); }
        return std::string(reinterpret_cast<const char*>(out.data()), out.size());
    }

    inline bool lz4_read_len(const uint8_t* in, size_t inSize, size_t& ip, size_t& len) {
        size_t s = 0; while (true) { if (ip>=inSize) return false; uint8_t v=in[ip++]; s+=v; if (v!=255) break; } len+=s; return true;
    }

    inline bool lz4_decompress_block(const uint8_t* in, size_t inSize, std::string& out) {
        std::vector<uint8_t> dst; dst.reserve(inSize*3); size_t ip = 0;
        while (ip < inSize) {
            uint8_t token = in[ip++]; size_t litLen = (token>>4)&0xF, matchLen = (token&0xF);
            if (litLen==15) { if (!lz4_read_len(in,inSize,ip,litLen)) return false; }
            if (ip+litLen > inSize) return false;
            dst.insert(dst.end(), in+ip, in+ip+litLen); ip += litLen;
            if (ip >= inSize) break;
            if (ip+2 > inSize) return false;
            uint16_t offset = uint16_t(in[ip])|(uint16_t(in[ip+1])<<8); ip += 2;
            if (offset==0 || offset > dst.size()) return false;
            matchLen += 4; if ((token&0xF)==15) { if (!lz4_read_len(in,inSize,ip,matchLen)) return false; }
            size_t start = dst.size() - offset; for (size_t k = 0; k < matchLen; k++) dst.push_back(dst[start+k]);
        }
        out.assign(reinterpret_cast<const char*>(dst.data()), dst.size()); return true;
    }

    inline int base64encode(lua_State* L) { size_t len=0; const char* d=luaL_checklstring(L,1,&len); std::string o=base64_encode_bytes(reinterpret_cast<const uint8_t*>(d),len); lua_pushlstring(L,o.data(),o.size()); return 1; }
    inline int base64decode(lua_State* L) { size_t len=0; const char* d=luaL_checklstring(L,1,&len); std::string o; if(!base64_decode_string(std::string(d,len),o)) luaL_error(L,"invalid base64"); lua_pushlstring(L,o.data(),o.size()); return 1; }
    inline int base64_encode(lua_State* L) { return base64encode(L); }
    inline int base64_decode(lua_State* L) { return base64decode(L); }
    inline int lz4compress(lua_State* L) { size_t len=0; const char* d=luaL_checklstring(L,1,&len); std::string o=lz4_compress_block(reinterpret_cast<const uint8_t*>(d),len); lua_pushlstring(L,o.data(),o.size()); return 1; }
    inline int lz4decompress(lua_State* L) { size_t len=0; const char* d=luaL_checklstring(L,1,&len); std::string o; if(!lz4_decompress_block(reinterpret_cast<const uint8_t*>(d),len,o)) luaL_error(L,"invalid lz4 data"); lua_pushlstring(L,o.data(),o.size()); return 1; }

    inline int generatebytes(lua_State* L) {
        int size = luaL_checkinteger(L, 1); if (size<0||size>1048576) luaL_argerror(L,1,"size must be between 0 and 1048576");
        std::string bytes; if (!random_bytes(bytes, size)) luaL_error(L,"failed to generate random bytes");
        std::string enc = base64_encode_bytes(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
        lua_pushlstring(L, enc.data(), enc.size()); return 1;
    }
    inline int generatekey(lua_State* L) { lua_pushinteger(L, 32); return generatebytes(L); }

    inline int hash(lua_State* L) {
        size_t dl=0; const char* data=luaL_checklstring(L,1,&dl); const char* alg=luaL_optstring(L,2,"sha384"); std::string digest;
        if (_stricmp(alg,"md5")==0) { if(!cryptoapi_hash(CALG_MD5,data,dl,digest)) luaL_error(L,"hash failed"); }
        else if (_stricmp(alg,"sha1")==0) { if(!cryptoapi_hash(CALG_SHA1,data,dl,digest)) luaL_error(L,"hash failed"); }
        else if (_stricmp(alg,"sha256")==0) { if(!cryptoapi_hash(CALG_SHA_256,data,dl,digest)) luaL_error(L,"hash failed"); }
        else if (_stricmp(alg,"sha384")==0) { if(!cryptoapi_hash(CALG_SHA_384,data,dl,digest)) luaL_error(L,"hash failed"); }
        else if (_stricmp(alg,"sha512")==0) { if(!cryptoapi_hash(CALG_SHA_512,data,dl,digest)) luaL_error(L,"hash failed"); }
        else if (_stricmp(alg,"sha3-256")==0||_stricmp(alg,"sha3_256")==0) { digest=sha3_impl::sha3(256,data,dl); }
        else if (_stricmp(alg,"sha3-384")==0||_stricmp(alg,"sha3_384")==0) { digest=sha3_impl::sha3(384,data,dl); }
        else if (_stricmp(alg,"sha3-512")==0||_stricmp(alg,"sha3_512")==0) { digest=sha3_impl::sha3(512,data,dl); }
        else { luaL_argerror(L,2,"unsupported hash algorithm"); }
        lua_pushlstring(L, digest.data(), digest.size()); return 1;
    }

    inline int getfunctionhash(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        const auto cl = clvalue(const_cast<TValue*>(luaA_toobject(L, 1)));
        if (cl->isC) { luaL_argerror(L, 1, "lua function expected"); return 0; }
        const Proto* p = cl->l.p;
        std::string raw(reinterpret_cast<const char*>(p->code), p->sizecode * sizeof(Instruction));
        std::string result = sha384_impl::sha384(raw);
        lua_pushlstring(L, result.c_str(), result.size()); return 1;
    }
}
